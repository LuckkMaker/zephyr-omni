/*
 * Copyright (c) 2026 LuckkMaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include "dap_backend_usb.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dap_usb, CONFIG_DAP_LOG_LEVEL);

NET_BUF_POOL_FIXED_DEFINE(dap_queue_out_pool, 1, 0, sizeof(struct udc_buf_info), NULL);
NET_BUF_POOL_FIXED_DEFINE(dap_queue_in_pool, 1, 0, sizeof(struct udc_buf_info), NULL);
UDC_STATIC_BUF_DEFINE(dap_queue_out_buf, DAP_QUEUE_PACKET_SIZE_MAX);
UDC_STATIC_BUF_DEFINE(dap_queue_in_buf, DAP_QUEUE_PACKET_SIZE_MAX);

#define DAP_QUEUE_FUNCTION_ENABLED 0
#define DAP_QUEUE_IN_BUSY          1

static uint8_t dap_queue_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct dap_queue_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_queue_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}
	return desc->if0_out_ep.bEndpointAddress;
}

static uint8_t dap_queue_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct dap_queue_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_queue_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}
	return desc->if0_in_ep.bEndpointAddress;
}

static void dap_queue_try_send_in(struct usbd_class_data *c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_queue_data *data = usbd_class_get_private(c_data);
	struct net_buf *buf;
	struct udc_buf_info *bi;
	uint16_t n;
	uint16_t len;
	uint8_t ep_in = dap_queue_get_bulk_in(c_data);

	if (atomic_test_bit(&data->state, DAP_QUEUE_IN_BUSY)) {
		return;
	}

	k_mutex_lock(&data->queue_mutex, K_FOREVER);
	if (data->response_count == 0) {
		k_mutex_unlock(&data->queue_mutex);
		return;
	}
	n = data->response_index_o;
	len = data->response_len[n];
	data->response_index_o++;
	if (data->response_index_o == DAP_QUEUE_PACKET_COUNT) {
		data->response_index_o = 0;
	}
	data->response_count--;
	k_mutex_unlock(&data->queue_mutex);

	buf = net_buf_alloc_with_data(&dap_queue_in_pool, dap_queue_in_buf,
				      DAP_QUEUE_PACKET_SIZE_MAX, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("Failed to alloc IN buf");
		k_mutex_lock(&data->queue_mutex, K_FOREVER);
		data->response_count++;
		data->response_index_o = (data->response_index_o == 0) ? DAP_QUEUE_PACKET_COUNT - 1
								       : data->response_index_o - 1;
		k_mutex_unlock(&data->queue_mutex);
		return;
	}
	net_buf_reset(buf);
	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep_in;
	net_buf_add_mem(buf, data->response_buf[n], len);

	atomic_set_bit(&data->state, DAP_QUEUE_IN_BUSY);

	if (usbd_ep_enqueue(c_data, buf)) {
		atomic_clear_bit(&data->state, DAP_QUEUE_IN_BUSY);
		usbd_ep_buf_free(uds_ctx, buf);
		LOG_ERR("Failed to enqueue IN");
	}
}

/* Process request queue: handle DAP_QueueCommands (0x7E) batching per CMSIS-DAP spec.
 * Queued commands execute when a packet without DAP_QueueCommands is received.
 */
static void dap_queue_process_work(struct k_work *work)
{
	struct dap_queue_data *data = CONTAINER_OF(work, struct dap_queue_data, process_work);
	struct usbd_class_data *c_data = data->class_data;
	uint16_t i;

	k_mutex_lock(&data->queue_mutex, K_FOREVER);

	while (data->request_count > 0) {
		uint16_t first_non_offset = 0; /* offset from request_index_o */

		/* Find first packet that is not DAP_QueueCommands (0x7E).
		 * Per spec we only execute when we see a non-QueueCommands packet.
		 */
		for (i = 0; i < data->request_count; i++) {
			uint16_t idx = (data->request_index_o + i) % DAP_QUEUE_PACKET_COUNT;

			if (data->request_buf[idx][0] != ID_DAP_QUEUE_COMMANDS) {
				first_non_offset = i;
				break;
			}
			if (i == data->request_count - 1) {
				/* All pending are QueueCommands, wait for more packets */
				first_non_offset = (uint16_t)-1;
			}
		}
		if (first_non_offset == (uint16_t)-1) {
			break;
		}

		/* Execute from request_index_o up to and including first_non_offset (inclusive) */
		for (i = 0; i <= first_non_offset; i++) {
			uint8_t *req = data->request_buf[data->request_index_o];
			uint8_t *resp = data->response_buf[data->response_index_i];
			uint32_t resp_len;

			if (req[0] == ID_DAP_QUEUE_COMMANDS) {
				req[0] = ID_DAP_EXECUTE_COMMANDS;
			}
			resp_len = dap_execute_command(req, resp);
			if (resp_len > DAP_QUEUE_PACKET_SIZE_MAX) {
				resp_len = DAP_QUEUE_PACKET_SIZE_MAX;
			}
			data->response_len[data->response_index_i] = (uint16_t)resp_len;

			data->request_index_o++;
			if (data->request_index_o == DAP_QUEUE_PACKET_COUNT) {
				data->request_index_o = 0;
			}
			data->request_count--;

			data->response_index_i++;
			if (data->response_index_i == DAP_QUEUE_PACKET_COUNT) {
				data->response_index_i = 0;
			}
			data->response_count++;
		}
	}

	k_mutex_unlock(&data->queue_mutex);

	/* Try to start IN transfer if we have responses and IN is idle */
	dap_queue_try_send_in(c_data);
}

static int dap_queue_request_handler(struct usbd_class_data *c_data, struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_queue_data *data = usbd_class_get_private(c_data);
	struct udc_buf_info *bi;
	uint8_t ep;
	size_t len;

	bi = (struct udc_buf_info *)net_buf_user_data(buf);
	ep = bi->ep;
	LOG_DBG("Transfer finished ep 0x%02x len %u err %d", ep, buf->len, err);

	if (!atomic_test_bit(&data->state, DAP_QUEUE_FUNCTION_ENABLED) || err != 0) {
		usbd_ep_buf_free(uds_ctx, buf);
		return 0;
	}

	if (ep == dap_queue_get_bulk_in(c_data)) {
		/* IN complete: one response sent */
		atomic_clear_bit(&data->state, DAP_QUEUE_IN_BUSY);
		usbd_ep_buf_free(uds_ctx, buf);
		dap_queue_try_send_in(c_data);
		return 0;
	}

	/* OUT complete: copy to request queue and re-enqueue OUT buffer */
	len = buf->len;
	if (len > 0 && len <= DAP_QUEUE_PACKET_SIZE_MAX) {
		k_mutex_lock(&data->queue_mutex, K_FOREVER);
		if (data->request_count < DAP_QUEUE_PACKET_COUNT) {
			uint16_t idx = data->request_index_i;

			memcpy(data->request_buf[idx], buf->data, len);
			data->request_len[idx] = (uint32_t)len;
			data->request_index_i++;
			if (data->request_index_i == DAP_QUEUE_PACKET_COUNT) {
				data->request_index_i = 0;
			}
			data->request_count++;

			if (buf->data[0] == ID_DAP_TRANSFER_ABORT) {
				/* Optional: set DAP_TransferAbort if supported by cmsis_dap.c */
			}
			k_work_submit(&data->process_work);
		}
		k_mutex_unlock(&data->queue_mutex);
	}

	/* Re-use same buffer for next OUT */
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = dap_queue_get_bulk_out(c_data);
	net_buf_reset(buf);
	if (usbd_ep_enqueue(c_data, buf)) {
		LOG_ERR("Failed to re-enqueue OUT buffer");
		usbd_ep_buf_free(uds_ctx, buf);
	}
	return 0;
}

static void *dap_queue_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	struct dap_queue_data *data = usbd_class_get_private(c_data);

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}
	return data->fs_desc;
}

static struct net_buf *dap_queue_buf_alloc(struct usbd_class_data *const c_data, const uint8_t ep)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf;
	struct udc_buf_info *bi;
	size_t size;

	size = (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) ? 512U : 64U;

	if (ep == dap_queue_get_bulk_in(c_data)) {
		buf = net_buf_alloc_with_data(&dap_queue_in_pool, dap_queue_in_buf, size,
					      K_NO_WAIT);
	} else {
		buf = net_buf_alloc_with_data(&dap_queue_out_pool, dap_queue_out_buf, size,
					      K_NO_WAIT);
	}
	if (!buf) {
		return NULL;
	}
	net_buf_reset(buf);
	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;
	return buf;
}

static void dap_queue_enable(struct usbd_class_data *const c_data)
{
	struct dap_queue_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf;

	LOG_INF("DAP USB queue backend enabled");

	if (!atomic_test_and_set_bit(&data->state, DAP_QUEUE_FUNCTION_ENABLED)) {
		if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
			dap_update_pkt_size(512);
		} else {
			dap_update_pkt_size(64);
		}
		data->class_data = c_data;

		buf = dap_queue_buf_alloc(c_data, dap_queue_get_bulk_out(c_data));
		if (buf) {
			if (usbd_ep_enqueue(c_data, buf)) {
				usbd_ep_buf_free(uds_ctx, buf);
				LOG_ERR("Failed to enqueue OUT buffer");
			}
		} else {
			LOG_ERR("Failed to allocate OUT buffer");
		}
	}
}

static void dap_queue_disable(struct usbd_class_data *const c_data)
{
	struct dap_queue_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, DAP_QUEUE_FUNCTION_ENABLED);
	atomic_clear_bit(&data->state, DAP_QUEUE_IN_BUSY);
	LOG_INF("DAP USB queue backend disabled");
}

static int dap_queue_init(struct usbd_class_data *c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_queue_data *data = usbd_class_get_private(c_data);
	struct dap_queue_desc *desc = data->desc;

	LOG_DBG("Init DAP queue class %p", (void *)c_data);

	k_mutex_init(&data->queue_mutex);
	k_work_init(&data->process_work, dap_queue_process_work);
	data->class_data = c_data;

	if (usbd_add_descriptor(uds_ctx, data->iface_str_desc_nd)) {
		LOG_ERR("Failed to add interface string descriptor");
		return -EINVAL;
	}
	desc->if0.iInterface = usbd_str_desc_get_idx(data->iface_str_desc_nd);
	return 0;
}

struct usbd_class_api dap_queue_api = {
	.request = dap_queue_request_handler,
	.get_desc = dap_queue_get_desc,
	.enable = dap_queue_enable,
	.disable = dap_queue_disable,
	.init = dap_queue_init,
};
