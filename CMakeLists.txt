#
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 LuckkMaker
#

cmake_minimum_required(VERSION 3.20.0)

# omni
zephyr_library()
zephyr_include_directories(include)

add_subdirectory(subsys)
# add_subdirectory(drivers)
