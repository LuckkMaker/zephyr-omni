/*
 * Copyright (c) 2026 LuckkMaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/swdp.h>

#include <string.h>
#include <stdint.h>

#include "cmsis_dap.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dap, CONFIG_DAP_LOG_LEVEL);

#define DAP_STATE_CONNECTED 0

static uint8_t dap_info(struct dap_context *const ctx, const uint8_t *request, uint8_t *response)
{
	uint8_t *info = response + 1U;
	uint8_t id = request[0];
	uint8_t length = 0U;

	switch (id) {
	case DAP_ID_VENDOR:
		LOG_DBG("DAP_ID_VENDOR");
		length = (uint8_t)sizeof(CONFIG_DAP_PROBE_VENDOR);
		memcpy(info, CONFIG_DAP_PROBE_VENDOR, length);
		break;
	case DAP_ID_PRODUCT:
		LOG_DBG("DAP_ID_PRODUCT");
		length = (uint8_t)sizeof(CONFIG_DAP_PROBE_NAME);
		memcpy(info, CONFIG_DAP_PROBE_NAME, length);
		break;
	case DAP_ID_SER_NUM:
		LOG_DBG("DAP_ID_SER_NUM");
		if (ctx->ser_num_str_cb != NULL) {
			char *ser_num_str = ctx->ser_num_str_cb(ctx);
			if (ser_num_str != NULL) {
				strcpy((char *)info, ser_num_str);
				length = (uint8_t)(strlen(ser_num_str) + 1U);
			} else {
				LOG_ERR("Serial number string callback returned NULL");
			}
		} else {
			LOG_ERR("Serial number string callback is not registered");
		}
		break;
	case DAP_ID_DAP_FW_VER:
		LOG_DBG("DAP_ID_DAP_FW_VER");
		length = (uint8_t)sizeof(DAP_FW_VER);
		memcpy(info, DAP_FW_VER, length);
		break;
	case DAP_ID_DEVICE_VENDOR:
		LOG_DBG("DAP_ID_DEVICE_VENDOR");
		length = (uint8_t)sizeof(CONFIG_DAP_DEVICE_VENDOR);
		memcpy(info, CONFIG_DAP_DEVICE_VENDOR, length);
		break;
	case DAP_ID_DEVICE_NAME:
		LOG_DBG("DAP_ID_DEVICE_NAME");
		length = (uint8_t)sizeof(CONFIG_DAP_DEVICE_NAME);
		memcpy(info, CONFIG_DAP_DEVICE_NAME, length);
		break;
	case DAP_ID_BOARD_VENDOR:
		LOG_DBG("DAP_ID_BOARD_VENDOR");
		length = (uint8_t)sizeof(CONFIG_DAP_BOARD_VENDOR);
		memcpy(info, CONFIG_DAP_BOARD_VENDOR, length);
		break;
	case DAP_ID_BOARD_NAME:
		LOG_DBG("DAP_ID_BOARD_NAME");
		length = (uint8_t)sizeof(CONFIG_DAP_BOARD_NAME);
		memcpy(info, CONFIG_DAP_BOARD_NAME, length);
		break;
	case DAP_ID_PRODUCT_FW_VER:
		LOG_DBG("DAP_ID_PRODUCT_FW_VER");
		length = (uint8_t)sizeof(CONFIG_DAP_PROBE_FW_VER);
		memcpy(info, CONFIG_DAP_PROBE_FW_VER, length);
		break;
	case DAP_ID_CAPABILITIES:
		LOG_DBG("DAP_ID_CAPABILITIES");
		info[0] = ctx->capabilities;
		info[1] = ctx->capabilities >> 8U;
		length = 2U;
		break;
	case DAP_ID_TIMESTAMP_CLOCK:
		LOG_DBG("DAP_ID_TIMESTAMP_CLOCK");
#if defined(CONFIG_DAP_TIMESTAMP_CLOCK)
		sys_put_le32(CONFIG_DAP_TIMESTAMP_CLOCK, &info[0]);
		length = 4U;
#endif
		break;
	case DAP_ID_UART_RX_BUFFER_SIZE:
		LOG_DBG("DAP_ID_UART_RX_BUFFER_SIZE");
#if defined(CONFIG_DAP_UART) && defined(CONFIG_DAP_UART_RX_BUFFER_SIZE)
		sys_put_le32(CONFIG_DAP_UART_RX_BUFFER_SIZE, &info[0]);
		length = 4U;
#endif
		break;
	case DAP_ID_UART_TX_BUFFER_SIZE:
		LOG_DBG("DAP_ID_UART_TX_BUFFER_SIZE");
#if defined(CONFIG_DAP_UART) && defined(CONFIG_DAP_UART_TX_BUFFER_SIZE)
		sys_put_le32(CONFIG_DAP_UART_TX_BUFFER_SIZE, &info[0]);
		length = 4U;
#endif
		break;
	case DAP_ID_SWO_BUFFER_SIZE:
		LOG_DBG("DAP_ID_SWO_BUFFER_SIZE");
#if (defined(CONFIG_DAP_SWO_UART) || defined(CONFIG_DAP_SWO_MANCHESTER)) &&                        \
	defined(CONFIG_DAP_SWO_BUFFER_SIZE)
		sys_put_le32(CONFIG_DAP_SWO_BUFFER_SIZE, &info[0]);
		length = 4U;
#endif
		break;
	case DAP_ID_PACKET_SIZE:
		LOG_DBG("DAP_ID_PACKET_SIZE");
		sys_put_le16(ctx->pkt_size, &info[0]);
		length = 2U;
		break;
	case DAP_ID_PACKET_COUNT:
		LOG_DBG("DAP_ID_PACKET_COUNT");
		info[0] = (uint8_t)(CONFIG_DAP_PACKET_COUNT);
		length = 1U;
		break;
	default:
		LOG_ERR("Unknown DAP Info ID: 0x%02X", id);
		break;
	}

	return length;
}

static uint32_t dap_host_status(struct dap_context *const ctx, const uint8_t *request,
				uint8_t *response)
{
	switch (*request) {
	case DAP_DEBUGGER_CONNECTED:
		if ((*request + 1) & 1U) {
			LOG_DBG("Debugger connected");
			if (ctx->msg_cb != NULL) {
				ctx->msg_cb(ctx, DAP_MSG_CONNECTED);
			}
		}
		break;
	case DAP_TARGET_RUNNING:
		if ((*request + 1) & 1U) {
			LOG_DBG("Target running");
			if (ctx->msg_cb != NULL) {
				ctx->msg_cb(ctx, DAP_MSG_RUNNING);
			}
		}
		break;
	default:
		LOG_ERR("Unknown Host Status ID: 0x%02X", *request);
		*response = DAP_ERROR;
		return ((2U << 16) | 1U);
	}

	*response = DAP_OK;
	return ((2U << 16) | 1U);
}

static uint32_t dap_connect(struct dap_context *const ctx, const uint8_t *request,
			    uint8_t *response)
{
#if (CONFIG_DAP_SWD != 0U)
	const struct swdp_api *swdp_api = ctx->swdp_dev->api;
#endif

#if (CONFIG_DAP_JTAG != 0U)
	const struct device *jtagdp_dev = ctx->jtagdp_dev;
#endif
	uint32_t port;

	if (*request == DAP_PORT_AUTODETECT) {
		port = CONFIG_DAP_DEFAULT_PORT;
	} else {
		port = *request;
	}

	switch (port) {
#if (CONFIG_DAP_SWD != 0U)
	case DAP_PORT_SWD:
		ctx->debug_port = DAP_PORT_SWD;

		if (atomic_test_and_set_bit(&ctx->state, DAP_STATE_CONNECTED)) {
			LOG_ERR("DAP device is already connected");
			break;
		}

		swdp_api->swdp_port_on(ctx->swdp_dev);
		break;
#endif

#if (CONFIG_DAP_JTAG != 0U)
	case DAP_PORT_JTAG:
		ctx->debug_port = DAP_PORT_JTAG;

		if (atomic_test_and_set_bit(&ctx->state, DAP_STATE_CONNECTED)) {
			LOG_ERR("DAP device is already connected");
			break;
		}

		jtagdp_dev->api->jtagdp_port_on(jtagdp_dev);
		break;
#endif
	default:
		LOG_DBG("port disabled");
		ctx->debug_port = DAP_PORT_DISABLED;
		break;
	}

	*response = (uint8_t)port;
	return ((1U << 16) | 1U);
}

static uint32_t dap_disconnect(struct dap_context *const ctx, const uint8_t *request,
			       uint8_t *response)
{
#if (CONFIG_DAP_SWD != 0U)
	const struct swdp_api *swdp_api = ctx->swdp_dev->api;
#endif

#if (CONFIG_DAP_JTAG != 0U)
	const struct device *jtagdp_dev = ctx->jtagdp_dev;
#endif

	ctx->debug_port = DAP_PORT_DISABLED;

	if (atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
#if (CONFIG_DAP_SWD != 0U)
		swdp_api->swdp_port_off(ctx->swdp_dev);
#endif
#if (CONFIG_DAP_JTAG != 0U)
		jtagdp_dev->api->jtagdp_port_off(jtagdp_dev);
#endif
	} else {
		LOG_WRN("DAP device is not connected");
	}

	*response = DAP_OK;
	atomic_clear_bit(&ctx->state, DAP_STATE_CONNECTED);

	return (1U);
}

static uint32_t dap_delay(struct dap_context *const ctx, const uint8_t *request, uint8_t *response)
{
	uint16_t delay = sys_get_le16(&request[0]);

	LOG_DBG("DAP delay %u ms", delay);

	k_busy_wait(delay * USEC_PER_MSEC);
	*response = DAP_OK;

	return ((2U << 16) | 1U);
}

static uint32_t dap_reset_target(struct dap_context *const ctx, const uint8_t *request,
				 uint8_t *response)
{
	*(response + 1) = 0U; // Unsupported reset options, return 0 in response value
	*(response + 0) = DAP_OK;

	LOG_WRN("Reset Target command unsupported");

	return (2U);
}

static uint32_t dap_swj_pins(struct dap_context *const ctx, const uint8_t *request,
			     uint8_t *response)
{
#if (CONFIG_DAP_SWD != 0U) || (CONFIG_DAP_JTAG != 0U)
	const struct swdp_api *swdp_api = ctx->swdp_dev->api;

	uint8_t value = request[0];
	uint8_t select = request[1];
	uint32_t wait = sys_get_le32(&request[2]);
	k_timepoint_t end = sys_timepoint_calc(K_USEC(wait));
	uint8_t state;

	if (atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		/* Skip if nothing selected. */
		if (select) {
			swdp_api->swdp_set_pins(ctx->swdp_dev, select, value);
		}

		do {
			swdp_api->swdp_get_pins(ctx->swdp_dev, &state);
			LOG_INF("select 0x%02x, value 0x%02x, wait %u, state 0x%02x", select, value,
				wait, state);
			if ((value & select) == (state & select)) {
				LOG_DBG("swdp_get_pins succeeded before timeout");
				break;
			}
		} while (!sys_timepoint_expired(end));
	} else {
		LOG_ERR("DAP device is not connected");
		*response = DAP_ERROR;
	}
#else
	*response = 0U;
#endif

	return ((6U << 16) | 1U);
}

static uint32_t dap_swj_clock(struct dap_context *const ctx, const uint8_t *request,
			      uint8_t *response)
{
#if (CONFIG_DAP_SWD != 0U) || (CONFIG_DAP_JTAG != 0U)
	const struct swdp_api *swdp_api = ctx->swdp_dev->api;

	uint32_t clock = sys_get_le32(&request[0]);

	LOG_DBG("DAP SWJ clock %u Hz", clock);

	if (atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		if (clock) {
			swdp_api->swdp_set_clock(ctx->swdp_dev, clock);
			*response = DAP_OK;
		} else {
			*response = DAP_ERROR;
		}
	} else {
		LOG_WRN("DAP device is not connected");
		*response = DAP_OK;
	}
#else
	*response = DAP_ERROR;
#endif

	return ((4U << 16) | 1U);
}

static uint32_t dap_swj_sequence(struct dap_context *const ctx, const uint8_t *request,
				 uint8_t *response)
{
	uint32_t count = request[0];

#if (CONFIG_DAP_SWD != 0U) || (CONFIG_DAP_JTAG != 0U)
	const struct swdp_api *swdp_api = ctx->swdp_dev->api;

	LOG_DBG("DAP SWJ sequence count %u", count);

	count = MIN(count, 256U);

	if (atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		swdp_api->swdp_output_sequence(ctx->swdp_dev, count, &request[1]);
		*response = DAP_OK;
	} else {
		LOG_ERR("DAP device is not connected");
		*response = DAP_ERROR;
	}
#else
	*response = DAP_ERROR;
#endif

	count = (count + 7U) >> 3;
	return (((count + 1U) << 16) | 1U);
}

static uint32_t dap_swd_configure(struct dap_context *const ctx, const uint8_t *request,
				  uint8_t *response)
{
#if (CONFIG_DAP_SWD != 0U)
	const struct swdp_api *swdp_api = ctx->swdp_dev->api;
	uint8_t value = *request;
	uint8_t turnaround = (value & 0x03U) + 1U;
	bool data_phase = (value & 0x04U) ? true : false;

	if (atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		swdp_api->swdp_configure(ctx->swdp_dev, turnaround, data_phase);
		*response = DAP_OK;
	} else {
		LOG_ERR("DAP device is not connected");
		*response = DAP_ERROR;
	}
#else
	*response = DAP_ERROR;
#endif

	return ((1U << 16) | 1U);
}

static uint32_t dap_swd_sequence(struct dap_context *const ctx, const uint8_t *request,
				 uint8_t *response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	const uint8_t *request_data = request + 1;
	uint8_t *response_data = response + 1;
	uint8_t count = request[0];
	uint8_t num_cycles;
	uint32_t num_bytes;
	bool input;
	uint32_t request_count;
	uint32_t response_count;

	switch (ctx->debug_port) {
	case DAP_PORT_SWD:
		response[0] = DAP_OK;
		break;
	case DAP_PORT_JTAG:
	default:
		LOG_ERR("port unsupported");
		response[0] = DAP_ERROR;
		return ((1U << 16) | 1U);
	}

	for (size_t i = 0; i < count; ++i) {
		input = *request_data & BIT(7);
		num_cycles = *request_data & BIT_MASK(7);
		num_bytes = (num_cycles + 7) >> 3; /* rounded up to full bytes */

		if (num_cycles == 0) {
			num_cycles = 64;
		}

		request_data += 1;

		if (input) {
			api->swdp_input_sequence(ctx->swdp_dev, num_cycles, response_data);
			response_data += num_bytes;
		} else {
			api->swdp_output_sequence(ctx->swdp_dev, num_cycles, request_data);
			request_data += num_bytes;
		}
	}

	request_count = request_data - request;
	response_count = response_data - response;

	return ((request_count << 16) | response_count);
}

static uint32_t dap_transfer_configure(struct dap_context *const ctx, const uint8_t *request,
				       uint8_t *response)
{
	ctx->transfer.idle_cycles = request[0];
	ctx->transfer.retry_count = sys_get_le16(&request[1]);
	ctx->transfer.match_retry = sys_get_le16(&request[3]);
	LOG_DBG("DAP transfer configure: idle_cycles %u, retry_count %u, match_retry %u",
		ctx->transfer.idle_cycles, ctx->transfer.retry_count, ctx->transfer.match_retry);

	*response = DAP_OK;
	return ((5U << 16) | 1U);
}

static uint32_t dap_swd_transfer(struct dap_context *const ctx, const uint8_t *request,
				 uint8_t *response)
{
	uint8_t *rspns_buf;
	const uint8_t *req_buf;
	uint8_t rspns_cnt = 0;
	uint8_t rspns_val = 0;
	bool post_read = false;
	uint32_t check_write = 0;
	uint8_t req_cnt;
	uint8_t req_val;
	uint32_t match_val;
	uint32_t data;

	/* Ignore DAP index request[0] */
	req_cnt = request[1];
	req_buf = request + sizeof(req_cnt) + 1;
	rspns_buf = response + (sizeof(rspns_cnt) + sizeof(rspns_val));

	for (; req_cnt; req_cnt--) {
		req_val = *req_buf++;
		if (req_val & SWDP_REQUEST_RnW) {
			/* Read register */
			if (post_read) {
				/*
				 * Read was posted before, read previous AP
				 * data or post next AP read.
				 */
				if ((req_val & (SWDP_REQUEST_APnDP | DAP_TRANSFER_MATCH_VALUE)) !=
				    SWDP_REQUEST_APnDP) {
					req_val = DP_RDBUFF | SWDP_REQUEST_RnW;
					post_read = false;
				}

				rspns_val = do_swdp_transfer(ctx, req_val, &data);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}

				/* Store previous AP data */
				sys_put_le32(data, rspns_buf);
				rspns_buf += sizeof(data);
			}
			if (req_val & DAP_TRANSFER_MATCH_VALUE) {
				LOG_INF("match value read");
				/* Read with value match */
				match_val = sys_get_le32(req_buf);
				req_buf += sizeof(match_val);

				rspns_val = swdp_transfer_match(ctx, req_val, match_val);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}

			} else if (req_val & SWDP_REQUEST_APnDP) {
				/* Normal read */
				if (!post_read) {
					/* Post AP read */
					rspns_val = do_swdp_transfer(ctx, req_val, NULL);
					if (rspns_val != SWDP_ACK_OK) {
						break;
					}
					post_read = true;
				}
			} else {
				/* Read DP register */
				rspns_val = do_swdp_transfer(ctx, req_val, &data);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}
				/* Store data */
				sys_put_le32(data, rspns_buf);
				rspns_buf += sizeof(data);
			}
			check_write = 0U;
		} else {
			/* Write register */
			if (post_read) {
				/* Read previous data */
				rspns_val =
					do_swdp_transfer(ctx, DP_RDBUFF | SWDP_REQUEST_RnW, &data);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}

				/* Store previous data */
				sys_put_le32(data, rspns_buf);
				rspns_buf += sizeof(data);
				post_read = false;
			}
			/* Load data */
			data = sys_get_le32(req_buf);
			req_buf += sizeof(data);
			if (req_val & DAP_TRANSFER_MATCH_MASK) {
				/* Write match mask */
				ctx->transfer.match_mask = data;
				rspns_val = SWDP_ACK_OK;
			} else {
				/* Write DP/AP register */
				rspns_val = do_swdp_transfer(ctx, req_val, &data);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}

				check_write = 1U;
			}
		}
		rspns_cnt++;
	}

	while (req_cnt) {
		req_val = *req_buf++;
		if ((req_val & SWDP_REQUEST_RnW) != 0U) {
			/* Read register */
			if ((req_val & DAP_TRANSFER_MATCH_VALUE) != 0U) {
				/* Read with value match */
				req_buf += 4;
			}
		} else {
			/* Write register */
			req_buf += 4;
		}
		req_cnt--;
	}

	if (rspns_val == SWDP_ACK_OK) {
		if (post_read) {
			/* Read previous data */
			rspns_val = do_swdp_transfer(ctx, DP_RDBUFF | SWDP_REQUEST_RnW, &data);
			if (rspns_val != SWDP_ACK_OK) {
				goto end;
			}

			/* Store previous data */
			sys_put_le32(data, rspns_buf);
			rspns_buf += sizeof(data);
		} else if (check_write) {
			/* Check last write */
			rspns_val = do_swdp_transfer(ctx, DP_RDBUFF | SWDP_REQUEST_RnW, NULL);
		}
	}

end:
	response[0] = rspns_cnt;
	response[1] = rspns_val;

	return (((uint32_t)(req_buf - request) << 16) | (uint32_t)(rspns_buf - response));
}

static uint32_t dap_dummy_transfer(struct dap_context *const ctx, const uint8_t *request,
				   uint8_t *response)
{
	uint8_t *request_head;
	uint32_t request_count;
	uint32_t request_value;

	request_head = request;

	request++; // Ignore DAP index

	request_count = *request++;

	for (; request_count != 0U; request_count--) {
		// Process dummy requests
		request_value = *request++;
		if ((request_value & DAP_TRANSFER_RnW) != 0U) {
			// Read register
			if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U) {
				// Read with value match
				request += 4;
			}
		} else {
			// Write register
			request += 4;
		}
	}

	*(response + 0) = 0U; // Response count
	*(response + 1) = 0U; // Response value

	return (((uint32_t)(request - request_head) << 16) | 2U);
}

static uint32_t dap_transfer(struct dap_context *const ctx, const uint8_t *request,
			     uint8_t *response)
{
	uint32_t ret;

	if (atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		switch (ctx->debug_port) {
		case DAP_PORT_SWD:
			ret = dap_swd_transfer(ctx, request, response);
			break;
		case DAP_PORT_JTAG:
		default:
			ret = dap_dummy_transfer(ctx, request, response);
		}
	} else {
		LOG_ERR("DAP device is not connected");
		response[0] = DAP_ERROR;
	}

	return ret;
}

/**
 * @brief Process DAP command request and prepare response
 *
 * @param ctx DAP context
 * @param request Pointer to request data
 * @param response Pointer to response data
 * @return number of bytes in response (lower 16 bits)
 *         number of bytes in request (upper 16 bits)
 */
static uint32_t dap_process_command(struct dap_context *const ctx, const uint8_t *request,
				    uint8_t *response)
{
	uint32_t ret;

	LOG_HEXDUMP_DBG(request, 8, "DAP Command Request");

	if ((*request >= ID_DAP_VENDOR0) && (*request <= ID_DAP_VENDOR31)) {
		if (ctx->vendor_cmd_cb != NULL) {
			return ctx->vendor_cmd_cb(ctx, request, response);
		} else {
			LOG_ERR("Vendor command callback is not registered");
			*response = ID_DAP_INVALID;
			return ((1U << 16) | 1U);
		}
	}

	*response++ = *request;
	LOG_DBG("DAP Command ID: 0x%02X", request[0]);

	switch (*request++) {
	case ID_DAP_INFO:
		ret = dap_info(ctx, request, response);
		*response = (uint8_t)ret;
		return ((2U << 16) + 2U + ret);
	case ID_DAP_HOST_STATUS:
		ret = dap_host_status(ctx, request, response);
		break;
	case ID_DAP_CONNECT:
		ret = dap_connect(ctx, request, response);
		break;
	case ID_DAP_DISCONNECT:
		ret = dap_disconnect(ctx, request, response);
		break;
	case ID_DAP_DELAY:
		ret = dap_delay(ctx, request, response);
		break;
	case ID_DAP_RESET_TARGET:
		ret = dap_reset_target(ctx, request, response);
		break;
	case ID_DAP_SWJ_PINS:
		ret = dap_swj_pins(ctx, request, response);
		break;
	case ID_DAP_SWJ_CLOCK:
		ret = dap_swj_clock(ctx, request, response);
		break;
	case ID_DAP_SWJ_SEQUENCE:
		ret = dap_swj_sequence(ctx, request, response);
		break;
	case ID_DAP_SWD_CONFIGURE:
		ret = dap_swd_configure(ctx, request, response);
		break;
	case ID_DAP_SWD_SEQUENCE:
		ret = dap_swd_sequence(ctx, request, response);
		break;
	case ID_DAP_JTAG_SEQUENCE:
		LOG_WRN("DAP JTAG sequence command not implemented");
		*response = DAP_ERROR;
		ret = (1U << 16) | 1U;
		break;
	case ID_DAP_JTAG_CONFIGURE:
		LOG_WRN("DAP JTAG configure command not implemented");
		*response = DAP_ERROR;
		ret = (1U << 16) | 1U;
		break;
	case ID_DAP_JTAG_IDCODE:
		LOG_WRN("DAP JTAG IDCODE command not implemented");
		*response = DAP_ERROR;
		ret = (1U << 16) | 1U;
		break;
	case ID_DAP_TRANSFER_CONFIGURE:
		ret = dap_transfer_configure(ctx, request, response);
		break;
	case ID_DAP_TRANSFER:
		ret = dap_transfer(ctx, request, response);
		break;

	default:
		LOG_ERR("Unknown DAP command ID: 0x%02X", request[-1]);
		*(response - 1) = ID_DAP_INVALID;
		return (1U << 16) | 1U;
	}

	return ((1U << 16) + 1U + ret);
}

/*
 * Execute DAP command (process request and prepare response)
 *   request:  pointer to request data
 *   response: pointer to response data
 *   return:   number of bytes in response (lower 16 bits)
 *             number of bytes in request (upper 16 bits)
 */
uint32_t dap_execute_command(struct dap_context *const ctx, const uint8_t *request,
			     uint8_t *response)
{
	uint32_t cnt;
	uint32_t n;
	uint32_t ret;

	if (*request == ID_DAP_EXECUTE_COMMANDS) {
		*response++ = *request++;
		cnt = *request++;
		*response++ = (uint8_t)cnt;
		ret = (2U << 16) | 2U;
		while (cnt--) {
			n = dap_process_command(ctx, request, response);
			ret += n;
			request += (uint16_t)(n >> 16);
			response += (uint16_t)n;
		}
		return ret;
	}

	return dap_process_command(ctx, request, response);
}

void dap_update_pkt_size(struct dap_context *const ctx, const uint16_t pkt_size)
{
	ctx->pkt_size = pkt_size;
	LOG_INF("New packet size %u", pkt_size);
}

int dap_vendor_cmd_register_cb(struct dap_context *const ctx, const dap_vendor_cmd_cb_t cb)
{
	int ret = 0;

	if (ctx->vendor_cmd_cb != NULL) {
		LOG_ERR("Vendor command callback already registered");
		return -EALREADY;
	}

	ctx->vendor_cmd_cb = cb;

	return ret;
}

int dap_ser_num_str_register_cb(struct dap_context *const ctx, const dap_serial_number_str_cb_t cb)
{
	int ret = 0;

	if (ctx->ser_num_str_cb != NULL) {
		LOG_ERR("Serial number string callback already registered");
		return -EALREADY;
	}

	ctx->ser_num_str_cb = cb;

	return ret;
}

int dap_msg_register_cb(struct dap_context *const ctx, const dap_msg_cb_t cb)
{
	int ret = 0;

	if (ctx->msg_cb != NULL) {
		LOG_ERR("DAP message callback already registered");
		return -EALREADY;
	}

	ctx->msg_cb = cb;

	return ret;
}

int dap_setup(struct dap_context *const ctx)
{
	if (ctx == NULL) {
		LOG_ERR("DAP context is NULL");
		return -EINVAL;
	}

#if (CONFIG_DAP_SWD != 0U)
	if ((ctx->swdp_dev != NULL) && !device_is_ready(ctx->swdp_dev)) {
		LOG_ERR("SWDP device is not ready");
		return -ENODEV;
	}
#endif

#if (CONFIG_DAP_JTAG != 0U)
	if ((ctx->jtagdp_dev != NULL) && !device_is_ready(ctx->jtagdp_dev)) {
		LOG_ERR("JTAGDP device is not ready");
		return -ENODEV;
	}
#endif

	/* Default settings */
	ctx->debug_port = 0U;
	ctx->pkt_size = CONFIG_DAP_PACKET_SIZE;
	ctx->transfer.idle_cycles = 0U;
	ctx->transfer.retry_count = 100U;
	ctx->transfer.match_retry = 0U;
	ctx->transfer.match_mask = 0x00000000U;

	/* Configure capabilities based on enabled features */
#if (CONFIG_DAP_SWD != 0U)
	ctx->capabilities |= DAP_CAP_SWD;
#endif
#if (CONFIG_DAP_JTAG != 0U)
	ctx->capabilities |= DAP_CAP_JTAG;
#endif
#if (CONFIG_DAP_SWO_UART != 0U)
	ctx->capabilities |= DAP_CAP_SWO_UART;
#endif
#if (CONFIG_DAP_SWO_MANCHESTER != 0U)
	ctx->capabilities |= DAP_CAP_SWO_MANCHESTER;
#endif
	ctx->capabilities |= DAP_CAP_ATOMIC_COMMANDS;
#if (CONFIG_DAP_TIMESTAMP_CLOCK != 0U)
	ctx->capabilities |= DAP_CAP_TIMESTAMP_CLOCK;
#endif
#if (CONFIG_DAP_SWO_STREAM != 0U)
	ctx->capabilities |= DAP_CAP_SWO_STREAM;
#endif
#if (CONFIG_DAP_UART != 0U)
	ctx->capabilities |= DAP_CAP_UART;
#endif
#if (CONFIG_DAP_UART_USB_COM_PORT != 0U)
	ctx->capabilities |= DAP_CAP_UART_USB_COM_PORT;
#endif

	// #ifdef CONFIG_DAP_SWD
	// 	ctx->swd_conf.turnaround = 1U;
	// 	ctx->swd_conf.data_phase = 0U;
	// #endif
	// #ifdef CONFIG_DAP_JTAG
	// 	ctx->jtagdp_dev.count = 0U;
	// #endif

	return 0;
}
