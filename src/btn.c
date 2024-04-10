/*
 * btn.c
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
#if BTN_SLEEP == 1
#include "sleep.h"
#endif
#include "btn.h"

#if BTN == 1

struct isr_msg {
	TickType_t tm;
	boolean_t intr_sig;
};

static btn btn_list;
static int isr_clbk_num;

static void btn_tsk(void *p);
static BaseType_t isr_clbk(unsigned short intflg);
#if BTN_SLEEP == 1
static void sleep_clbk(enum sleep_cmd cmd, ...);
#endif

/**
 * add_btn_dev
 */
void add_btn_dev(btn dev)
{
	dev->intr_que = xQueueCreate(1, sizeof(struct isr_msg));
	if (dev->intr_que == NULL) {
		crit_err_exit(MALLOC_ERROR);
	}
	dev->evnt_que = xQueueCreate(dev->evnt_que_size, sizeof(struct btn_evnt));
	if (dev->evnt_que == NULL) {
		crit_err_exit(MALLOC_ERROR);
	}
	reg_eintctl_isr_clbk(isr_clbk);
        taskENTER_CRITICAL();
	if (btn_list) {
		btn b = btn_list;
		while (b->next) {
			b = b->next;
		}
		b->next = dev;
	} else {
		btn_list = dev;
	}
	taskEXIT_CRITICAL();
	if (pdPASS != xTaskCreate(btn_tsk, dev->tsk_nm, BTN_TASK_STACK_SIZE, dev, BTN_TASK_PRIO, &dev->tsk_hndl)) {
                crit_err_exit(MALLOC_ERROR);
        }
	if (dev->eintctl_pin.intr_mode == EINTCTL_INTR_HIGH) {
		conf_pin(dev->pin, dev->port, PIN_FUNC_PERIPHERAL_A,
		         PIN_FEAT_INPUT_BUFFER, PIN_FEAT_PULL_DOWN, PIN_FEAT_END);
	} else if (dev->eintctl_pin.intr_mode == EINTCTL_INTR_LOW) {
		conf_pin(dev->pin, dev->port, PIN_FUNC_PERIPHERAL_A,
		         PIN_FEAT_INPUT_BUFFER, PIN_FEAT_PULL_UP, PIN_FEAT_END);
	} else {
		crit_err_exit(BAD_PARAMETER);
	}
        conf_eintctl_pin(&dev->eintctl_pin);
#if BTN_SLEEP == 1
	reg_sleep_clbk(sleep_clbk, SLEEP_PRIO_SUSP_FIRST);
#endif
}

/**
 * btn_tsk
 */
static void btn_tsk(void *p)
{
	btn b = p;
        struct btn_evnt evnt;
        struct isr_msg msg;
	int cnt;

	eintctl_intr_clear(&b->eintctl_pin); // EIC errata: spurious flag may appear.
        vTaskDelay(20 / portTICK_PERIOD_MS);
        while (TRUE) {
		eintctl_intr_clear(&b->eintctl_pin);
		eintctl_intr_enable(&b->eintctl_pin);
		xQueueReceive(b->intr_que, &msg, portMAX_DELAY);
#if BTN_SLEEP == 1
		if (msg.intr_sig) {
			if (!b->eintctl_pin.wake) {
				disable_eintctl_pin(&b->eintctl_pin);
			} else {
				eintctl_intr_enable(&b->eintctl_pin);
			}
                        msg(INF, "btn.c: (%s) suspend\n", b->tsk_nm);
                        vTaskSuspend(NULL);
                        if (!b->eintctl_pin.wake) {
				conf_eintctl_pin(&b->eintctl_pin);
			} else {
				xQueueReceive(b->intr_que, &msg, 0);
                                cnt = 0;
				while (TRUE) {
					vTaskDelay(BTN_CHECK_DELAY / portTICK_PERIOD_MS);
					if (b->eintctl_pin.intr_mode == EINTCTL_INTR_LOW) {
						if (get_pin_lev(b->pin, b->port)) {
							cnt++;
						} else {
							cnt = 0;
						}
					} else {
						if (!get_pin_lev(b->pin, b->port)) {
							cnt++;
						} else {
							cnt = 0;
						}
					}
					if (cnt == BTN_CHECK_DELAY_CNT) {
						break;
					}
				}
			}
			continue;
		}
#endif
		if (b->mode == BTN_EVENT_MODE) {
			evnt.type = BTN_PRESS;
                        evnt.time = msg.tm;
			if (errQUEUE_FULL == xQueueSend(b->evnt_que, &evnt, 0)) {
				b->evnt_que_full_err++;
			}
		}
                cnt = 0;
                while (TRUE) {
			vTaskDelay(BTN_CHECK_DELAY / portTICK_PERIOD_MS);
			if (b->eintctl_pin.intr_mode == EINTCTL_INTR_LOW) {
				if (get_pin_lev(b->pin, b->port)) {
					cnt++;
				} else {
					cnt = 0;
				}
			} else {
				if (!get_pin_lev(b->pin, b->port)) {
					cnt++;
				} else {
					cnt = 0;
				}
			}
			if (cnt == BTN_CHECK_DELAY_CNT) {
				if (b->mode == BTN_EVENT_MODE) {
					evnt.type = BTN_RELEASE;
					evnt.time = xTaskGetTickCount();
				} else {
					evnt.type = BTN_PRESSED_DOWN;
                                        evnt.time = xTaskGetTickCount() - msg.tm;

				}
				if (errQUEUE_FULL == xQueueSend(b->evnt_que, &evnt, 0)) {
					b->evnt_que_full_err++;
				}
				break;
			}
		}
	}
}

/**
 * isr_clbk
 */
static BaseType_t isr_clbk(unsigned short intflg)
{
	BaseType_t tsk_wkn = pdFALSE;
        struct isr_msg msg;
	btn b = btn_list;

	isr_clbk_num++;
        while (b != NULL) {
		if (intflg & (1 << b->eintctl_pin.n) && is_eintctl_intr_enabled(&b->eintctl_pin)) {
			b->clbk_num++;
			if (b->eintctl_pin.intr_mode == EINTCTL_INTR_LOW) {
				if (get_pin_lev(b->pin, b->port)) {
					eintctl_intr_clear(&b->eintctl_pin);
					b->pin_lev_err++;
					b = b->next;
					continue;
				}
			} else {
				if (!get_pin_lev(b->pin, b->port)) {
					eintctl_intr_clear(&b->eintctl_pin);
					b->pin_lev_err++;
					b = b->next;
					continue;
				}
			}
			eintctl_intr_disable(&b->eintctl_pin);
			msg.tm = xTaskGetTickCountFromISR();
			msg.intr_sig = FALSE;
			BaseType_t wkn = pdFALSE;
			if (errQUEUE_FULL == xQueueSendFromISR(b->intr_que, &msg, &wkn)) {
				b->intr_que_full_err++;
			}
			if (wkn) {
				tsk_wkn = pdTRUE;
			}
		}
                b = b->next;
	}
        return (tsk_wkn);
}

#if BTN_SLEEP == 1
/**
 * sleep_clbk
 */
static void sleep_clbk(enum sleep_cmd cmd, ...)
{
	btn b;

	if (btn_list) {
		b = btn_list;
	} else {
		return;
	}
	if (cmd == SLEEP_CMD_SUSP) {
		do {
			struct isr_msg msg;
                        msg.tm = 0;
			msg.intr_sig = TRUE;
                        eintctl_intr_disable(&b->eintctl_pin);
			xQueueSend(b->intr_que, &msg, portMAX_DELAY);
			while (eSuspended != eTaskGetState(b->tsk_hndl)) {
				taskYIELD();
			}
		} while ((b = b->next));
	} else {
		do {
			vTaskResume(b->tsk_hndl);
		} while ((b = b->next));
	}
}
#endif

#if TERMOUT == 1
/**
 * log_btn_stats
 */
void log_btn_stats(btn dev)
{
	msg(INF, "btn.c: <%s> intr_que_full_err=%d evnt_que_full_err=%d\n",
	    dev->tsk_nm, dev->intr_que_full_err, dev->evnt_que_full_err);
	msg(INF, "btn.c: <%s> pin_lev_err=%d clbk_num=%d clbk_num_all=%d\n",
	    dev->tsk_nm, dev->pin_lev_err, dev->clbk_num, isr_clbk_num);
}
#endif

#endif
