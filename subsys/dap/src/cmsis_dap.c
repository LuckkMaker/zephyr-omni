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

struct dap_context {
#ifdef CONFIG_DAP_SWD
	struct device *swdp_dev;
#endif
#ifdef CONFIG_DAP_JTAG
	struct device *jtagdp_dev;
#endif
	atomic_t state;
	uint8_t debug_port;								// Debug Port
	uint8_t fast_clock;								// Fast Clock Flag
	uint8_t padding[2];
	uint32_t clock_delay;							// Clock Delay
	uint32_t timestamp;								// Last captured Timestamp
	struct {										// Transfer Configuration
		uint8_t idle_cycles;						// Idle cycles after transfer
		uint8_t padding[3];
		uint16_t retry_count;						// Number of retries after WAIT response
		uint16_t match_retry;						// Number of retries if read value does not match
		uint32_t match_mask;						// Match Mask
	} transfer;
#ifdef CONFIG_DAP_SWD
	struct {										// SWD Configuration
		uint8_t turnaround;							// Turnaround period
		uint8_t data_phase;							// Always generate Data Phase
	} swd_conf;
#endif
#ifdef CONFIG_DAP_JTAG
	struct {										// JTAG Device Chain
		uint8_t count;								// Number of devices
		uint8_t index;								// Device index (device at TDO has index 0)
#ifdef CONFIG_DAP_JTAG_DEV_CNT
		uint8_t ir_length[CONFIG_DAP_JTAG_DEV_CNT];
		uint16_t ir_before[CONFIG_DAP_JTAG_DEV_CNT];
		uint16_t ir_after[CONFIG_DAP_JTAG_DEV_CNT];
#endif
	} jtag_dev;
#endif
};

static struct dap_context dap_ctx[1];

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
#ifdef CONFIG_DAP_SWD
	dap_ctx[0].swd_conf.turnaround = 1U;
	dap_ctx[0].swd_conf.data_phase = 0U;
#endif
#ifdef CONFIG_DAP_JTAG
	dap_ctx[0].jtag_dev.count = 0U;
#endif

	return 0;
}
