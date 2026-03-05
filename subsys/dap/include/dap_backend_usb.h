/*
 * Copyright (c) 2026 LuckkMaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DAP_BACKEND_USB_H_
#define ZEPHYR_INCLUDE_DAP_BACKEND_USB_H_

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

#define DAP_QUEUE_DESCRIPTOR_DEFINE(n, _)                                                          \
	static struct dap_queue_desc dap_queue_desc_##n = {                                        \
		.if0 =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 0,                                             \
				.bAlternateSetting = 0,                                            \
				.bNumEndpoints = 2,                                                \
				.bInterfaceClass = USB_BCC_VENDOR,                                 \
				.bInterfaceSubClass = 0,                                           \
				.bInterfaceProtocol = 0,                                           \
				.iInterface = 0,                                                   \
			},                                                                         \
		.if0_out_ep =                                                                      \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x01,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
				.bInterval = 0x00,                                                 \
			},                                                                         \
		.if0_in_ep =                                                                       \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x81,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
				.bInterval = 0x00,                                                 \
			},                                                                         \
		.if0_hs_out_ep =                                                                   \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x01,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512),                            \
				.bInterval = 0x00,                                                 \
			},                                                                         \
		.if0_hs_in_ep =                                                                    \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x81,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512),                            \
				.bInterval = 0x00,                                                 \
			},                                                                         \
		.nil_desc = {.bLength = 0, .bDescriptorType = 0},                                  \
	};                                                                                         \
	const static struct usb_desc_header *dap_queue_fs_desc_##n[] = {                           \
		(struct usb_desc_header *)&dap_queue_desc_##n.if0,                                 \
		(struct usb_desc_header *)&dap_queue_desc_##n.if0_out_ep,                          \
		(struct usb_desc_header *)&dap_queue_desc_##n.if0_in_ep,                           \
		(struct usb_desc_header *)&dap_queue_desc_##n.nil_desc,                            \
	};                                                                                         \
	const static struct usb_desc_header *dap_queue_hs_desc_##n[] = {                           \
		(struct usb_desc_header *)&dap_queue_desc_##n.if0,                                 \
		(struct usb_desc_header *)&dap_queue_desc_##n.if0_hs_out_ep,                       \
		(struct usb_desc_header *)&dap_queue_desc_##n.if0_hs_in_ep,                        \
		(struct usb_desc_header *)&dap_queue_desc_##n.nil_desc,                            \
	};

#define DAP_QUEUE_FUNCTION_DATA_DEFINE(n, _)                                                       \
	USBD_DESC_STRING_DEFINE(iface_str_desc_nd_queue_##n, "CMSIS-DAP v2",                       \
				USBD_DUT_STRING_INTERFACE);                                        \
	static struct dap_queue_data dap_queue_data_##n = {                                        \
		.desc = &dap_queue_desc_##n,                                                       \
		.fs_desc = dap_queue_fs_desc_##n,                                                  \
		.hs_desc = dap_queue_hs_desc_##n,                                                  \
		.iface_str_desc_nd = &iface_str_desc_nd_queue_##n,                                 \
	};                                                                                         \
	USBD_DEFINE_CLASS(dap_queue_##n, &dap_queue_api, &dap_queue_data_##n, NULL);

// /* 
//  * 供应用层使用的 USB Class 定义宏 
//  * 参数 inst:     USB 实例名称 (例如 dap_usb_0)
//  * 参数 dap_ctx:  绑定的 DAP 上下文指针 (例如 &sample_dap)
//  */
// #define USBD_DAP_DEFINE(inst, _dap_ctx)                                            \
//     /* USB 描述符实例构建 (抽取之前的 DAP_QUEUE_DESCRIPTOR_DEFINE 逻辑) */       \
//     static struct dap_queue_desc dap_queue_desc_##inst = {                     \
//         /* ...在此处填入之前的接口和端点描述符定义... */                            \
//     };                                                                         \
//     const static struct usb_desc_header *dap_queue_fs_desc_##inst[] = {        \
//         /* ... */                                                              \
//     };                                                                         \
//     USBD_DESC_STRING_DEFINE(iface_str_desc_nd_##inst, "CMSIS-DAP v2",          \
//                 USBD_DUT_STRING_INTERFACE);                            \
//                                                                                \
//     /* 定义 dap_queue_data，并绑定 dap_ctx */                                   \
//     static struct dap_queue_data dap_queue_data_##inst = {                     \
//         .desc = &dap_queue_desc_##inst,                                        \
//         .fs_desc = dap_queue_fs_desc_##inst,                                   \
//         /* 其他字段... */                                                       \
//         .dap_ctx = (_dap_ctx),                                                 \
//     };                                                                         \
//                                                                                \
//     /* 最终调用 Zephyr 的 USBD 实例化宏 */                                       \
//     USBD_DEFINE_CLASS(inst, &dap_queue_api, &dap_queue_data_##inst, NULL)

LISTIFY(1, DAP_QUEUE_DESCRIPTOR_DEFINE, ())
LISTIFY(1, DAP_QUEUE_FUNCTION_DATA_DEFINE, ())

#endif /* ZEPHYR_INCLUDE_DAP_BACKEND_USB_H_ */
