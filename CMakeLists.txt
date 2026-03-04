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
if(CONFIG_OMNI_CMSIS_DAP)
    zephyr_include_directories(components/cmsis-dap/include)
    # zephyr_include_directories(components/cmsis-dap/template/zephyr)

    zephyr_library_sources(
        components/cmsis-dap/src/DAP.c
        components/cmsis-dap/src/JTAG_DP.c
        components/cmsis-dap/src/SW_DP.c
        components/cmsis-dap/src/SWO.c
        components/cmsis-dap/src/UART.c
        components/cmsis-dap/src/DAP_vendor.c
    )
endif()
