#
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 LuckkMaker
#

zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(src/cmsis_dap.c)

zephyr_library_sources_ifdef(CONFIG_DAP_BACKEND_USB_QUEUE src/dap_backend_usb.c)