/*
 * Copyright (c) 2026 LuckkMaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dap_usb, CONFIG_OMNI_DAP_LOG_LEVEL);

// Indicates whether the function is enabled and ready to transfer data
#define SAMPLE_FUNCTION_ENABLED 0

#define DAP_EP_OUT_ADDR   0x01
#define DAP_EP_OUT_FS_MPS 64U
#define DAP_EP_OUT_HS_MPS 512U
#define DAP_EP_IN_ADDR    0x81
#define DAP_EP_IN_FS_MPS  64U
#define DAP_EP_IN_HS_MPS  512U

// Descriptor for the CMSIS DAP USB function, including interface and endpoints
struct dap_usb_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	// Termination descriptor for the descriptor list
	struct usb_desc_header nil_desc;
};

struct dap_usb_private_data {
	struct dap_usb_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	struct usbd_desc_node *const iface_str_desc_nd;
	atomic_t state;
};

static int dap_usb_ep_request_callback(struct usbd_class_data *const c_data,
				       struct net_buf *const buf, int err)
{
	// TODO
	return 0;
}

static void dap_usb_class_enable_callback(struct usbd_class_data *const c_data)
{
	// TODO
}

static void dap_usb_class_disable_callback(struct usbd_class_data *const c_data)
{
	struct dap_usb_private_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, SAMPLE_FUNCTION_ENABLED);
	LOG_INF("Configuration disabled");
}

static int dap_usb_class_init_callback(struct usbd_class_data *c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_usb_private_data *data = usbd_class_get_private(c_data);
	struct dap_usb_desc *desc = data->desc;

	LOG_DBG("Init class instance %p", (void *)c_data);

	if (usbd_add_descriptor(uds_ctx, data->iface_str_desc_nd)) {
		LOG_ERR("Failed to add interface string descriptor");
	} else {
		desc->if0.iInterface = usbd_str_desc_get_idx(data->iface_str_desc_nd);
	}

	return 0;
}

static void *dap_usb_get_desc_callback(struct usbd_class_data *const c_data,
				       const enum usbd_speed speed)
{
	struct dap_usb_private_data *data = usbd_class_get_private(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

// TODO: (luckk) : Fix endpoint addresses

struct usbd_class_api dap_usb_api = {
	.request = dap_usb_ep_request_callback,
	.enable = dap_usb_class_enable_callback,
	.disable = dap_usb_class_disable_callback,
	.init = dap_usb_class_init_callback,
	.get_desc = dap_usb_get_desc_callback,
};

#define DAP_USB_DEFINE_DESCRIPTOR(n, _)                                                            \
	static struct dap_usb_desc dap_usb_desc_##n = {                                            \
		/* Interface descriptor 0 */                                                       \
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
                                                                                                   \
		/* Endpoint OUT */                                                                 \
		.if0_out_ep =                                                                      \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = DAP_EP_OUT_ADDR,                               \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(DAP_EP_OUT_FS_MPS),              \
				.bInterval = 0x00,                                                 \
			},                                                                         \
                                                                                                   \
		/* Endpoint IN */                                                                  \
		.if0_in_ep =                                                                       \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = DAP_EP_IN_ADDR,                                \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(DAP_EP_IN_FS_MPS),               \
				.bInterval = 0x00,                                                 \
			},                                                                         \
                                                                                                   \
		/* High-speed Endpoint OUT */                                                      \
		.if0_hs_out_ep =                                                                   \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = DAP_EP_OUT_ADDR,                               \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(DAP_EP_OUT_HS_MPS),              \
				.bInterval = 0x00,                                                 \
			},                                                                         \
                                                                                                   \
		/* High-speed Endpoint IN */                                                       \
		.if0_hs_in_ep =                                                                    \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = DAP_EP_IN_ADDR,                                \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(DAP_EP_IN_HS_MPS),               \
				.bInterval = 0x00,                                                 \
			},                                                                         \
                                                                                                   \
		/* Termination descriptor */                                                       \
		.nil_desc =                                                                        \
			{                                                                          \
				.bLength = 0,                                                      \
				.bDescriptorType = 0,                                              \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	const static struct usb_desc_header *dap_usb_fs_desc_##n[] = {                             \
		(struct usb_desc_header *)&dap_usb_desc_##n.if0,                                   \
		(struct usb_desc_header *)&dap_usb_desc_##n.if0_out_ep,                            \
		(struct usb_desc_header *)&dap_usb_desc_##n.if0_in_ep,                             \
		(struct usb_desc_header *)&dap_usb_desc_##n.nil_desc,                              \
	};                                                                                         \
                                                                                                   \
	const static struct usb_desc_header *dap_usb_hs_desc_##n[] = {                             \
		(struct usb_desc_header *)&dap_usb_desc_##n.if0,                                   \
		(struct usb_desc_header *)&dap_usb_desc_##n.if0_hs_out_ep,                         \
		(struct usb_desc_header *)&dap_usb_desc_##n.if0_hs_in_ep,                          \
		(struct usb_desc_header *)&dap_usb_desc_##n.nil_desc,                              \
	};

#define DAP_USB_DEFINE_FUNCTION_DATA(n, _)                                                         \
	USBD_DESC_STRING_DEFINE(iface_str_desc_nd_##n, "CMSIS-DAP v2", USBD_DUT_STRING_INTERFACE); \
                                                                                                   \
	static struct dap_usb_private_data dap_usb_data_##n = {                                    \
		.desc = &dap_usb_desc_##n,                                                         \
		.fs_desc = dap_usb_fs_desc_##n,                                                    \
		.hs_desc = dap_usb_hs_desc_##n,                                                    \
		.iface_str_desc_nd = &iface_str_desc_nd_##n,                                       \
	};                                                                                         \
                                                                                                   \
	USBD_DEFINE_CLASS(dap_usb_##n, &dap_usb_api, &dap_usb_data_##n, NULL);

LISTIFY(1, DAP_USB_DEFINE_DESCRIPTOR, ())
LISTIFY(1, DAP_USB_DEFINE_FUNCTION_DATA, ())
