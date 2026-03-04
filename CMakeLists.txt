#
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 LuckkMaker
#

cmake_minimum_required(VERSION 3.20.0)

# omni
zephyr_library()
zephyr_include_directories(include)

# components
# cmsis-dap
if(CONFIG_OMNI_DAP)
    zephyr_include_directories(components/dap/include)
    # zephyr_include_directories(components/cmsis-dap/template/zephyr)

    zephyr_library_sources(
        components/dap/src/dap_backend_usb.c
    )
endif()
