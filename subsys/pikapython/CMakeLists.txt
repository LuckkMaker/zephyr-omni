#
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 LuckkMaker
#

if(WIN32)
	set(PIKA_MSC "rust-msc-latest-win10.exe")
else()
	set(PIKA_MSC "rust-msc-latest-linux")
endif()
if(EXISTS "${PIKA_MSC}")
	add_custom_target(
		pika_precompile ALL
		COMMAND ${CMAKE_COMMAND} -E chdir "${PIKA_ROOT}" "${PIKA_MSC}"
		COMMENT "Pika precompiler: rust-msc (main.py -> pikascript-api)"
		VERBATIM
	)
	add_dependencies(app pika_precompile)
else()
	message(STATUS "Pika precompiler not found (${PIKA_MSC}), skip; run manually before build if main.py changed.")
endif()

zephyr_include_directories(
	pikascript-api
	pikascript-core
	pikascript-lib/PikaStdLib
)

zephyr_include_directories_ifdef(CONFIG_PIKAPYTHON_TIME pikascript-lib/time)
zephyr_include_directories_ifdef(CONFIG_PIKAPYTHON_TEST pikascript-lib/Math)

file(GLOB_RECURSE PIKA_SCRIPT_SOURCES
	pikascript-api/*.c
	pikascript-core/*.c
	pikascript-lib/PikaStdLib/*.c
)
file(GLOB PIKA_TIME_SOURCES "pikascript-lib/time/*.c")
file(GLOB PIKA_MATH_SOURCES "pikascript-lib/Math/*.c")

zephyr_library()
zephyr_library_sources(
	${PIKA_SCRIPT_SOURCES}
	zephyr/pika_config.c
	zephyr/pikaZephyr_Thread.c
)

zephyr_library_sources_ifdef(CONFIG_PIKAPYTHON_TIME ${PIKA_TIME_SOURCES})
zephyr_library_sources_ifdef(CONFIG_PIKAPYTHON_TEST ${PIKA_MATH_SOURCES})
