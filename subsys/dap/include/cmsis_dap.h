/*
 * Copyright (c) 2026 LuckkMaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CMSIS_DAP_H_
#define ZEPHYR_INCLUDE_CMSIS_DAP_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util_macro.h>

/* DAP Firmware Version */
#define DAP_FW_VER "2.1.2"

/* DAP Status Code */
#define DAP_OK    0U
#define DAP_ERROR 0xFFU

/* DAP Command IDs */
#define ID_DAP_INFO                0x00U
#define ID_DAP_HOST_STATUS         0x01U
#define ID_DAP_CONNECT             0x02U
#define ID_DAP_DISCONNECT          0x03U
#define ID_DAP_TRANSFER_CONFIGURE  0x04U
#define ID_DAP_TRANSFER            0x05U
#define ID_DAP_TRANSFER_BLOCK      0x06U
#define ID_DAP_TRANSFER_ABORT      0x07U
#define ID_DAP_WRITE_ABORT         0x08U
#define ID_DAP_DELAY               0x09U
#define ID_DAP_RESET_TARGET        0x0AU
#define ID_DAP_SWJ_PINS            0x10U
#define ID_DAP_SWJ_CLOCK           0x11U
#define ID_DAP_SWJ_SEQUENCE        0x12U
#define ID_DAP_SWD_CONFIGURE       0x13U
#define ID_DAP_SWD_SEQUENCE        0x1DU
#define ID_DAP_JTAG_SEQUENCE       0x14U
#define ID_DAP_JTAG_CONFIGURE      0x15U
#define ID_DAP_JTAG_IDCODE         0x16U
#define ID_DAP_SWO_TRANSPORT       0x17U
#define ID_DAP_SWO_MODE            0x18U
#define ID_DAP_SWO_BAUDRATE        0x19U
#define ID_DAP_SWO_CONTROL         0x1AU
#define ID_DAP_SWO_STATUS          0x1BU
#define ID_DAP_SWO_EXTENDED_STATUS 0x1EU
#define ID_DAP_SWO_DATA            0x1CU
#define ID_DAP_UART_TRANSPORT      0x1FU
#define ID_DAP_UART_CONFIGURE      0x20U
#define ID_DAP_UART_CONTROL        0x22U
#define ID_DAP_UART_STATUS         0x23U
#define ID_DAP_UART_TRANSFER       0x21U

#define ID_DAP_QUEUE_COMMANDS   0x7EU
#define ID_DAP_EXECUTE_COMMANDS 0x7FU

/* DAP Vendor Command IDs */
#define ID_DAP_VENDOR0  0x80U
#define ID_DAP_VENDOR1  0x81U
#define ID_DAP_VENDOR2  0x82U
#define ID_DAP_VENDOR3  0x83U
#define ID_DAP_VENDOR4  0x84U
#define ID_DAP_VENDOR5  0x85U
#define ID_DAP_VENDOR6  0x86U
#define ID_DAP_VENDOR7  0x87U
#define ID_DAP_VENDOR8  0x88U
#define ID_DAP_VENDOR9  0x89U
#define ID_DAP_VENDOR10 0x8AU
#define ID_DAP_VENDOR11 0x8BU
#define ID_DAP_VENDOR12 0x8CU
#define ID_DAP_VENDOR13 0x8DU
#define ID_DAP_VENDOR14 0x8EU
#define ID_DAP_VENDOR15 0x8FU
#define ID_DAP_VENDOR16 0x90U
#define ID_DAP_VENDOR17 0x91U
#define ID_DAP_VENDOR18 0x92U
#define ID_DAP_VENDOR19 0x93U
#define ID_DAP_VENDOR20 0x94U
#define ID_DAP_VENDOR21 0x95U
#define ID_DAP_VENDOR22 0x96U
#define ID_DAP_VENDOR23 0x97U
#define ID_DAP_VENDOR24 0x98U
#define ID_DAP_VENDOR25 0x99U
#define ID_DAP_VENDOR26 0x9AU
#define ID_DAP_VENDOR27 0x9BU
#define ID_DAP_VENDOR28 0x9CU
#define ID_DAP_VENDOR29 0x9DU
#define ID_DAP_VENDOR30 0x9EU
#define ID_DAP_VENDOR31 0x9FU

#define ID_DAP_INVALID 0xFFU

/* DAP ID */
#define DAP_ID_VENDOR              1U
#define DAP_ID_PRODUCT             2U
#define DAP_ID_SER_NUM             3U
#define DAP_ID_DAP_FW_VER          4U
#define DAP_ID_DEVICE_VENDOR       5U
#define DAP_ID_DEVICE_NAME         6U
#define DAP_ID_BOARD_VENDOR        7U
#define DAP_ID_BOARD_NAME          8U
#define DAP_ID_PRODUCT_FW_VER      9U
#define DAP_ID_CAPABILITIES        0xF0U
#define DAP_ID_TIMESTAMP_CLOCK     0xF1U
#define DAP_ID_UART_RX_BUFFER_SIZE 0xFBU
#define DAP_ID_UART_TX_BUFFER_SIZE 0xFCU
#define DAP_ID_SWO_BUFFER_SIZE     0xFDU
#define DAP_ID_PACKET_COUNT        0xFEU
#define DAP_ID_PACKET_SIZE         0xFFU

/* DAP Capabilities */
#define DAP_CAP_SWD               (1U << 0)
#define DAP_CAP_JTAG              (1U << 1)
#define DAP_CAP_SWO_UART          (1U << 2)
#define DAP_CAP_SWO_MANCHESTER    (1U << 3)
#define DAP_CAP_ATOMIC_COMMANDS   (1U << 4)
#define DAP_CAP_TIMESTAMP_CLOCK   (1U << 5)
#define DAP_CAP_SWO_STREAM        (1U << 6)
#define DAP_CAP_UART              (1U << 7)
#define DAP_CAP_UART_USB_COM_PORT (1U << 8)

/* DAP Host Status */
#define DAP_DEBUGGER_CONNECTED 0U
#define DAP_TARGET_RUNNING     1U

/* DAP Port */
#define DAP_PORT_AUTODETECT 0U // Autodetect Port
#define DAP_PORT_DISABLED   0U // Port Disabled (I/O pins in High-Z)
#define DAP_PORT_SWD        1U // SWD Port (SWCLK, SWDIO) + nRESET
#define DAP_PORT_JTAG       2U // JTAG Port (TCK, TMS, TDI, TDO, nTRST) + nRESET

/* DAP SWJ Pins */
#define DAP_SWJ_SWCLK_TCK 0 // SWCLK/TCK
#define DAP_SWJ_SWDIO_TMS 1 // SWDIO/TMS
#define DAP_SWJ_TDI       2 // TDI
#define DAP_SWJ_TDO       3 // TDO
#define DAP_SWJ_nTRST     5 // nTRST
#define DAP_SWJ_nRESET    7 // nRESET

/* DAP Transfer Request */
#define DAP_TRANSFER_APnDP       (1U << 0)
#define DAP_TRANSFER_RnW         (1U << 1)
#define DAP_TRANSFER_A2          (1U << 2)
#define DAP_TRANSFER_A3          (1U << 3)
#define DAP_TRANSFER_MATCH_VALUE (1U << 4)
#define DAP_TRANSFER_MATCH_MASK  (1U << 5)
#define DAP_TRANSFER_TIMESTAMP   (1U << 7)

/* DAP Transfer Response */
#define DAP_TRANSFER_OK       (1U << 0)
#define DAP_TRANSFER_WAIT     (1U << 1)
#define DAP_TRANSFER_FAULT    (1U << 2)
#define DAP_TRANSFER_ERROR    (1U << 3)
#define DAP_TRANSFER_MISMATCH (1U << 4)

/* DAP SWO Trace Mode */
#define DAP_SWO_OFF        0U
#define DAP_SWO_UART       1U
#define DAP_SWO_MANCHESTER 2U

/* DAP SWO Trace Status */
#define DAP_SWO_CAPTURE_ACTIVE (1U << 0)
#define DAP_SWO_CAPTURE_PAUSED (1U << 1)
#define DAP_SWO_STREAM_ERROR   (1U << 6)
#define DAP_SWO_BUFFER_OVERRUN (1U << 7)

/* DAP UART Transport */
#define DAP_UART_TRANSPORT_NONE         0U
#define DAP_UART_TRANSPORT_USB_COM_PORT 1U
#define DAP_UART_TRANSPORT_DAP_COMMAND  2U

/* DAP UART Control */
#define DAP_UART_CONTROL_RX_ENABLE    (1U << 0)
#define DAP_UART_CONTROL_RX_DISABLE   (1U << 1)
#define DAP_UART_CONTROL_RX_BUF_FLUSH (1U << 2)
#define DAP_UART_CONTROL_TX_ENABLE    (1U << 4)
#define DAP_UART_CONTROL_TX_DISABLE   (1U << 5)
#define DAP_UART_CONTROL_TX_BUF_FLUSH (1U << 6)

/* DAP UART Status */
#define DAP_UART_STATUS_RX_ENABLED    (1U << 0)
#define DAP_UART_STATUS_RX_DATA_LOST  (1U << 1)
#define DAP_UART_STATUS_FRAMING_ERROR (1U << 2)
#define DAP_UART_STATUS_PARITY_ERROR  (1U << 3)
#define DAP_UART_STATUS_TX_ENABLED    (1U << 4)

/* DAP UART Configure Error */
#define DAP_UART_CFG_ERROR_DATA_BITS (1U << 0)
#define DAP_UART_CFG_ERROR_PARITY    (1U << 1)
#define DAP_UART_CFG_ERROR_STOP_BITS (1U << 2)

/* Debug Port Register Addresses */
#define DP_IDCODE    0x00U // IDCODE Register (SW Read only)
#define DP_ABORT     0x00U // Abort Register (SW Write only)
#define DP_CTRL_STAT 0x04U // Control & Status
#define DP_WCR       0x04U // Wire Control Register (SW Only)
#define DP_SELECT    0x08U // Select Register (JTAG R/W & SW W)
#define DP_RESEND    0x08U // Resend (SW Read Only)
#define DP_RDBUFF    0x0CU // Read Buffer (Read Only)

/* JTAG IR Codes */
#define JTAG_ABORT  0x08U
#define JTAG_DPACC  0x0AU
#define JTAG_APACC  0x0BU
#define JTAG_IDCODE 0x0EU
#define JTAG_BYPASS 0x0FU

/* JTAG Sequence Info */
#define JTAG_SEQUENCE_TCK 0x3FU // TCK count
#define JTAG_SEQUENCE_TMS 0x40U // TMS value
#define JTAG_SEQUENCE_TDO 0x80U // TDO capture

/* SWD Sequence Info */
#define SWD_SEQUENCE_CLK 0x3FU // SWCLK count
#define SWD_SEQUENCE_DIN 0x80U // SWDIO capture

/**
 * @brief DAP supported message types
 */
enum dap_msg_type {
	/** Debugger connected message */
	DAP_MSG_CONNECTED,
	/** Debugger running message */
	DAP_MSG_RUNNING,
};

struct dap_context;

/**
 * @brief Callback type definition for DAP vendor command handler
 *
 * @param ctx Pointer to DAP context
 * @param request Pointer to the request data
 * @param response Pointer to the response data
 * @return Number of bytes in response (lower 16 bits) and number of bytes in request (upper 16
 * bits)
 */
typedef uint32_t (*dap_vendor_cmd_cb_t)(struct dap_context *const ctx, const uint8_t *request,
					uint8_t *response);

/**
 * @brief Callback type definition for DAP serial number string provider
 *
 * @param ctx Pointer to DAP context
 * @return Pointer to a null-terminated string containing the serial number
 */
typedef uint8_t *(*dap_serial_number_str_cb_t)(struct dap_context *const ctx);

typedef void (*dap_msg_cb_t)(struct dap_context *const ctx, enum dap_msg_type msg);

struct dap_context {
	/** Name of the DAP device */
	const char *name;
	/** Pointer to SWDP device */
	const struct device *swdp_dev;
	/** Pointer to JTAGDP device */
	const struct device *jtagdp_dev;
	/** Vendor command handler callback */
	dap_vendor_cmd_cb_t vendor_cmd_cb;
	/** Serial number string callback */
	dap_serial_number_str_cb_t ser_num_str_cb;
	/** DAP message callback */
	dap_msg_cb_t msg_cb;
	atomic_t state;
	uint8_t debug_port;          // Debug Port
	uint16_t capabilities;       // Capabilities
	uint16_t pkt_size;           // Packet Size
	uint8_t fast_clock;          // Fast Clock Flag
	uint32_t clock_delay;        // Clock Delay
	uint32_t timestamp;          // Last captured Timestamp
	struct {                     // Transfer Configuration
		uint8_t idle_cycles; // Idle cycles after transfer
		uint8_t padding[3];
		uint16_t retry_count; // Number of retries after WAIT response
		uint16_t match_retry; // Number of retries if read value does not match
		uint32_t match_mask;  // Match Mask
	} transfer;
#ifdef CONFIG_DAP_SWD
	struct {                    // SWD Configuration
		uint8_t turnaround; // Turnaround period
		uint8_t data_phase; // Always generate Data Phase
	} swd_conf;
#endif
#ifdef CONFIG_DAP_JTAG
	struct {               // JTAG Device Chain
		uint8_t count; // Number of devices
		uint8_t index; // Device index (device at TDO has index 0)
#ifdef CONFIG_DAP_JTAG_DEV_CNT
		uint8_t ir_length[CONFIG_DAP_JTAG_DEV_CNT];
		uint16_t ir_before[CONFIG_DAP_JTAG_DEV_CNT];
		uint16_t ir_after[CONFIG_DAP_JTAG_DEV_CNT];
#endif
	} jtag_dev;
#endif
};

/**
 * @brief Define a DAP device context structure
 *
 * Example of use:
 *
 * @code{.c}
 * DAP_DEFINE(sample_dap,
 *        DEVICE_DT_GET(DT_NODELABEL(swdp)),
 *        DEVICE_DT_GET(DT_NODELABEL(jtagdp)));
 * @endcode
 *
 * @param device_name DAP context name
 * @param swdp_dev	  Pointer to SWDP device structure
 * @param jtagdp_dev  Pointer to JTAGDP device structure
 */
#define DAP_DEVICE_DEFINE(device_name, _swdp_dev, _jtagdp_dev)                                     \
	static STRUCT_SECTION_ITERABLE(dap_context, device_name) = {                               \
		.name = STRINGIFY(device_name),                                                    \
				  IF_ENABLED(CONFIG_DAP_SWD, (.swdp_dev = _swdp_dev,)) IF_ENABLED(CONFIG_DAP_JTAG, (.jtagdp_dev = _jtagdp_dev,)) }

int dap_vendor_cmd_register_cb(struct dap_context *const ctx, const dap_vendor_cmd_cb_t cb);
int dap_ser_num_str_register_cb(struct dap_context *const ctx, const dap_serial_number_str_cb_t cb);
int dap_msg_register_cb(struct dap_context *const ctx, const dap_msg_cb_t cb);
int dap_setup(struct dap_context *const ctx);
void dap_update_pkt_size(struct dap_context *const ctx, const uint16_t pkt_size);
uint32_t dap_execute_command(struct dap_context *const ctx, const uint8_t *request,
			     uint8_t *response);

#endif /* ZEPHYR_INCLUDE_CMSIS_DAP_H_ */
