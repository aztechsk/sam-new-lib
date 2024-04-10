/*
 * sysctrl_d.c
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
#include "nvm.h"
#include "sysctrl.h"

/**
 * enable_osc8m
 */
void enable_osc8m(enum osc8m_mhz mhz, unsigned int flags)
{
	if (flags & ~(SYSCTRL_OSC8M_ONDEMAND | SYSCTRL_OSC8M_RUNSTDBY)) {
		crit_err_exit(BAD_PARAMETER);
	}
	disable_osc8m();
	unsigned int r = SYSCTRL->OSC8M.reg;
	r &= ~(SYSCTRL_OSC8M_PRESC_Msk | SYSCTRL_OSC8M_ONDEMAND | SYSCTRL_OSC8M_RUNSTDBY);
	r |= SYSCTRL_OSC8M_PRESC(mhz) | flags;
	SYSCTRL->OSC8M.reg = r;
	SYSCTRL->OSC8M.reg |= SYSCTRL_OSC8M_ENABLE;
	while (!(SYSCTRL->OSC8M.reg & SYSCTRL_OSC8M_ENABLE) || !(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_OSC8MRDY));
}

/**
 * disable_osc8m
 */
void disable_osc8m(void)
{
	SYSCTRL->OSC8M.reg &= ~SYSCTRL_OSC8M_ENABLE;
	while (SYSCTRL->OSC8M.reg & SYSCTRL_OSC8M_ENABLE);
}

/**
 * enable_xosc
 */
void enable_xosc(enum xosc_mode mode, enum xosc_gain gain, int st_tm, unsigned int flags)
{
	if (flags & ~(SYSCTRL_XOSC_ONDEMAND | SYSCTRL_XOSC_RUNSTDBY)) {
		crit_err_exit(BAD_PARAMETER);
	}
	if (st_tm > 15) {
		crit_err_exit(BAD_PARAMETER);
	}
	unsigned int r = SYSCTRL_XOSC_STARTUP(st_tm);
	if (mode == XOSC_XTAL) {
		if (gain != XOSC_GAIN_AUTO) {
			r |= SYSCTRL_XOSC_GAIN(gain);
		}
		r |= SYSCTRL_XOSC_XTALEN;
	}
	if (flags & SYSCTRL_XOSC_RUNSTDBY) {
		r |= SYSCTRL_XOSC_RUNSTDBY;
	}
	SYSCTRL->XOSC.reg = r;
	SYSCTRL->XOSC.reg |= SYSCTRL_XOSC_ENABLE;
	while (!(SYSCTRL->XOSC.reg & SYSCTRL_XOSC_ENABLE) || !(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_XOSCRDY));
	if (mode == XOSC_XTAL && gain == XOSC_GAIN_AUTO) {
		SYSCTRL->XOSC.reg |= SYSCTRL_XOSC_AMPGC;
	}
	if (flags & SYSCTRL_XOSC_ONDEMAND) {
		SYSCTRL->XOSC.reg |= SYSCTRL_XOSC_ONDEMAND;
	}
}

/**
 * disable_xosc
 */
void disable_xosc(void)
{
	SYSCTRL->XOSC.reg &= ~SYSCTRL_XOSC_ENABLE;
	while (SYSCTRL->XOSC.reg & SYSCTRL_XOSC_ENABLE);
}

/**
 * enable_xosc32k
 */
void enable_xosc32k(enum xosc_mode mode, int st_tm, unsigned int flags)
{
	if (flags & ~(SYSCTRL_XOSC32K_ONDEMAND | SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_AAMPEN)) {
		crit_err_exit(BAD_PARAMETER);
	}
	if (st_tm > 7) {
		crit_err_exit(BAD_PARAMETER);
	}
        unsigned int r = SYSCTRL_XOSC32K_STARTUP(st_tm);
	r |= flags | SYSCTRL_XOSC32K_EN32K;
	if (mode == XOSC_XTAL) {
		r |= SYSCTRL_XOSC32K_XTALEN;
	}
	r &= ~SYSCTRL_XOSC32K_ONDEMAND;
	SYSCTRL->XOSC32K.reg = r;
	SYSCTRL->XOSC32K.reg |= SYSCTRL_XOSC32K_ENABLE;
	while (!(SYSCTRL->XOSC32K.reg & SYSCTRL_XOSC32K_ENABLE) || !(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_XOSC32KRDY));
	if (flags & SYSCTRL_XOSC32K_ONDEMAND) {
		SYSCTRL->XOSC32K.reg |= SYSCTRL_XOSC32K_ONDEMAND;
	}
}

/**
 * disable_xosc32k
 */
void disable_xosc32k(void)
{
	SYSCTRL->XOSC32K.reg &= ~SYSCTRL_XOSC32K_ENABLE;
	while (SYSCTRL->XOSC32K.reg & SYSCTRL_XOSC32K_ENABLE);
}

/**
 * enable_osc32k
 */
void enable_osc32k(int st_tm, unsigned int flags)
{
	if (flags & ~(SYSCTRL_OSC32K_ONDEMAND | SYSCTRL_OSC32K_RUNSTDBY)) {
		crit_err_exit(BAD_PARAMETER);
	}
	if (st_tm > 7) {
		crit_err_exit(BAD_PARAMETER);
	}
        SYSCTRL->OSC32K.reg = SYSCTRL_OSC32K_CALIB(nvm_calib_row.osc32k_cal);
	SYSCTRL->OSC32K.reg |= SYSCTRL_OSC32K_STARTUP(st_tm) | flags | SYSCTRL_OSC32K_EN32K;
	SYSCTRL->OSC32K.reg |= SYSCTRL_OSC32K_ENABLE;
	while (!(SYSCTRL->OSC32K.reg & SYSCTRL_OSC32K_ENABLE) || !(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_OSC32KRDY));
}

/**
 * disable_osc32k
 */
void disable_osc32k(void)
{
	SYSCTRL->OSC32K.reg &= ~SYSCTRL_OSC32K_ENABLE;
	while (SYSCTRL->OSC32K.reg & SYSCTRL_OSC32K_ENABLE);
}

/**
 * enable_dfll48m_openloop
 */
void enable_dfll48m_openloop(unsigned int flags)
{
	if (flags & ~(SYSCTRL_DFLLCTRL_ONDEMAND | SYSCTRL_DFLLCTRL_RUNSTDBY)) {
		crit_err_exit(BAD_PARAMETER);
	}
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
	SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
	SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(nvm_calib_row.dfll48m_coarse_cal) | SYSCTRL_DFLLVAL_FINE(511);
        SYSCTRL->DFLLMUL.reg = 0;
	SYSCTRL->DFLLCTRL.reg |= flags;
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
}

/**
 * enable_dfll48m_closedloop
 */
void enable_dfll48m_closedloop(int refmul, unsigned int flags)
{
	struct {
		unsigned int ctrl, val, mul;
	} cfg;

	if (flags & ~(SYSCTRL_DFLLCTRL_QLDIS | SYSCTRL_DFLLCTRL_CCDIS | SYSCTRL_DFLLCTRL_ONDEMAND |
	              SYSCTRL_DFLLCTRL_RUNSTDBY | SYSCTRL_DFLLCTRL_LLAW)) {
		crit_err_exit(BAD_PARAMETER);
	}
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
	SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
	cfg.ctrl = flags | SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_ENABLE;
	cfg.ctrl &= ~SYSCTRL_DFLLCTRL_ONDEMAND;
	cfg.val = SYSCTRL_DFLLVAL_COARSE(nvm_calib_row.dfll48m_coarse_cal) | SYSCTRL_DFLLVAL_FINE(511);
	cfg.mul = SYSCTRL_DFLLMUL_CSTEP(7) | SYSCTRL_DFLLMUL_FSTEP(63) | SYSCTRL_DFLLMUL_MUL(refmul);
	SYSCTRL->DFLLMUL.reg = cfg.mul;
	SYSCTRL->DFLLVAL.reg = cfg.val;
        SYSCTRL->DFLLCTRL.reg = 0;
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
	SYSCTRL->DFLLCTRL.reg = cfg.ctrl;
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
	if (flags & SYSCTRL_DFLLCTRL_ONDEMAND) {
		SYSCTRL->DFLLCTRL.reg |= SYSCTRL_DFLLCTRL_ONDEMAND;
		while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
	}
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLLCKC) || !(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLLCKF));
}

/**
 * disable_dfll48m
 */
void disable_dfll48m(void)
{
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));
	SYSCTRL->DFLLCTRL.reg &= ~SYSCTRL_DFLLCTRL_ENABLE;
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) || SYSCTRL->DFLLCTRL.reg & SYSCTRL_DFLLCTRL_ENABLE);
}

/**
 * enable_bod33
 */
void enable_bod33(int lev, enum bod33_action act, unsigned int flags)
{
	if (flags & ~(SYSCTRL_BOD33_RUNSTDBY | SYSCTRL_BOD33_HYST)) {
		crit_err_exit(BAD_PARAMETER);
	}
	disable_bod33();
	SYSCTRL->BOD33.reg = SYSCTRL_BOD33_LEVEL(lev) | SYSCTRL_BOD33_ACTION(act) | flags | SYSCTRL_BOD33_ENABLE;
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_B33SRDY) || !(SYSCTRL->BOD33.reg & SYSCTRL_BOD33_ENABLE));
}

/**
 * enable_bod33_low_pow
 */
void enable_bod33_low_pow(int lev, enum bod33_action act, int psel, unsigned int flags)
{
	if (flags & ~(SYSCTRL_BOD33_RUNSTDBY | SYSCTRL_BOD33_HYST)) {
		crit_err_exit(BAD_PARAMETER);
	}
	disable_bod33();
	SYSCTRL->BOD33.reg = SYSCTRL_BOD33_LEVEL(lev) | SYSCTRL_BOD33_PSEL(psel) |
	                     SYSCTRL_BOD33_CEN | SYSCTRL_BOD33_MODE |
                             SYSCTRL_BOD33_ACTION(act) | flags | SYSCTRL_BOD33_ENABLE;
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_B33SRDY) || !(SYSCTRL->BOD33.reg & SYSCTRL_BOD33_ENABLE));
}

/**
 * disable_bod33
 */
void disable_bod33(void)
{
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_B33SRDY));
	SYSCTRL->BOD33.reg &= ~(SYSCTRL_BOD33_CEN | SYSCTRL_BOD33_ENABLE);
	while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_B33SRDY) || SYSCTRL->BOD33.reg & SYSCTRL_BOD33_ENABLE);
}

/**
 * vreg_control
 */
void vreg_control(unsigned int flags)
{
	if (flags & ~(SYSCTRL_VREG_FORCELDO | SYSCTRL_VREG_RUNSTDBY)) {
		crit_err_exit(BAD_PARAMETER);
	}
	SYSCTRL->VREG.reg = flags | (1 << 1);
}

/**
 * vref_control
 */
void vref_control(unsigned int flags)
{
	unsigned int r = SYSCTRL->VREF.reg;

	if (flags & ~(SYSCTRL_VREF_BGOUTEN | SYSCTRL_VREF_TSEN)) {
		crit_err_exit(BAD_PARAMETER);
	}
        r &= ~(SYSCTRL_VREF_BGOUTEN | SYSCTRL_VREF_TSEN);
	SYSCTRL->VREF.reg = r | flags;
}
