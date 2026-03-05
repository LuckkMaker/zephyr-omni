#
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 LuckkMaker
#

zephyr_include_directories(include)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SWDP_BITBANG_DRIVER src/swdp_bitbang.c)
zephyr_library_sources_ifdef(CONFIG_JTAGDP_BITBANG_DRIVER src/jtagdp_bitbang.c)
