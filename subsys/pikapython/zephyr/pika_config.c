/*
 * Copyright (c) 2026 LuckkMaker
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PikaPlatform.h"
#include "dataArg.h"

/* _time.c 使用 snake_case */
int64_t pika_platform_get_tick(void)
{
	return (int64_t)k_uptime_get_32();
}

/* _time.c 在 sleep 前后调用；单线程 VM 下可为空 */
void pika_GIL_ENTER(void) {}
void pika_GIL_EXIT(void) {}

/* _time.c 里写成 arg_newNone，与 dataArg.h 中 arg_newNull 等价 */
Arg *arg_newNone(void)
{
	return arg_setNull(NULL);
}

/* time 模块 _time.c 会调用；pika_platform_getTick 非 -1 时走 _do_sleep_ms_tick -> pika_sleep_ms */
int64_t pika_platform_getTick(void)
{
	return pika_platform_get_tick();
}

void pika_platform_sleep_ms(uint32_t ms)
{
	k_msleep((int32_t)ms);
}

void pika_platform_sleep_s(uint32_t s)
{
	k_msleep((int32_t)(s * 1000U));
}

void pika_sleep_ms(uint32_t ms)
{
	k_msleep((int32_t)ms);
}

static unsigned int _irq_lock_key;

void __platform_disable_irq_handle(void)
{
	_irq_lock_key = irq_lock();
}

void __platform_enable_irq_handle(void)
{
	irq_unlock(_irq_lock_key);
}

int __platform_sprintf(char *buff, char *fmt, ...)
{
	va_list args;
	int n;

	va_start(args, fmt);
	n = vsprintf(buff, fmt, args);
	va_end(args);
	return n;
}

int __platform_vsprintf(char *buff, char *fmt, va_list args)
{
	return vsprintf(buff, fmt, args);
}

int __platform_vsnprintf(char *buff, size_t size, const char *fmt, va_list args)
{
	return vsnprintf(buff, size, fmt, args);
}

void *__platform_malloc(size_t size)
{
#if defined(CONFIG_HEAP_MEM_POOL_SIZE) && (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	return k_malloc(size);
#else
	return malloc(size);
#endif
}

void __platform_free(void *ptr)
{
#if defined(CONFIG_HEAP_MEM_POOL_SIZE) && (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	k_free(ptr);
#else
	free(ptr);
#endif
}

void *__platform_memset(void *mem, int ch, size_t size)
{
	return memset(mem, ch, size);
}

void *__platform_memcpy(void *dir, const void *src, size_t size)
{
	return memcpy(dir, src, size);
}
