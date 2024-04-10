/*
 * criterr.c
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
#include "wdt.h"
#include "port.h"
#include "tc.h"
#include "criterr.h"

#if CRITERR == 1

#define CRITERR_INTR_FREQ 20

static volatile int intr_cnt;
static volatile boolean_t led_sig_en;
static struct tc_timer_dsc tc;
#if TERMOUT == 1
static const char err0[] = "UNEXP_PROG_STATE";
static const char err1[] = "TASK_STACK_OVERFLOW";
static const char err2[] = "MALLOC_ERROR";
static const char err3[] = "BAD_PARAMETER";
static const char err4[] = "APPLICATION_ERROR_1";
static const char err5[] = "APPLICATION_ERROR_2";
static const char err6[] = "APPLICATION_ERROR_3";
static const char err7[] = "HARDWARE_ERROR";
static const char *const errors[] = {err0, err1, err2, err3, err4, err5, err6, err7};
#endif

#if TERMOUT == 1
static char *fname(char *file);
#endif
static void crit_err(enum crit_err err);
static void show_err(enum crit_err err);
static void init_tc_50ms(void);
static BaseType_t isr_clbk(void *dev);

/**
 * crit_err_exit_fn
 */
void crit_err_exit_fn(enum crit_err err, char *file, int line)
{
#if TERMOUT == 1
	char *p;
#endif
	if (err == TASK_STACK_OVERFLOW) {
		if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
			vTaskPrioritySet(NULL, TASK_PRIO_HIGH);
		}
		crit_err(TASK_STACK_OVERFLOW);
	}
	switch (xTaskGetSchedulerState()) {
	case taskSCHEDULER_RUNNING     :
#if TERMOUT == 1
		vTaskPrioritySet(NULL, TASK_PRIO_HIGH);
		msg(INF, "%s: crit_err_exit(%s) on line %d\n", fname(file), errors[err], line);
		disable_tout();
		vTaskPrioritySet(tout_tsk_hndl(), TASK_PRIO_HIGH);
		while (pdTRUE == xQueuePeek(tout_mque(), &p, 0)) {
                        taskYIELD();
                }
		vTaskDelay(100 / portTICK_PERIOD_MS);
#endif
		/* FALLTHRU */
	case taskSCHEDULER_NOT_STARTED :
		/* FALLTHRU */
	case taskSCHEDULER_SUSPENDED   :
		/* FALLTHRU */
	default                        :
		crit_err(err);
		break;
	}
}

#if TERMOUT == 1
/**
 * fname
 */
static char *fname(char *file)
{
	int i = 0, p = 0;

	while (*(file + i) != '\0') {
		if (*(file + i) == '/' || *(file + i) == '\\') {
			p = i;
		}
		i++;
	}
	return ((p) ? file + ++p : file);
}
#endif

/**
 * crit_err
 */
static void crit_err(enum crit_err err)
{
#if defined(CRITERR_ANODE_ON_IO_PIN)
#if CRITERR_ANODE_ON_IO_PIN == 1
	conf_pin(CRITERR_SIG_PIN, CRITERR_SIG_PORT, PIN_FUNC_OUTPUT_HIGH, PIN_FEAT_END);
        conf_pin(CRITERR_0_PIN, CRITERR_0_PORT, PIN_FUNC_OUTPUT_HIGH, PIN_FEAT_END);
        conf_pin(CRITERR_1_PIN, CRITERR_1_PORT, PIN_FUNC_OUTPUT_HIGH, PIN_FEAT_END);
        conf_pin(CRITERR_2_PIN, CRITERR_2_PORT, PIN_FUNC_OUTPUT_HIGH, PIN_FEAT_END);
#else
	conf_pin(CRITERR_SIG_PIN, CRITERR_SIG_PORT, PIN_FUNC_OUTPUT_LOW, PIN_FEAT_END);
	conf_pin(CRITERR_0_PIN, CRITERR_0_PORT, PIN_FUNC_OUTPUT_LOW, PIN_FEAT_END);
	conf_pin(CRITERR_1_PIN, CRITERR_1_PORT, PIN_FUNC_OUTPUT_LOW, PIN_FEAT_END);
	conf_pin(CRITERR_2_PIN, CRITERR_2_PORT, PIN_FUNC_OUTPUT_LOW, PIN_FEAT_END);
#endif
#endif
	taskENTER_CRITICAL();
	while (WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);
	WDT->CTRL.reg &= ~WDT_CTRL_ENABLE;
        taskEXIT_CRITICAL();
	for (int i = 0; i < PERIPH_COUNT_IRQn; i++) {
		NVIC_DisableIRQ(i);
		NVIC_ClearPendingIRQ(i);
	}
        init_tc_50ms();
        if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
		vTaskSuspendAll();
	} else {
		__enable_irq();
	}
#if defined(CRITERR_ANODE_ON_IO_PIN)
#if CRITERR_ANODE_ON_IO_PIN == 1
	set_pin_lev(CRITERR_SIG_PIN, CRITERR_SIG_PORT, HIGH);
	set_pin_lev(CRITERR_0_PIN, CRITERR_0_PORT, HIGH);
	set_pin_lev(CRITERR_1_PIN, CRITERR_1_PORT, HIGH);
	set_pin_lev(CRITERR_2_PIN, CRITERR_2_PORT, HIGH);
#else
	set_pin_lev(CRITERR_SIG_PIN, CRITERR_SIG_PORT, LOW);
	set_pin_lev(CRITERR_0_PIN, CRITERR_0_PORT, LOW);
	set_pin_lev(CRITERR_1_PIN, CRITERR_1_PORT, LOW);
	set_pin_lev(CRITERR_2_PIN, CRITERR_2_PORT, LOW);
#endif
#endif
        intr_cnt = CRITERR_INTR_FREQ * 3;
        while (intr_cnt);
#if defined(CRITERR_ANODE_ON_IO_PIN)
#if CRITERR_ANODE_ON_IO_PIN == 1
	set_pin_lev(CRITERR_SIG_PIN, CRITERR_SIG_PORT, LOW);
	set_pin_lev(CRITERR_0_PIN, CRITERR_0_PORT, LOW);
	set_pin_lev(CRITERR_1_PIN, CRITERR_1_PORT, LOW);
	set_pin_lev(CRITERR_2_PIN, CRITERR_2_PORT, LOW);
#else
	set_pin_lev(CRITERR_SIG_PIN, CRITERR_SIG_PORT, HIGH);
	set_pin_lev(CRITERR_0_PIN, CRITERR_0_PORT, HIGH);
	set_pin_lev(CRITERR_1_PIN, CRITERR_1_PORT, HIGH);
	set_pin_lev(CRITERR_2_PIN, CRITERR_2_PORT, HIGH);
#endif
#endif
        intr_cnt = CRITERR_INTR_FREQ * 2;
        while (intr_cnt);
	show_err(err);
	led_sig_en = TRUE;
        while (TRUE);
}

/**
 * show_err
 */
static void show_err(enum crit_err err)
{
        int i;

        for (i = 0; i < 3; i++) {
                switch (i) {
                case 0 :
                        if (err & 1) {
#if defined(CRITERR_ANODE_ON_IO_PIN)
#if CRITERR_ANODE_ON_IO_PIN == 1
				set_pin_lev(CRITERR_2_PIN, CRITERR_2_PORT, HIGH);
#else
				set_pin_lev(CRITERR_2_PIN, CRITERR_2_PORT, LOW);
#endif
#endif
                        }
                        break;
                case 1 :
                        if (err & 1) {
#if defined(CRITERR_ANODE_ON_IO_PIN)
#if CRITERR_ANODE_ON_IO_PIN == 1
				set_pin_lev(CRITERR_1_PIN, CRITERR_1_PORT, HIGH);
#else
				set_pin_lev(CRITERR_1_PIN, CRITERR_1_PORT, LOW);
#endif
#endif
                        }
                        break;
                case 2 :
                        if (err & 1) {
#if defined(CRITERR_ANODE_ON_IO_PIN)
#if CRITERR_ANODE_ON_IO_PIN == 1
				set_pin_lev(CRITERR_0_PIN, CRITERR_0_PORT, HIGH);
#else
				set_pin_lev(CRITERR_0_PIN, CRITERR_0_PORT, LOW);
#endif
#endif
                        }
                        break;
                }
                err >>= 1;
        }
}

/**
 * init_tc_50ms
 */
static void init_tc_50ms(void)
{
	tc.id = CRITERR_TC_ID;
	tc.clk_gen = CRITERR_GCLK_ID;
	tc.clock_freq = CRITERR_GCLK_FREQ;
        tc.cnt_size = TC_CNT_SIZE_16_BIT;
        tc.cnt_sync = TC_CNT_SYNC_GCLK;
        tc.wavegen = TC_WAVEGEN_MFRQ;
	tc.direction = TC_DIRECTION_UP;
	tc.cc0 = tc.clock_freq / 1000 * 50;
	tc.int_enable_mask = TC_INTENSET_MC0;
        tc.isr_clbk = isr_clbk;
	init_tc(&tc);
}

/**
 * isr_clbk
 */
static BaseType_t isr_clbk(void *dev)
{
	static boolean_t ld_st;

	tc_clear_mc0_intr((tc_timer) dev);
	if (intr_cnt) {
		intr_cnt--;
	}
	if (led_sig_en) {
		if (ld_st) {
#if defined(CRITERR_ANODE_ON_IO_PIN)
#if CRITERR_ANODE_ON_IO_PIN == 1
			set_pin_lev(CRITERR_SIG_PIN, CRITERR_SIG_PORT, LOW);
#else
			set_pin_lev(CRITERR_SIG_PIN, CRITERR_SIG_PORT, HIGH);
#endif
#elif defined(CRITERR_LED_PIN) && defined(CRITERR_LED_PORT)
			set_pin_lev(CRITERR_LED_PIN, CRITERR_LED_PORT, LOW);
#endif
			ld_st = FALSE;
		} else {
#if defined(CRITERR_ANODE_ON_IO_PIN)
#if CRITERR_ANODE_ON_IO_PIN == 1
			set_pin_lev(CRITERR_SIG_PIN, CRITERR_SIG_PORT, HIGH);
#else
			set_pin_lev(CRITERR_SIG_PIN, CRITERR_SIG_PORT, LOW);
#endif
#elif defined(CRITERR_LED_PIN) && defined(CRITERR_LED_PORT)
			set_pin_lev(CRITERR_LED_PIN, CRITERR_LED_PORT, HIGH);
#endif
			ld_st = TRUE;
		}
	}
	return (pdFALSE);
}
#else

/**
 * crit_err_exit
 */
void crit_err_exit(enum crit_err err)
{
	portDISABLE_INTERRUPTS();
#if CRITERR_0_DISABLE_WDT == 1
	while (WDT->STATUS.reg & WDT_STATUS_SYNCBUSY);
	WDT->CTRL.reg &= ~WDT_CTRL_ENABLE;
#endif
	for (;;) {
		;
	}
}
#endif
