/*
 * eic.c
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
#include "pm.h"
#include "gclk.h"
#include "eic.h"
#include <stdarg.h>

#if EINTCTL == 1
extern inline void eintctl_intr_enable(eintctl_pin pin);
extern inline void eintctl_intr_disable(eintctl_pin pin);
extern inline boolean_t is_eintctl_intr_enabled(eintctl_pin pin);
extern inline void eintctl_intr_clear(eintctl_pin pin);

static BaseType_t (*clbk_ary[EINTCTL_CLBK_ARY_SIZE])(unsigned short);
static boolean_t gclk_enabled;

static inline boolean_t sync(void);

/**
 * init_eintctl
 */
void init_eintctl(enum eintctl_gclk gclk, ...)
{
	NVIC_DisableIRQ(EIC_IRQn);
	if (gclk == EINTCTL_GCLK_ON) {
		va_list ap;
                va_start(ap, gclk);
		enable_clk_channel(GCLK_CLKCTRL_ID_EIC_Val, va_arg(ap, int));
                va_end(ap);
                gclk_enabled = TRUE;
	}
        enable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_EIC);
        EIC->CTRL.reg = EIC_CTRL_SWRST;
	while (!sync());
	EIC->CTRL.reg = EIC_CTRL_ENABLE;
	while (!sync());
        NVIC_SetPriority(EIC_IRQn, configLIBRARY_API_CALL_INTERRUPT_PRIORITY);
        NVIC_ClearPendingIRQ(EIC_IRQn);
	NVIC_EnableIRQ(EIC_IRQn);
}

/**
 * enable_eintctl
 */
void enable_eintctl(void)
{
	if (gclk_enabled) {
		re_enable_clk_channel(GCLK_CLKCTRL_ID_EIC_Val);
	}
	enable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_EIC);
	EIC->CTRL.reg = EIC_CTRL_ENABLE;
	while (!sync());
        NVIC_ClearPendingIRQ(EIC_IRQn);
	NVIC_EnableIRQ(EIC_IRQn);
}

/**
 * disable_eintctl
 */
void disable_eintctl(void)
{
	NVIC_DisableIRQ(EIC_IRQn);
	EIC->CTRL.reg &= ~EIC_CTRL_ENABLE;
	while (!sync());
	if (gclk_enabled) {
		disable_clk_channel(GCLK_CLKCTRL_ID_EIC_Val);
	}
	disable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_EIC);
}

/**
 * is_eintctl_enabled
 */
boolean_t is_eintctl_enabled(void)
{
	if (EIC->CTRL.reg & EIC_CTRL_ENABLE) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}

/**
 * reg_eintctl_isr_clbk
 */
void reg_eintctl_isr_clbk(BaseType_t (*clbk)(unsigned short intflg))
{
	taskENTER_CRITICAL();
	for (int i = 0; i < EINTCTL_CLBK_ARY_SIZE; i++) {
		if (!clbk_ary[i]) {
			clbk_ary[i] = clbk;
                        taskEXIT_CRITICAL();
			return;
		} else {
			if (clbk_ary[i] == clbk) {
				taskEXIT_CRITICAL();
				return;
			}
		}
	}
        taskEXIT_CRITICAL();
	crit_err_exit(UNEXP_PROG_STATE);
}

/**
 * conf_eintctl_pin
 */
void conf_eintctl_pin(eintctl_pin pin)
{
	unsigned int fs, r, m;
	int shf;

	fs = pin->intr_mode;
	if (pin->filt) {
		fs |= 1 << 3;
	}
	if (pin->n > 7) {
		shf = (pin->n - 8) * 4;
	} else {
		shf = pin->n * 4;
	}
	fs <<= shf;
	m = 0x0F << shf;
	taskENTER_CRITICAL();
	if (pin->n > 7) {
		r = EIC->CONFIG[1].reg;
	} else {
		r = EIC->CONFIG[0].reg;
	}
	r &= ~m;
	r |= fs;
	if (pin->n > 7) {
		EIC->CONFIG[1].reg = r;
	} else {
		EIC->CONFIG[0].reg = r;
	}
	if (pin->wake) {
		EIC->WAKEUP.reg |= 1 << pin->n;
	} else {
		EIC->WAKEUP.reg &= ~(1 << pin->n);
	}
	taskEXIT_CRITICAL();
}

/**
 * disable_eintctl_pin
 */
void disable_eintctl_pin(eintctl_pin pin)
{
	unsigned int r, m;
	int shf;

	if (pin->n > 7) {
		shf = (pin->n - 8) * 4;
	} else {
		shf = pin->n * 4;
	}
        m = 0x0F << shf;
	taskENTER_CRITICAL();
	if (pin->n > 7) {
		r = EIC->CONFIG[1].reg;
	} else {
		r = EIC->CONFIG[0].reg;
	}
	r &= ~m;
	if (pin->n > 7) {
		EIC->CONFIG[1].reg = r;
	} else {
		EIC->CONFIG[0].reg = r;
	}
	EIC->WAKEUP.reg &= ~(1 << pin->n);
	taskEXIT_CRITICAL();
}

/**
 * eintctl_intr_clear_all
 */
void eintctl_intr_clear_all(void)
{
	EIC->INTFLAG.reg = ~0;
	NVIC_ClearPendingIRQ(EIC_IRQn);
}

/**
 * EIC_Handler
 */
void EIC_Handler(void)
{
	BaseType_t tsk_wkn = pdFALSE;
	unsigned short intflg = EIC->INTFLAG.reg;

	for (int i = 0; i < EINTCTL_CLBK_ARY_SIZE; i++) {
		if (clbk_ary[i]) {
			if (pdTRUE == clbk_ary[i](intflg)) {
				tsk_wkn = pdTRUE;
			}
			continue;
		}
		break;
	}
	portEND_SWITCHING_ISR(tsk_wkn);
}

/**
 * sync
 */
static inline boolean_t sync(void)
{
	return ((EIC->STATUS.reg & EIC_STATUS_SYNCBUSY) ? FALSE : TRUE);
}
#endif
