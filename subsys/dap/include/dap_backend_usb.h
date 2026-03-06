/*
 * Copyright (c) 2026 LuckkMaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DAP_BACKEND_USB_H_
#define ZEPHYR_INCLUDE_DAP_BACKEND_USB_H_

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbd.h>

#include "cmsis_dap.h"

#define DAP_QUEUE_PACKET_SIZE_MAX 512U
#define DAP_QUEUE_PACKET_COUNT    CONFIG_DAP_PACKET_COUNT

extern struct usbd_class_api dap_queue_api;

struct dap_queue_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_desc_header nil_desc;
};

struct dap_queue_data {
	struct dap_queue_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	struct usbd_desc_node *const iface_str_desc_nd;
	atomic_t state;

	struct dap_context *dap_ctx;

	/* Request queue (OUT): produced by USB callback, consumed by work */
	uint8_t request_buf[DAP_QUEUE_PACKET_COUNT][DAP_QUEUE_PACKET_SIZE_MAX];
	uint32_t request_len[DAP_QUEUE_PACKET_COUNT];
	uint16_t request_index_i; /* producer (OUT complete) */
	uint16_t request_index_o; /* consumer (work) */
	uint16_t request_count;

	/* Response queue (IN): produced by work, consumed by USB IN complete */
	uint8_t response_buf[DAP_QUEUE_PACKET_COUNT][DAP_QUEUE_PACKET_SIZE_MAX];
	uint16_t response_len[DAP_QUEUE_PACKET_COUNT];
	uint16_t response_index_i; /* producer (work) */
	uint16_t response_index_o; /* consumer (IN complete) */
	uint16_t response_count;

	struct k_mutex queue_mutex;
	struct k_work process_work;
	struct usbd_class_data *class_data; /* for work to enqueue IN */
};

/**
 * @brief Define one CMSIS-DAP USB queue backend instance and bind it to a DAP context.
 *
 * This macro should be used by the application in the same translation unit where
 * `DAP_DEVICE_DEFINE()` is used, for example:
 *
 * @code{.c}
 * DAP_DEVICE_DEFINE(sample_dap, swd_dev, NULL);
 * DAP_BACKEND_USB_DEFINE(sample_dap_usb, &sample_dap);
 * @endcode
 *
 * @param inst_name USB class instance name
 * @param _dap_ctx Pointer to the bound `struct dap_context`
 */
#define DAP_BACKEND_USB_DEFINE(inst_name, _dap_ctx)                                         \
	static struct dap_queue_desc inst_name##_desc = {                                       \
		.if0 = {                                                                          \
			.bLength = sizeof(struct usb_if_descriptor),                                \
			.bDescriptorType = USB_DESC_INTERFACE,                                      \
			.bInterfaceNumber = 0,                                                      \
			.bAlternateSetting = 0,                                                     \
			.bNumEndpoints = 2,                                                         \
			.bInterfaceClass = USB_BCC_VENDOR,                                          \
			.bInterfaceSubClass = 0,                                                    \
			.bInterfaceProtocol = 0,                                                    \
			.iInterface = 0,                                                            \
		},                                                                                 \
		.if0_out_ep = {                                                                   \
			.bLength = sizeof(struct usb_ep_descriptor),                                \
			.bDescriptorType = USB_DESC_ENDPOINT,                                       \
			.bEndpointAddress = 0x01,                                                   \
			.bmAttributes = USB_EP_TYPE_BULK,                                           \
			.wMaxPacketSize = sys_cpu_to_le16(64U),                                     \
			.bInterval = 0x00,                                                          \
		},                                                                                 \
		.if0_in_ep = {                                                                    \
			.bLength = sizeof(struct usb_ep_descriptor),                                \
			.bDescriptorType = USB_DESC_ENDPOINT,                                       \
			.bEndpointAddress = 0x81,                                                   \
			.bmAttributes = USB_EP_TYPE_BULK,                                           \
			.wMaxPacketSize = sys_cpu_to_le16(64U),                                     \
			.bInterval = 0x00,                                                          \
		},                                                                                 \
		.if0_hs_out_ep = {                                                                \
			.bLength = sizeof(struct usb_ep_descriptor),                                \
			.bDescriptorType = USB_DESC_ENDPOINT,                                       \
			.bEndpointAddress = 0x01,                                                   \
			.bmAttributes = USB_EP_TYPE_BULK,                                           \
			.wMaxPacketSize = sys_cpu_to_le16(512),                                     \
			.bInterval = 0x00,                                                          \
		},                                                                                 \
		.if0_hs_in_ep = {                                                                 \
			.bLength = sizeof(struct usb_ep_descriptor),                                \
			.bDescriptorType = USB_DESC_ENDPOINT,                                       \
			.bEndpointAddress = 0x81,                                                   \
			.bmAttributes = USB_EP_TYPE_BULK,                                           \
			.wMaxPacketSize = sys_cpu_to_le16(512),                                     \
			.bInterval = 0x00,                                                          \
		},                                                                                 \
		.nil_desc = {.bLength = 0, .bDescriptorType = 0},                               \
	};                                                                                    \
	static const struct usb_desc_header *inst_name##_fs_desc[] = {                       \
		(struct usb_desc_header *)&inst_name##_desc.if0,                              \
		(struct usb_desc_header *)&inst_name##_desc.if0_out_ep,                       \
		(struct usb_desc_header *)&inst_name##_desc.if0_in_ep,                        \
		(struct usb_desc_header *)&inst_name##_desc.nil_desc,                         \
	};                                                                                    \
	static const struct usb_desc_header *inst_name##_hs_desc[] = {                       \
		(struct usb_desc_header *)&inst_name##_desc.if0,                              \
		(struct usb_desc_header *)&inst_name##_desc.if0_hs_out_ep,                    \
		(struct usb_desc_header *)&inst_name##_desc.if0_hs_in_ep,                     \
		(struct usb_desc_header *)&inst_name##_desc.nil_desc,                         \
	};                                                                                    \
	USBD_DESC_STRING_DEFINE(inst_name##_iface_str_desc, "CMSIS-DAP v2",                \
				USBD_DUT_STRING_INTERFACE);                                 \
	static struct dap_queue_data inst_name##_data = {                                    \
		.desc = &inst_name##_desc,                                                      \
		.fs_desc = inst_name##_fs_desc,                                                 \
		.hs_desc = inst_name##_hs_desc,                                                 \
		.iface_str_desc_nd = &inst_name##_iface_str_desc,                               \
		.dap_ctx = (_dap_ctx),                                                       \
	};                                                                                    \
	USBD_DEFINE_CLASS(inst_name, &dap_queue_api, &inst_name##_data, NULL)

#endif /* ZEPHYR_INCLUDE_DAP_BACKEND_USB_H_ */
