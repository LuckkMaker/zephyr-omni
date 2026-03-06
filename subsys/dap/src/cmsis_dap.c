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

// static struct dap_context dap_ctx[1];

static uint32_t dap_process_command(struct dap_context *const ctx,
				    const uint8_t *const request,
				    uint8_t *response)
{
	uint32_t ret;

	LOG_HEXDUMP_DBG(request, 8, "DAP Command Request");

	if ((*request >= ID_DAP_VENDOR0) && (*request <= ID_DAP_VENDOR31)) {
		// return DAP_ProcessVendorCommand(request, response);
	}

	*response++ = *request;
}

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
