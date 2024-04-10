/*
 * tc.h
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

#ifndef TC_H
#define TC_H

#ifndef TC_TIMER
 #define TC_TIMER 0
#endif

#if TC_TIMER == 1

#include "pm.h"

enum tc_cnt_size {
	TC_CNT_SIZE_16_BIT,
        TC_CNT_SIZE_8_BIT,
	TC_CNT_SIZE_32_BIT
};

enum tc_cnt_sync {
	TC_CNT_SYNC_GCLK,
        TC_CNT_SYNC_PRESC,
        TC_CNT_SYNC_RESYNC
};

enum tc_prescaler {
	TC_PRESCALER_DIV1,
	TC_PRESCALER_DIV2,
        TC_PRESCALER_DIV4,
        TC_PRESCALER_DIV8,
        TC_PRESCALER_DIV16,
        TC_PRESCALER_DIV64,
        TC_PRESCALER_DIV256,
        TC_PRESCALER_DIV1024
};

enum tc_wavegen {
	TC_WAVEGEN_NFRQ,
        TC_WAVEGEN_MFRQ,
        TC_WAVEGEN_NPWM,
        TC_WAVEGEN_MPWM
};

enum tc_direction {
	TC_DIRECTION_UP,
	TC_DIRECTION_DOWN
};

enum capture_mode {
	CAPTURE_MODE_OFF,
	CAPTURE_MODE_PPW = 5,
        CAPTURE_MODE_PWP = 6
};

enum tc_conf_pins_cmd {
	TC_CONF_PINS,
	TC_PINS_TO_PORT
};

typedef struct tc_timer_dsc *tc_timer;

struct tc_timer_dsc {
	int id; // <SetIt>
	int clk_gen; // <SetIt> - GCLK instance for TC clock channel.
	int clock_freq; // <SetIt> - GCLK frequency.
        enum tc_cnt_size cnt_size; // <SetIt>
        enum tc_cnt_sync cnt_sync; // <SetIt>
	enum tc_prescaler prescaler; // <SetIt>
        enum tc_wavegen wavegen; // <SetIt>
	enum tc_direction direction; // <SetIt>
	boolean_t oneshot_mode; // <SetIt>
	unsigned int cc0; // <SetIt>
	unsigned int cc1; // <SetIt>
	boolean_t chan_0_capt; // <SetIt>
	boolean_t chan_1_capt; // <SetIt>
	boolean_t chan_0_wave_inv; // <SetIt>
	boolean_t chan_1_wave_inv; // <SetIt>
        enum capture_mode capmode; // <SetIt>
	uint8_t int_enable_mask; // <SetIt>
        BaseType_t (*isr_clbk)(void *); // <SetIt>
	void (*conf_pins)(enum tc_conf_pins_cmd); // <SetIt> - NULL if io pins not used.
	boolean_t runstdby; // <SetIt>
        Tc *mmio;
        enum apb_bus_ins apb_bus_ins;
	unsigned int apb_mask;
        enum apb_bus_ins apb_bus_ins_2;
	unsigned int apb_mask_2;
        IRQn_Type irqn;
        int clk_chn;
        unsigned short reg_ctrla;
        unsigned short reg_ctrlc;
};

/**
 * init_tc
 *
 * Configure and enable TC instance. Timer starts counting after initialization.
 *
 * @dev: TC instance.
 */
void init_tc(tc_timer dev);

/**
 * enable_tc
 *
 * Enable TC instance (revert disable_tc() function effects).
 * Timer starts counting after enabled.
 *
 * @dev: TC instance.
 */
void enable_tc(tc_timer dev);

/**
 * disable_tc
 *
 * Disable TC instance.
 *
 * @dev: TC instance.
 */
void disable_tc(tc_timer dev);

/**
 * disable_tc_clk_chn
 *
 * Disable TC clock channel (GCLK).
 */
void disable_tc_clk_chn(tc_timer dev);

/**
 * tc_trigger
 *
 * Trigger.
 */
void tc_trigger(tc_timer dev);

/**
 * tc_stop
 *
 * Stop.
 */
void tc_stop(tc_timer dev);

/**
 * tc_set_direction
 *
 * Set counting direction.
 */
void tc_set_direction(tc_timer dev, enum tc_direction dir);

/**
 * tc_set_cc0
 *
 * Set CC0 register.
 */
void tc_set_cc0(tc_timer dev, unsigned int v);

/**
 * tc_set_cc1
 *
 * Set CC1 register.
 */
void tc_set_cc1(tc_timer dev, unsigned int v);

/**
 * tc_set_cnt
 *
 * Set counter value.
 */
void tc_set_cnt(tc_timer dev, unsigned int v);

/**
 * tc_enable_mc0_intr
 */
inline void tc_enable_mc0_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
}

/**
 * tc_enable_mc1_intr
 */
inline void tc_enable_mc1_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENSET.reg = TC_INTENSET_MC1;
}

/**
 * tc_enable_err_intr
 */
inline void tc_enable_err_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENSET.reg = TC_INTENSET_ERR;
}

/**
 * tc_enable_ovf_intr
 */
inline void tc_enable_ovf_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENSET.reg = TC_INTENSET_OVF;
}

/**
 * tc_disable_mc0_intr
 */
inline void tc_disable_mc0_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENCLR.reg = TC_INTENCLR_MC0;
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
}

/**
 * tc_disable_mc1_intr
 */
inline void tc_disable_mc1_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENCLR.reg = TC_INTENCLR_MC1;
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_MC1;
}

/**
 * tc_disable_err_intr
 */
inline void tc_disable_err_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENCLR.reg = TC_INTENCLR_ERR;
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_ERR;
}

/**
 * tc_disable_ovf_intr
 */
inline void tc_disable_ovf_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENCLR.reg = TC_INTENCLR_OVF;
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF;
}

/**
 * tc_clear_mc0_intr
 */
inline void tc_clear_mc0_intr(tc_timer dev)
{
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
}

/**
 * tc_clear_mc1_intr
 */
inline void tc_clear_mc1_intr(tc_timer dev)
{
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_MC1;
}

/**
 * tc_clear_err_intr
 */
inline void tc_clear_err_intr(tc_timer dev)
{
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_ERR;
}

/**
 * tc_clear_ovf_intr
 */
inline void tc_clear_ovf_intr(tc_timer dev)
{
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF;
}

/**
 * tc_disable_all_intr
 */
inline void tc_disable_all_intr(tc_timer dev)
{
	dev->mmio->COUNT16.INTENCLR.reg = TC_INTENCLR_MC0 | TC_INTENCLR_MC1 | TC_INTENCLR_ERR | TC_INTENCLR_OVF;
        dev->mmio->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0 | TC_INTFLAG_MC1 | TC_INTFLAG_ERR | TC_INTFLAG_OVF;
}
#endif

#endif
