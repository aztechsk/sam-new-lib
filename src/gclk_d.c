/*
 * gclk_d.c
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

static inline boolean_t sync(void);

/**
 * init_gclk
 */
void init_gclk(void)
{
	enable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_GCLK);
}

/**
 * reset_gclk
 */
void reset_gclk(void)
{
	enable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_GCLK);
	GCLK->CTRL.reg = GCLK_CTRL_SWRST;
	while (!sync() || GCLK->CTRL.reg & GCLK_CTRL_SWRST);
}

/**
 * enable_clk_gen
 */
void enable_clk_gen(int ins, int src, int div, unsigned int flags)
{
	unsigned int genctrl = GCLK_GENCTRL_ID(ins), gendiv = GCLK_GENDIV_ID(ins);

	if (div > 1) {
		int df = 0;
		if ((div & (div - 1)) == 0) {
			int b = 2;
			while (b < div) {
				df++;
				b <<= 1;
			}
			gendiv |= df << GCLK_GENDIV_DIV_Pos;
			genctrl |= GCLK_GENCTRL_DIVSEL;
		} else {
			gendiv |= div << GCLK_GENDIV_DIV_Pos;
			genctrl |= GCLK_GENCTRL_IDC;
		}
	}
        genctrl |= GCLK_GENCTRL_SRC(src);
        if (flags & ~(GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_OE | GCLK_GENCTRL_OOV)) {
		crit_err_exit(BAD_PARAMETER);
	}
	genctrl |= flags | GCLK_GENCTRL_GENEN;
        taskENTER_CRITICAL();
	while (!sync());
        *((volatile uint8_t *) &GCLK->GENDIV.reg) = GCLK_GENDIV_ID(ins);
        while (!sync());
        GCLK->GENDIV.reg = gendiv;
	while (!sync());
	*((volatile uint8_t *) &GCLK->GENCTRL.reg) = GCLK_GENCTRL_ID(ins);
        while (!sync());
        GCLK->GENCTRL.reg = genctrl;
	while (!sync());
        taskEXIT_CRITICAL();
}

/**
 * disable_clk_gen
 */
void disable_clk_gen(int ins)
{
	unsigned int reg;

	taskENTER_CRITICAL();
	while (!sync());
        *((volatile uint8_t *) &GCLK->GENCTRL.reg) = GCLK_GENCTRL_ID(ins);
        while (!sync());
	reg = GCLK->GENCTRL.reg;
	reg &= ~GCLK_GENCTRL_GENEN;
        *((volatile uint8_t *) &GCLK->GENCTRL.reg) = GCLK_GENCTRL_ID(ins);
	while (!sync());
        GCLK->GENCTRL.reg = reg;
	while (!sync());
	taskEXIT_CRITICAL();
}

/**
 * re_enable_clk_gen
 */
void re_enable_clk_gen(int ins)
{
	unsigned int reg;

	taskENTER_CRITICAL();
	while (!sync());
        *((volatile uint8_t *) &GCLK->GENCTRL.reg) = GCLK_GENCTRL_ID(ins);
	while (!sync());
	reg = GCLK->GENCTRL.reg;
	reg |= GCLK_GENCTRL_GENEN;
        *((volatile uint8_t *) &GCLK->GENCTRL.reg) = GCLK_GENCTRL_ID(ins);
	while (!sync());
        GCLK->GENCTRL.reg = reg;
	while (!sync());
	taskEXIT_CRITICAL();
}

/**
 * change_clk_gen_src
 */
void change_clk_gen_src(int ins, int src)
{
	unsigned int reg;

	taskENTER_CRITICAL();
	while (!sync());
        *((volatile uint8_t *) &GCLK->GENCTRL.reg) = GCLK_GENCTRL_ID(ins);
	while (!sync());
	reg = GCLK->GENCTRL.reg;
        *((volatile uint8_t *) &GCLK->GENCTRL.reg) = GCLK_GENCTRL_ID(ins);
	while (!sync());
	GCLK->GENCTRL.reg = (reg & ~GCLK_GENCTRL_SRC_Msk) | GCLK_GENCTRL_SRC(src);
	while (!sync());
	taskEXIT_CRITICAL();
}

/**
 * enable_clk_channel
 */
void enable_clk_channel(int chn, int gen)
{
	taskENTER_CRITICAL();
	while (!sync());
	*((volatile uint8_t *) &GCLK->CLKCTRL.reg) = GCLK_CLKCTRL_ID(chn);
        while (!sync());
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(gen) | GCLK_CLKCTRL_ID(chn);
        while (!sync());
	taskEXIT_CRITICAL();
}

/**
 * re_enable_clk_channel
 */
void re_enable_clk_channel(int chn)
{
	unsigned short reg;

	taskENTER_CRITICAL();
	while (!sync());
        *((volatile uint8_t *) &GCLK->CLKCTRL.reg) = GCLK_CLKCTRL_ID(chn);
        while (!sync());
	reg = GCLK->CLKCTRL.reg;
	reg |= GCLK_CLKCTRL_CLKEN;
	GCLK->CLKCTRL.reg = reg;
        while (!sync());
	taskEXIT_CRITICAL();
}

/**
 * disable_clk_channel
 */
void disable_clk_channel(int chn)
{
	unsigned short reg;

	taskENTER_CRITICAL();
	while (!sync());
        *((volatile uint8_t *) &GCLK->CLKCTRL.reg) = GCLK_CLKCTRL_ID(chn);
        while (!sync());
        reg = GCLK->CLKCTRL.reg;
        reg &= ~GCLK_CLKCTRL_CLKEN;
	GCLK->CLKCTRL.reg = reg;
        while (!sync());
	taskEXIT_CRITICAL();
}

/**
 * get_clk_channel_gen
 */
int get_clk_channel_gen(int chn)
{
	int gclk;

	taskENTER_CRITICAL();
	while (!sync());
        *((volatile uint8_t *) &GCLK->CLKCTRL.reg) = GCLK_CLKCTRL_ID(chn);
        while (!sync());
	gclk = GCLK->CLKCTRL.reg;
        taskEXIT_CRITICAL();
	gclk &= GCLK_CLKCTRL_GEN_Msk;
	gclk >>= GCLK_CLKCTRL_GEN_Pos;
	return (gclk);
}

/**
 * sync
 */
static inline boolean_t sync(void)
{
	return ((GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) ? FALSE : TRUE);
}
