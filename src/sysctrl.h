/*
 * sysctrl_d.h
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

#ifndef SYSCTRL_D_H
#define SYSCTRL_D_H

enum osc8m_mhz {
	OSC8M_1_MHZ = 3,
	OSC8M_2_MHZ = 2,
	OSC8M_4_MHZ = 1,
	OSC8M_8_MHZ = 0
};

/**
 * enable_osc8m
 *
 * Configures and enables OSC8M.
 *
 * @mhz: Frequency.
 * @flags: SYSCTRL_OSC8M_ONDEMAND | SYSCTRL_OSC8M_RUNSTDBY.
 */
void enable_osc8m(enum osc8m_mhz mhz, unsigned int flags);

/**
 * disable_osc8m
 *
 * Disables OSC8M.
 */
void disable_osc8m(void);

enum xosc_gain {
	XOSC_GAIN_2MHZ  = 0,
	XOSC_GAIN_4MHZ  = 1,
	XOSC_GAIN_8MHZ  = 2,
	XOSC_GAIN_16MHZ = 3,
	XOSC_GAIN_30MHZ = 4,
	XOSC_GAIN_AUTO  = 255
};

enum xosc_mode {
	XOSC_XTAL,
	XOSC_EXT_CLK
};

/**
 * enable_xosc
 *
 * Configures and enables XOSC.
 *
 * @mode: Mode.
 * @gain: Gain.
 * @st_tm: Start time (2 ^ st_tm of OSCULP32K cycles).
 * @flags: SYSCTRL_XOSC_ONDEMAND | SYSCTRL_XOSC_RUNSTDBY.
 */
void enable_xosc(enum xosc_mode mode, enum xosc_gain gain, int st_tm, unsigned int flags);

/**
 * disable_xosc
 *
 * Disables XOSC.
 */
void disable_xosc(void);

/**
 * enable_xosc32k
 *
 * Configures and enables XOSC32K.
 *
 * @mode: Mode.
 * @st_tm: Start time (SYSCTRL_XOSC32K_STARTUP bits).
 * @flags: SYSCTRL_XOSC32K_ONDEMAND | SYSCTRL_XOSC32K_RUNSTDBY | SYSCTRL_XOSC32K_AAMPEN.
 *   SAMD21 errata: SYSCTRL_XOSC32K_AAMPEN - not working.
 */
void enable_xosc32k(enum xosc_mode mode, int st_tm, unsigned int flags);

/**
 * disable_xosc32k
 *
 * Disables XOSC32K.
 */
void disable_xosc32k(void);

/**
 * enable_osc32k
 *
 * Configures and enables OSC32K.
 *
 * @stm: Start time (SYSCTRL_OSC32K_STARTUP bits).
 * @flags: SYSCTRL_OSC32K_ONDEMAND | SYSCTRL_OSC32K_RUNSTDBY.
 */
void enable_osc32k(int st_tm, unsigned int flags);

/**
 * disable_osc32k
 *
 * Disables OSC32K.
 */
void disable_osc32k(void);

/**
 * enable_dfll48m_openloop
 *
 * Configures and enables DFLL48M in openloop mode.
 *
 * @flags: SYSCTRL_DFLLCTRL_ONDEMAND | SYSCTRL_DFLLCTRL_RUNSTDBY.
 */
void enable_dfll48m_openloop(unsigned int flags);

/**
 * enable_dfll48m_closedloop
 *
 * Configures and enables DFLL48M in closedloop mode.
 *
 * @refmul: Reference frequency multiplier.
 * @flags: SYSCTRL_DFLLCTRL_QLDIS | SYSCTRL_DFLLCTRL_CCDIS | SYSCTRL_DFLLCTRL_ONDEMAND | SYSCTRL_DFLLCTRL_RUNSTDBY |
 *         SYSCTRL_DFLLCTRL_LLAW.
 *  SYSCTRL_DFLLCTRL_QLDIS - quick lock is disabled.
 *  SYSCTRL_DFLLCTRL_CCDIS - chill cycle is disabled.
 *  SYSCTRL_DFLLCTRL_LLAW - locks will be lost after waking up from sleep modes if the DFLL48M clock has been stopped.
 */
void enable_dfll48m_closedloop(int refmul, unsigned int flags);

/**
 * disable_dfll48m
 *
 * Disables DFLL48M.
 */
void disable_dfll48m(void);

enum bod33_action {
	BOD33_ACTION_NONE,
	BOD33_ACTION_RESET,
        BOD33_ACTION_INTR
};

/**
 * enable_bod33
 *
 * Configures and enables BOD33 in standard continuous mode.
 *
 * @lev: Threshold level.
 * @act: Threshold action.
 * @flags: SYSCTRL_BOD33_RUNSTDBY | SYSCTRL_BOD33_HYST.
 */
void enable_bod33(int lev, enum bod33_action act, unsigned int flags);

/**
 * enable_bod33_low_pow
 *
 * Configures and enables BOD33 in low power sampling mode.
 *
 * @lev: Threshold level.
 * @act: Threshold action.
 * @psel: Prescaler of 1kHz slow clock (1024 / (2 ^ (psel + 1))).
 * @flags: SYSCTRL_BOD33_RUNSTDBY | SYSCTRL_BOD33_HYST.
 */
void enable_bod33_low_pow(int lev, enum bod33_action act, int psel, unsigned int flags);

/**
 * disable_bod33
 *
 * Disables BOD33.
 */
void disable_bod33(void);

/**
 * vreg_control
 *
 * Configures LDO voltage regulator.
 *
 * @flags: SYSCTRL_VREG_FORCELDO | SYSCTRL_VREG_RUNSTDBY.
 *  SYSCTRL_VREG_FORCELDO - LDO is in low-power and high-drive configuration during standby sleep.
 *  SYSCTRL_VREG_RUNSTDBY - LDO is in normal configuration during standby sleep.
 */
void vreg_control(unsigned int flags);

/**
 * vref_control
 *
 * Configures voltage reference system.
 *
 * @flags: SYSCTRL_VREF_BGOUTEN | SYSCTRL_VREF_TSEN.
 *  SYSCTRL_VREF_BGOUTEN - bandgap voltage routed to ADC.
 *  SYSCTRL_VREF_TSEN - temperature sensor enabled and routed to ADC.
 */
void vref_control(unsigned int flags);
#endif
