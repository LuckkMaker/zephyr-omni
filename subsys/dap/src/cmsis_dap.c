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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dap, CONFIG_DAP_LOG_LEVEL);

static struct dap_context dap_ctx[1];

static uint32_t dap_process_command(const struct dap_context *const ctx,
				    const uint8_t *const request,
				    uint8_t *const response);

static uint32_t dap_process_command(const struct dap_context *const ctx,
				    const uint8_t *const request,
				    uint8_t *const response)
{
	uint32_t ret;

	LOG_HEXDUMP_DBG(request, 8, "DAP Command Request");

	if ((*request >= ID_DAP_VENDOR0) && (*request <= ID_DAP_VENDOR31)) {
		return DAP_ProcessVendorCommand(request, response);
	}

	*response++ = *request;
}

uint32_t dap_execute_command(const uint8_t *request, uint8_t *response)
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
			n = dap_process_command(request, response);
			ret += n;
			request += (uint16_t)(n >> 16);
			response += (uint16_t)n;
		}
		return ret;
	}

	return dap_process_command(&dap_ctx[0], request, response);
}

int dap_setup(const struct device *const dev)
{
	dap_ctx[0].swdp_dev = (void *)dev;

	if (!device_is_ready(dap_ctx[0].swdp_dev)) {
		LOG_ERR("SWDP device is not ready");
		return -ENODEV;
	}

#ifdef CONFIG_DAP_JTAG
	//TODO: Initialize JTAG device
#endif

	/* Default settings */
	dap_ctx[0].debug_port = 0U;
	dap_ctx[0].transfer.idle_cycles = 0U;
	dap_ctx[0].transfer.retry_count = 100U;
	dap_ctx[0].transfer.match_retry = 0U;
	dap_ctx[0].transfer.match_mask = 0x00000000U;
// #ifdef CONFIG_DAP_SWD
// 	dap_ctx[0].swd_conf.turnaround = 1U;
// 	dap_ctx[0].swd_conf.data_phase = 0U;
// #endif
// #ifdef CONFIG_DAP_JTAG
// 	dap_ctx[0].jtag_dev.count = 0U;
// #endif

	return 0;
}
