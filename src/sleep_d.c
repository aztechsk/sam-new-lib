/*
 * sleep_d.c
 *
 * Copyright (c) 2021 Jan Rusnak <jan@rusnak.sk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <gentyp.h>
#include "sysconf.h"
#include "board.h"
#include <mmio.h>
#include "atom.h"
#include "msgconf.h"
#include "criterr.h"
#if EINTCTL == 1
#include "eic.h"
#endif
#if DMAC_ON_CHIP == 1
#include "dmac.h"
#endif
#include "sleep.h"

#if SLEEP_FEAT == 1

static void (*first[SLEEP_FIRST_ARY_SIZE])(enum sleep_cmd, ...);
static void (*second[SLEEP_SECOND_ARY_SIZE])(enum sleep_cmd, ...);
static void (*last[SLEEP_LAST_ARY_SIZE])(enum sleep_cmd, ...);
static TaskHandle_t tsk_hndl;
static enum sleep_mode sleep_mode;
static boolean_t idle_sleep;

static void tsk(void *p);

/**
 * reg_sleep_clbk
 */
void reg_sleep_clbk(void (*clbk)(enum sleep_cmd, ...), enum sleep_prio prio)
{
	void (**ary)(enum sleep_cmd, ...) = NULL;
	int sz = 0;

	switch (prio) {
	case SLEEP_PRIO_SUSP_FIRST  :
		ary = first;
		sz = SLEEP_FIRST_ARY_SIZE;
		break;
        case SLEEP_PRIO_SUSP_SECOND :
		ary = second;
		sz = SLEEP_SECOND_ARY_SIZE;
		break;
        case SLEEP_PRIO_SUSP_LAST   :
		ary = last;
		sz = SLEEP_LAST_ARY_SIZE;
		break;
	}
	taskENTER_CRITICAL();
	for (int i = 0; i < sz; i++) {
		if (!ary[i]) {
			ary[i] = clbk;
                        taskEXIT_CRITICAL();
			return;
		} else {
			if (ary[i] == clbk) {
				taskEXIT_CRITICAL();
				return;
			}
		}
	}
        taskEXIT_CRITICAL();
	crit_err_exit(UNEXP_PROG_STATE);
}

/**
 * init_sleep
 */
void init_sleep(void)
{
	if (pdPASS != xTaskCreate(tsk, "SLEEP", SLEEP_TASK_STACK_SIZE, NULL,
				  SLEEP_TASK_PRIO, &tsk_hndl)) {
		crit_err_exit(MALLOC_ERROR);
	}
}

/**
 * start_sleep
 */
void start_sleep(enum sleep_mode mode)
{
	sleep_mode = mode;
	barrier();
	vTaskResume(tsk_hndl);
}

/**
 * enable_idle_sleep
 */
void enable_idle_sleep(void)
{
	idle_sleep = TRUE;
}

/**
 * disable_idle_sleep
 */
void disable_idle_sleep(void)
{
	idle_sleep = FALSE;
}

/**
 * vApplicationIdleHook
 */
void vApplicationIdleHook(void)
{
	if (idle_sleep) {
		portDISABLE_INTERRUPTS();
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
		PM->SLEEP.reg = SLEEP_MODE_IDLE_0;
		__DSB();
		__WFI();
                portENABLE_INTERRUPTS();
	}
}

/**
 * tsk
 */
static void tsk(void *p)
{
	int max_f = -1, max_s = -1, max_l = -1;
#if DMAC_ON_CHIP == 1
	boolean_t dmac = FALSE;
#endif
#if EINTCTL == 1 && EINTCTL_DISABLE_IN_SLEEP == 1
	boolean_t eintctl = FALSE;
#endif
	for (;;) {
		vTaskSuspend(NULL);
		msg(INF, "sleep.c: init sleep (%d)\n", sleep_mode);
		for (int i = 0; i < SLEEP_FIRST_ARY_SIZE; i++) {
			if (first[i]) {
				max_f = i;
				first[i](SLEEP_CMD_SUSP, sleep_mode);
				continue;
			}
			break;
		}
		for (int i = 0; i < SLEEP_SECOND_ARY_SIZE; i++) {
			if (second[i]) {
				max_s = i;
				second[i](SLEEP_CMD_SUSP, sleep_mode);
				continue;
			}
			break;
		}
		for (int i = 0; i < SLEEP_LAST_ARY_SIZE; i++) {
			if (last[i]) {
				max_l = i;
				last[i](SLEEP_CMD_SUSP, sleep_mode);
				continue;
			}
			break;
		}
#if EINTCTL == 1 && EINTCTL_DISABLE_IN_SLEEP == 1
		if (is_eintctl_enabled()) {
			disable_eintctl();
			eintctl = TRUE;
		}
#endif
#if DMAC_ON_CHIP == 1
		if (is_dmac_enabled()) {
			disable_dmac();
			dmac = TRUE;
		}
#endif
                portDISABLE_INTERRUPTS();
                SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		if (sleep_mode == SLEEP_MODE_STANDBY) {
			SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		} else {
			SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
			PM->SLEEP.reg = sleep_mode;
		}
		__DSB();
		__WFI();
                SCB->ICSR |= SCB_ICSR_PENDSTCLR_Pos;
                SysTick->VAL = 0;
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
                portENABLE_INTERRUPTS();
#if DMAC_ON_CHIP == 1
		if (dmac) {
			enable_dmac();
		}
#endif
#if EINTCTL == 1 && EINTCTL_DISABLE_IN_SLEEP == 1
		if (eintctl) {
			enable_eintctl();
		}
#endif
		for (int i = max_l; i >= 0; i--) {
			last[i](SLEEP_CMD_WAKE);
		}
		for (int i = max_s; i >= 0; i--) {
			second[i](SLEEP_CMD_WAKE);
		}
		for (int i = max_f; i >= 0; i--) {
			first[i](SLEEP_CMD_WAKE);
		}
                msg(INF, "\n");
                msg(INF, "sleep.c: waked\n");
	}
}
#endif
