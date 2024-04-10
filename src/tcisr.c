/*
 * tcisr.c
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
#include "tcisr.h"

#ifndef TCC_TIMER
 #define TCC_TIMER 0
#endif

#ifndef TC_TIMER
 #define TC_TIMER 0
#endif

struct clbk {
	void *dev;
        BaseType_t (*hndlr)(void *);
};

#if TCC_TIMER == 1

#ifdef TCC0
static struct clbk clbk_tcc0;

/**
 * TCC0_Handler
 */
void TCC0_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk_tcc0.hndlr)(clbk_tcc0.dev));
}
#endif

#ifdef TCC1
static struct clbk clbk_tcc1;

/**
 * TCC1_Handler
 */
void TCC1_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk_tcc1.hndlr)(clbk_tcc1.dev));
}
#endif

#ifdef TCC2
static struct clbk clbk_tcc2;

/**
 * TCC2_Handler
 */
void TCC2_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk_tcc2.hndlr)(clbk_tcc2.dev));
}
#endif

#endif

#if TC_TIMER == 1

#ifdef TC3
static struct clbk clbk_tc3;

/**
 * TC3_Handler
 */
void TC3_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk_tc3.hndlr)(clbk_tc3.dev));
}
#endif

#ifdef TC4
static struct clbk clbk_tc4;

/**
 * TC4_Handler
 */
void TC4_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk_tc4.hndlr)(clbk_tc4.dev));
}
#endif

#ifdef TC5
static struct clbk clbk_tc5;

/**
 * TC5_Handler
 */
void TC5_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk_tc5.hndlr)(clbk_tc5.dev));
}
#endif

#ifdef TC6
static struct clbk clbk_tc6;

/**
 * TC6_Handler
 */
void TC6_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk_tc6.hndlr)(clbk_tc6.dev));
}
#endif

#ifdef TC7
static struct clbk clbk_tc7;

/**
 * TC7_Handler
 */
void TC7_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk_tc7.hndlr)(clbk_tc7.dev));
}
#endif

#endif

/**
 * reg_tc_isr_clbk
 */
void reg_tc_isr_clbk(int id, BaseType_t (*clbk)(void *), void *dev)
{
#if TCC_TIMER == 1
#ifdef TCC0
	if (id == ID_TCC0) {
		clbk_tcc0.hndlr = clbk;
		clbk_tcc0.dev = dev;
		return;
	}
#endif
#ifdef TCC1
	if (id == ID_TCC1) {
		clbk_tcc1.hndlr = clbk;
		clbk_tcc1.dev = dev;
		return;
	}
#endif
#ifdef TCC2
	if (id == ID_TCC2) {
		clbk_tcc2.hndlr = clbk;
		clbk_tcc2.dev = dev;
		return;
	}
#endif
#endif
#if TC_TIMER == 1
#ifdef TC3
	if (id == ID_TC3) {
		clbk_tc3.hndlr = clbk;
		clbk_tc3.dev = dev;
		return;
	}
#endif
#ifdef TC4
	if (id == ID_TC4) {
		clbk_tc4.hndlr = clbk;
		clbk_tc4.dev = dev;
		return;
	}
#endif
#ifdef TC5
	if (id == ID_TC5) {
		clbk_tc5.hndlr = clbk;
		clbk_tc5.dev = dev;
		return;
	}
#endif
#ifdef TC6
	if (id == ID_TC6) {
		clbk_tc6.hndlr = clbk;
		clbk_tc6.dev = dev;
		return;
	}
#endif
#ifdef TC7
	if (id == ID_TC7) {
		clbk_tc7.hndlr = clbk;
		clbk_tc7.dev = dev;
		return;
	}
#endif
#endif
	crit_err_exit(BAD_PARAMETER);
}
