/*
 * wdt.c
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
#include "wdt.h"

static inline boolean_t sync(void);

/**
 * init_wdt
 */
void init_wdt(int gen)
{
	enable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_WDT);
        if (gen != get_clk_channel_gen(GCLK_CLKCTRL_ID_WDT_Val)) {
		disable_clk_channel(GCLK_CLKCTRL_ID_WDT_Val);
		enable_clk_channel(GCLK_CLKCTRL_ID_WDT_Val, gen);
	}
	if (WDT->CTRL.reg & WDT_CTRL_ALWAYSON) {
		return;
	}
	while (!sync());
	WDT->CTRL.reg &= ~WDT_CTRL_ENABLE;
	while (!sync());
	WDT->CTRL.reg = 0;
        while (!sync());
	WDT->CONFIG.reg = WDT_CONFIG_WINDOW(WDT_CFG_WINDOW) | WDT_CONFIG_PER(WDT_CFG_PERIOD);
        while (!sync());
	WDT->INTENCLR.reg = WDT_INTENCLR_EW;
#if WDT_CFG_WIND_EN == 1
	WDT->CTRL.reg = WDT_CTRL_WEN | WDT_CTRL_ENABLE;
#else
	WDT->CTRL.reg = WDT_CTRL_ENABLE;
#endif
	while (!sync());
}

/**
 * enable_wdt
 */
void enable_wdt(void)
{
	re_enable_clk_channel(GCLK_CLKCTRL_ID_WDT_Val);
	enable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_WDT);
	taskENTER_CRITICAL();
        while (!sync());
	WDT->CTRL.reg |= WDT_CTRL_ENABLE;
        while (!sync());
        taskEXIT_CRITICAL();
}

/**
 * disable_wdt
 */
void disable_wdt(void)
{
	enable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_WDT);
	taskENTER_CRITICAL();
        while (!sync());
	WDT->CTRL.reg &= ~WDT_CTRL_ENABLE;
        while (!sync());
        WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
        while (!sync());
        taskEXIT_CRITICAL();
        disable_per_apb_clk(APB_BUS_INST_A, PM_APBAMASK_WDT);
        disable_clk_channel(GCLK_CLKCTRL_ID_WDT_Val);
}

/**
 * clear_wdt
 */
void clear_wdt(void)
{
	while (!sync());
	WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
}

/**
 * sync
 */
static inline boolean_t sync(void)
{
	return ((WDT->STATUS.reg & WDT_STATUS_SYNCBUSY) ? FALSE : TRUE);
}
