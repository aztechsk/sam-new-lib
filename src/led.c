/*
 * led.c
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
#include "tc.h"
#if LED_SLEEP == 1
#include "sleep.h"
#endif
#include "led.h"
#include <stdarg.h>

#if LED == 1

static led led_list;
static struct tc_timer_dsc tc;
static TaskHandle_t tsk_hndl;
static const char *const tsk_nm = "LED";
#if LED_SLEEP == 1
static volatile boolean_t sleep_req;
#endif

static void led_tsk(void *p);
static BaseType_t isr_clbk(void *dev);
static void set_led_on(led ld);
static void set_led_off(led ld);
#if LED_SLEEP == 1
static void sleep_clbk(enum sleep_cmd cmd, ...);
#endif

/**
 * init_led
 */
void init_led(void)
{
	tc.id = LED_TC_ID;
	tc.clk_gen = LED_GCLK_ID;
	tc.clock_freq = LED_GCLK_FREQ;
        tc.cnt_size = TC_CNT_SIZE_16_BIT;
        tc.cnt_sync = TC_CNT_SYNC_GCLK;
        tc.wavegen = TC_WAVEGEN_MFRQ;
	tc.direction = TC_DIRECTION_UP;
	tc.cc0 = tc.clock_freq / 1000 * LED_ON_TIME;
        tc.isr_clbk = isr_clbk;
	init_tc(&tc);
        if (pdPASS != xTaskCreate(led_tsk, tsk_nm, LED_TASK_STACK_SIZE, NULL,
                                  LED_TASK_PRIO, &tsk_hndl)) {
                crit_err_exit(MALLOC_ERROR);
        }
#if LED_SLEEP == 1
	reg_sleep_clbk(sleep_clbk, SLEEP_PRIO_SUSP_SECOND);
#endif
}

/**
 * add_led_dev
 */
void add_led_dev(led dev)
{
	led l;

	if (dev->anode_on_pin) {
		conf_pin(dev->pin, dev->port, PIN_FUNC_OUTPUT_LOW, PIN_FEAT_END);
	} else {
		conf_pin(dev->pin, dev->port, PIN_FUNC_OUTPUT_HIGH, PIN_FEAT_END);
	}
	taskENTER_CRITICAL();
	if (led_list) {
		l = led_list;
		while (l->next) {
			l = l->next;
		}
		l->next = dev;
	} else {
		led_list = dev;
	}
        dev->state = dev->state_chng = LED_STATE_OFF;
	dev->off = FALSE;
        dev->next = NULL;
	taskEXIT_CRITICAL();
}

/**
 * set_led_dev_state
 */
void set_led_dev_state(led dev, enum led_state state, ...)
{
	va_list ap;

	switch (state) {
	case LED_STATE_ON     :
		/* FALLTHRU */
	case LED_STATE_OFF    :
		/* FALLTHRU */
        case LED_STATE_FLASH  :
		dev->state_chng = state;
		break;
        case LED_STATE_BLINK  :
		va_start(ap, state);
                taskENTER_CRITICAL();
		dev->state_chng = state;
		dev->delay_chng = va_arg(ap, int);;
		taskEXIT_CRITICAL();
                va_end(ap);
                break;
	default               :
		crit_err_exit(BAD_PARAMETER);
		break;
	}
}

/**
 * led_tsk
 */
static void led_tsk(void *p)
{
        static TickType_t xLastWakeTime, freq;
	static led ld;
	static boolean_t swtrg;

        freq = LED_BASE_FREQ / portTICK_PERIOD_MS;
        xLastWakeTime = xTaskGetTickCount();
        while (TRUE) {
                vTaskDelayUntil(&xLastWakeTime, freq);
#if LED_SLEEP == 1
		if (sleep_req) {
			sleep_req = FALSE;
			if (led_list) {
				ld = led_list;
				do {
					if (ld->state == LED_STATE_ON) {
						set_led_off(ld);
					}
				} while ((ld = ld->next));
			}
                        disable_tc(&tc);
                        msg(INF, "led.c: suspend\n");
                        vTaskSuspend(NULL);
			enable_tc(&tc);
			if (led_list) {
				ld = led_list;
				do {
					if (ld->state == LED_STATE_ON) {
						set_led_on(ld);
					}
				} while ((ld = ld->next));
			}
			xLastWakeTime = xTaskGetTickCount();
			continue;
		}
#endif
		if (!led_list) {
			continue;
		}
		ld = led_list;
		swtrg = FALSE;
		do {
			taskENTER_CRITICAL();
			if (ld->state != ld->state_chng) {
				ld->state = ld->state_chng;
				if (ld->state == LED_STATE_FLASH) {
					ld->state_chng = LED_STATE_OFF;
				}
                                taskEXIT_CRITICAL();
				if (ld->state == LED_STATE_ON) {
					set_led_on(ld);
				} else if (ld->state == LED_STATE_OFF) {
					set_led_off(ld);
				} else if (ld->state == LED_STATE_FLASH) {
					set_led_on(ld);
                                        ld->off = TRUE;
                                        swtrg = TRUE;
                                        ld->state = LED_STATE_OFF;
				} else if (ld->state == LED_STATE_BLINK) {
					set_led_on(ld);
                                        ld->off = TRUE;
                                        swtrg = TRUE;
                                        ld->dly_cnt = ld->delay = ld->delay_chng;
				}
			} else {
				taskEXIT_CRITICAL();
				if (ld->state == LED_STATE_BLINK) {
					if (ld->delay != ld->delay_chng) {
						ld->dly_cnt = ld->delay = ld->delay_chng;
						set_led_on(ld);
                                                ld->off = TRUE;
                                                swtrg = TRUE;
					} else {
						if (ld->dly_cnt) {
							ld->dly_cnt--;
						} else {
							set_led_on(ld);
							ld->off = TRUE;
                                                        swtrg = TRUE;
							ld->dly_cnt = ld->delay;
						}
					}
				}
			}
		} while ((ld = ld->next));
		if (swtrg) {
			tc_stop(&tc);
			tc_set_cnt(&tc, 0);
                        tc_clear_mc0_intr(&tc);
			tc_enable_mc0_intr(&tc);
			tc_trigger(&tc);
		}
        }
}

/**
 * isr_clbk
 */
static BaseType_t isr_clbk(void *dev)
{
	led ld;

	tc_disable_mc0_intr((tc_timer) dev);
	ld = led_list;
	do {
		if (ld->off) {
			ld->off = FALSE;
			set_led_off(ld);
		}
	} while ((ld = ld->next));
	return (pdFALSE);
}

/**
 * set_led_on
 */
static void set_led_on(led ld)
{
	if (ld->anode_on_pin) {
                set_pin_lev(ld->pin, ld->port, HIGH);
	} else {
                set_pin_lev(ld->pin, ld->port, LOW);
	}
}

/**
 * set_led_off
 */
static void set_led_off(led ld)
{
	if (ld->anode_on_pin) {
                set_pin_lev(ld->pin, ld->port, LOW);
	} else {
                set_pin_lev(ld->pin, ld->port, HIGH);
	}
}

#if LED_SLEEP == 1
/**
 * sleep_clbk
 */
static void sleep_clbk(enum sleep_cmd cmd, ...)
{
	if (cmd == SLEEP_CMD_SUSP) {
		sleep_req = TRUE;
		while (eSuspended != eTaskGetState(tsk_hndl)) {
			taskYIELD();
		}
	} else {
		vTaskResume(tsk_hndl);
	}
}
#endif

#endif
