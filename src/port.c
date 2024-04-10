/*
 * port.c
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
#include "hwerr.h"
#include "pm.h"
#include "port.h"
#include <stdarg.h>

extern inline boolean_t get_pin_lev(int pin, PortGroup *grp);
extern inline void set_pin_lev(int pin, PortGroup *grp, boolean_t lev);
extern inline boolean_t get_pin_out(int pin, PortGroup *grp);

/**
 * init_port
 */
void init_port(void)
{
	enable_per_apb_clk(APB_BUS_INST_B, PM_APBBMASK_PORT);
}

/**
 * conf_pin
 */
void conf_pin(int pin, PortGroup *grp, enum port_pin_func func, ...)
{
	va_list ap;
        enum port_pin_feat feat;
	uint8_t pincfg;
	boolean_t cont_in = FALSE, pup = FALSE, pdown = FALSE;

	if (grp == PORTA) {
#ifdef PORTB
	} else if (grp == PORTB) {
#endif
#ifdef PORTC
	} else if (grp == PORTC) {
#endif
	} else {
		crit_err_exit(BAD_PARAMETER);
	}
	if (pin > 31) {
                crit_err_exit(BAD_PARAMETER);
	}
        pincfg = 0;
        va_start(ap, func);
	while ((feat = va_arg(ap, int)) != PIN_FEAT_END) {
		if (feat == PIN_FEAT_INPUT_BUFFER) {
			pincfg |= PORT_PINCFG_INEN;
		} else if (feat == PIN_FEAT_CONT_INPUT_READ) {
			cont_in = TRUE;
		} else if (feat == PIN_FEAT_STRONG_DRIVER) {
			pincfg |= PORT_PINCFG_DRVSTR;
		} else if (feat == PIN_FEAT_PULL_UP) {
			pup = TRUE;
		} else if (feat == PIN_FEAT_PULL_DOWN) {
			pdown = TRUE;
		} else {
			crit_err_exit(BAD_PARAMETER);
		}
	}
	va_end(ap);
        if (pup && pdown) {
		crit_err_exit(BAD_PARAMETER);
	}
	if (func > PIN_FUNC_DIGITAL_OFF) {
		if (func > PIN_FUNC_PERIPHERAL_I) {
			crit_err_exit(BAD_PARAMETER);
		}
		uint8_t pmux = func - PIN_FUNC_PERIPHERAL_A;
                pincfg |= PORT_PINCFG_PMUXEN;
		if (pup || pdown) {
			pincfg |= PORT_PINCFG_PULLEN;
		}
                taskENTER_CRITICAL();
		uint8_t reg = grp->PMUX[pin / 2].reg;
		if (pin % 2) {
			reg &= ~PORT_PMUX_PMUXO_Msk;
			reg |= pmux << PORT_PMUX_PMUXO_Pos;
		} else {
			reg &= ~PORT_PMUX_PMUXE_Msk;
			reg |= pmux << PORT_PMUX_PMUXE_Pos;
		}
		if (pup) {
			grp->OUT.reg |= 1 << pin;
		}
		if (pdown) {
			grp->OUT.reg &= ~(1 << pin);
		}
		grp->PMUX[pin / 2].reg = reg;
		grp->PINCFG[pin].reg = pincfg;
		if (pincfg & PORT_PINCFG_INEN && cont_in == TRUE) {
			grp->CTRL.reg |= 1 << pin;
		} else {
			grp->CTRL.reg &= ~(1 << pin);
		}
                taskEXIT_CRITICAL();
		return;
	}
	taskENTER_CRITICAL();
	switch (func) {
	case PIN_FUNC_INPUT           :
		pincfg |= PORT_PINCFG_INEN;
		grp->DIR.reg &= ~(1 << pin);
		break;
	case PIN_FUNC_INPUT_PULL_UP   :
		pincfg |= PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
		grp->OUT.reg |= 1 << pin;
                grp->DIR.reg &= ~(1 << pin);
		break;
        case PIN_FUNC_INPUT_PULL_DOWN :
		pincfg |= PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
                grp->OUT.reg &= ~(1 << pin);
                grp->DIR.reg &= ~(1 << pin);
		break;
	case PIN_FUNC_OUTPUT_HIGH     :
                grp->OUT.reg |= 1 << pin;
		grp->DIR.reg |= 1 << pin;
		break;
        case PIN_FUNC_OUTPUT_LOW      :
                grp->OUT.reg &= ~(1 << pin);
		grp->DIR.reg |= 1 << pin;
		break;
	case PIN_FUNC_PULL_UP         :
		pincfg &= ~PORT_PINCFG_INEN;
		pincfg |= PORT_PINCFG_PULLEN;
                grp->OUT.reg |= 1 << pin;
		grp->DIR.reg &= ~(1 << pin);
		break;
        case PIN_FUNC_PULL_DOWN       :
		pincfg &= ~PORT_PINCFG_INEN;
		pincfg |= PORT_PINCFG_PULLEN;
                grp->OUT.reg &= ~(1 << pin);
                grp->DIR.reg &= ~(1 << pin);
		break;
	case PIN_FUNC_DIGITAL_OFF     :
		pincfg = 0;
                grp->DIR.reg &= ~(1 << pin);
		break;
	default                       :
		taskEXIT_CRITICAL();
                crit_err_exit(BAD_PARAMETER);
		break;
	}
	grp->PINCFG[pin].reg = pincfg;
	if (pincfg & PORT_PINCFG_INEN && cont_in == TRUE) {
		grp->CTRL.reg |= 1 << pin;
	} else {
		grp->CTRL.reg &= ~(1 << pin);
	}
        taskEXIT_CRITICAL();
}
