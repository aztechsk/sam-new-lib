/*
 * sercom.c
 *
 * Copyright (c) 2020 Jan Rusnak <jan@rusnak.sk>
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
#include "sercom.h"

struct clbk {
	void *dev;
        BaseType_t (*hndlr)(void *);
};

#ifdef SERCOM0
static struct clbk clbk0;

/**
 * SERCOM0_Handler
 */
void SERCOM0_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk0.hndlr)(clbk0.dev));
}
#endif

#ifdef SERCOM1
static struct clbk clbk1;

/**
 * SERCOM1_Handler
 */
void SERCOM1_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk1.hndlr)(clbk1.dev));
}
#endif

#ifdef SERCOM2
static struct clbk clbk2;

/**
 * SERCOM2_Handler
 */
void SERCOM2_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk2.hndlr)(clbk2.dev));
}
#endif

#ifdef SERCOM3
static struct clbk clbk3;

/**
 * SERCOM3_Handler
 */
void SERCOM3_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk3.hndlr)(clbk3.dev));
}
#endif

#ifdef SERCOM4
static struct clbk clbk4;

/**
 * SERCOM4_Handler
 */
void SERCOM4_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk4.hndlr)(clbk4.dev));
}
#endif

#ifdef SERCOM5
static struct clbk clbk5;

/**
 * SERCOM5_Handler
 */
void SERCOM5_Handler(void)
{
	portEND_SWITCHING_ISR((*clbk5.hndlr)(clbk5.dev));
}
#endif

/**
 * reg_sercom_isr_clbk
 */
void reg_sercom_isr_clbk(int id, BaseType_t (*hndlr)(void *), void *dev)
{
#ifdef SERCOM0
	if (id == ID_SERCOM0) {
		clbk0.hndlr = hndlr;
		clbk0.dev = dev;
		return;
	}
#endif
#ifdef SERCOM1
	if (id == ID_SERCOM1) {
		clbk1.hndlr = hndlr;
		clbk1.dev = dev;
		return;
	}
#endif
#ifdef SERCOM2
	if (id == ID_SERCOM2) {
		clbk2.hndlr = hndlr;
		clbk2.dev = dev;
		return;
	}
#endif
#ifdef SERCOM3
	if (id == ID_SERCOM3) {
		clbk3.hndlr = hndlr;
		clbk3.dev = dev;
		return;
	}
#endif
#ifdef SERCOM4
	if (id == ID_SERCOM4) {
		clbk4.hndlr = hndlr;
		clbk4.dev = dev;
		return;
	}
#endif
#ifdef SERCOM5
	if (id == ID_SERCOM5) {
		clbk5.hndlr = hndlr;
		clbk5.dev = dev;
		return;
	}
#endif
	crit_err_exit(BAD_PARAMETER);
}
