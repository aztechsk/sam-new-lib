/*
 * pm_d.c
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

#if TERMOUT == 1
static const char *const reset_cause_ary[] = {"SYST", "WDT", "EXT", "BOD33", "BOD12", "POR", "ERR"};
#endif
static uint8_t rcause = 0xFF;

/**
 * init_pm
 */
void init_pm(void)
{
	rcause = PM->RCAUSE.reg;
#if PM_USER_RESET_ENABLED == 1
	PM->EXTCTRL.reg = 0;
#else
	PM->EXTCTRL.reg = PM_EXTCTRL_SETDIS;
#endif
}

/**
 * enable_per_ahb_clk
 */
void enable_per_ahb_clk(unsigned int bmp)
{
	taskENTER_CRITICAL();
	PM->AHBMASK.reg |= bmp;
        taskEXIT_CRITICAL();
}

/**
 * disable_per_ahb_clk
 */
void disable_per_ahb_clk(unsigned int bmp)
{
	taskENTER_CRITICAL();
	PM->AHBMASK.reg &= ~bmp;
        taskEXIT_CRITICAL();
}

/**
 * set_cpu_clk_div
 */
void set_cpu_clk_div(int div)
{
	if (div < 1 || div > 128 || ((div & (div - 1)) != 0)) {
		crit_err_exit(BAD_PARAMETER);
	}
	int df = 0, b = 2;
	while (b - 1 < div) {
		df++;
		b <<= 1;
	}
        taskENTER_CRITICAL();
        PM->INTFLAG.reg = PM_INTFLAG_CKRDY;
	PM->CPUSEL.reg = df;
	while (!PM->INTFLAG.bit.CKRDY);
        taskEXIT_CRITICAL();
}

/**
 * enable_per_apb_clk
 */
void enable_per_apb_clk(enum apb_bus_ins ins, unsigned int bmp)
{
	switch (ins) {
	case APB_BUS_INST_A :
		taskENTER_CRITICAL();
		PM->APBAMASK.reg |= bmp;
                taskEXIT_CRITICAL();
		break;
	case APB_BUS_INST_B :
		taskENTER_CRITICAL();
		PM->APBBMASK.reg |= bmp;
                taskEXIT_CRITICAL();
		break;
	case APB_BUS_INST_C :
		taskENTER_CRITICAL();
		PM->APBCMASK.reg |= bmp;
                taskEXIT_CRITICAL();
		break;
	default             :
		crit_err_exit(BAD_PARAMETER);
		break;
	}
}

/**
 * disable_per_apb_clk
 */
void disable_per_apb_clk(enum apb_bus_ins ins, unsigned int bmp)
{
	switch (ins) {
	case APB_BUS_INST_A :
		taskENTER_CRITICAL();
		PM->APBAMASK.reg &= ~bmp;
		taskEXIT_CRITICAL();
		break;
	case APB_BUS_INST_B :
		taskENTER_CRITICAL();
		PM->APBBMASK.reg &= ~bmp;
                taskEXIT_CRITICAL();
		break;
	case APB_BUS_INST_C :
		taskENTER_CRITICAL();
		PM->APBCMASK.reg &= ~bmp;
                taskEXIT_CRITICAL();
		break;
	default             :
		crit_err_exit(BAD_PARAMETER);
		break;
	}
}

/**
 * set_apb_clk_div
 */
void set_apb_clk_div(enum apb_bus_ins ins, int div)
{
	if (div < 1 || div > 128 || ((div & (div - 1)) != 0)) {
		crit_err_exit(BAD_PARAMETER);
	}
	if (ins != APB_BUS_INST_A && ins != APB_BUS_INST_B && ins != APB_BUS_INST_C) {
		crit_err_exit(BAD_PARAMETER);
	}
	int df = 0, b = 2;
	while (b - 1 < div) {
		df++;
		b <<= 1;
	}
        taskENTER_CRITICAL();
        PM->INTFLAG.reg = PM_INTFLAG_CKRDY;
	switch (ins) {
	case APB_BUS_INST_A :
		PM->APBASEL.reg = df;
		break;
	case APB_BUS_INST_B :
		PM->APBBSEL.reg = df;
		break;
	case APB_BUS_INST_C :
		PM->APBCSEL.reg = df;
		break;
	}
	while (!PM->INTFLAG.bit.CKRDY);
        taskEXIT_CRITICAL();
}

/**
 * reset_cause
 */
enum reset_cause reset_cause(void)
{
	if (rcause == 0xFF) {
		crit_err_exit(UNEXP_PROG_STATE);
	}
	if (rcause & PM_RCAUSE_POR) {
		return (RESET_CAUSE_POR);
	} else if (rcause & PM_RCAUSE_BOD12) {
		return (RESET_CAUSE_BOD12);
	} else if (rcause & PM_RCAUSE_BOD33) {
		return (RESET_CAUSE_BOD33);
	} else if (rcause & PM_RCAUSE_EXT) {
		return (RESET_CAUSE_EXT);
	} else if (rcause & PM_RCAUSE_WDT) {
		return (RESET_CAUSE_WDT);
	} else if (rcause & PM_RCAUSE_SYST) {
		return (RESET_CAUSE_SYST);
	} else {
		return (RESET_CAUSE_ERR);
	}
}

#if TERMOUT == 1
/**
 * reset_cause_str
 */
const char *reset_cause_str(void)
{
	return (reset_cause_ary[reset_cause()]);
}
#endif
