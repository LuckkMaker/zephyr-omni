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

static uint8_t dap_info(struct dap_context *const ctx,
						const uint8_t *request,
						uint8_t *response)
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
		default:
			break;
	}

	return length;
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
static uint32_t dap_process_command(struct dap_context *const ctx,
				    const uint8_t *request,
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
uint32_t dap_execute_command(struct dap_context *const ctx, const uint8_t *request, uint8_t *response)
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
	// ctx->pkt_size = pkt_size;
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

int dap_setup(struct dap_context *const ctx)
{
	if (ctx == NULL) {
		LOG_ERR("DAP context is NULL");
		return -EINVAL;
	}

	if ((ctx->swdp_dev != NULL) && !device_is_ready(ctx->swdp_dev)) {
		LOG_ERR("SWDP device is not ready");
		return -ENODEV;
	}

#ifdef CONFIG_DAP_JTAG
	if ((ctx->jtagdp_dev != NULL) && !device_is_ready(ctx->jtagdp_dev)) {
		LOG_ERR("JTAGDP device is not ready");
		return -ENODEV;
	}
#endif

	/* Default settings */
	ctx->debug_port = 0U;
	ctx->transfer.idle_cycles = 0U;
	ctx->transfer.retry_count = 100U;
	ctx->transfer.match_retry = 0U;
	ctx->transfer.match_mask = 0x00000000U;
// #ifdef CONFIG_DAP_SWD
// 	ctx->swd_conf.turnaround = 1U;
// 	ctx->swd_conf.data_phase = 0U;
// #endif
// #ifdef CONFIG_DAP_JTAG
// 	ctx->jtagdp_dev.count = 0U;
// #endif

	return 0;
}
