/*
 * Copyright (c) 2025, PikaPython Zephyr Port
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * pikaZephyr Thread/Task: mdelay and platformGetTick.
 */
#include <zephyr/kernel.h>
#include <pikaScript.h>

void pikaZephyr_Thread_mdelay(PikaObj *self, int ms)
{
	(void)self;
	k_msleep((int32_t)ms);
}

void pikaZephyr_Task_platformGetTick(PikaObj *self)
{
	obj_setInt(self, "tick", (int64_t)k_uptime_get_32());
}

