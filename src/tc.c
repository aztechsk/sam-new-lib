/*
 * tc.c
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
#include "tcisr.h"
#include "tc.h"

#if TC_TIMER == 1

extern inline void tc_enable_mc0_intr(tc_timer dev);
extern inline void tc_enable_mc1_intr(tc_timer dev);
extern inline void tc_enable_err_intr(tc_timer dev);
extern inline void tc_enable_ovf_intr(tc_timer dev);
extern inline void tc_disable_mc0_intr(tc_timer dev);
extern inline void tc_disable_mc1_intr(tc_timer dev);
extern inline void tc_disable_err_intr(tc_timer dev);
extern inline void tc_disable_ovf_intr(tc_timer dev);
extern inline void tc_clear_mc0_intr(tc_timer dev);
extern inline void tc_clear_mc1_intr(tc_timer dev);
extern inline void tc_clear_err_intr(tc_timer dev);
extern inline void tc_clear_ovf_intr(tc_timer dev);
extern inline void tc_disable_all_intr(tc_timer dev);

static inline boolean_t sync(tc_timer dev);

/**
 * init_tc
 */
void init_tc(tc_timer dev)
{
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		switch (dev->id) {
		case ID_TC4 :
			dev->mmio = TC4;
                        dev->irqn = TC4_IRQn;
                        dev->apb_bus_ins = APB_BUS_INST_C;
                        dev->apb_mask = PM_APBCMASK_TC4;
                        dev->apb_bus_ins_2 = APB_BUS_INST_C;
                        dev->apb_mask_2 = PM_APBCMASK_TC5;
                        dev->clk_chn = GCLK_CLKCTRL_ID_TC4_TC5_Val;
			break;
#ifdef TC6
		case ID_TC6 :
			dev->mmio = TC6;
                        dev->irqn = TC6_IRQn;
                        dev->apb_bus_ins = APB_BUS_INST_C;
                        dev->apb_mask = PM_APBCMASK_TC6;
                        dev->apb_bus_ins_2 = APB_BUS_INST_C;
                        dev->apb_mask_2 = PM_APBCMASK_TC7;
                        dev->clk_chn = GCLK_CLKCTRL_ID_TC6_TC7_Val;
			break;
#endif
		default     :
			crit_err_exit(BAD_PARAMETER);
			break;
		}
	} else {
		switch (dev->id) {
		case ID_TC3 :
			dev->mmio = TC3;
                        dev->irqn = TC3_IRQn;
                        dev->apb_bus_ins = APB_BUS_INST_C;
                        dev->apb_mask = PM_APBCMASK_TC3;
                        dev->clk_chn = GCLK_CLKCTRL_ID_TCC2_TC3_Val;
			break;
		case ID_TC4 :
			dev->mmio = TC4;
                        dev->irqn = TC4_IRQn;
                        dev->apb_bus_ins = APB_BUS_INST_C;
                        dev->apb_mask = PM_APBCMASK_TC4;
                        dev->clk_chn = GCLK_CLKCTRL_ID_TC4_TC5_Val;
			break;
		case ID_TC5 :
			dev->mmio = TC5;
                        dev->irqn = TC5_IRQn;
                        dev->apb_bus_ins = APB_BUS_INST_C;
                        dev->apb_mask = PM_APBCMASK_TC5;
                        dev->clk_chn = GCLK_CLKCTRL_ID_TC4_TC5_Val;
			break;
#ifdef TC6
		case ID_TC6 :
			dev->mmio = TC6;
                        dev->irqn = TC6_IRQn;
                        dev->apb_bus_ins = APB_BUS_INST_C;
                        dev->apb_mask = PM_APBCMASK_TC6;
                        dev->clk_chn = GCLK_CLKCTRL_ID_TC6_TC7_Val;
			break;
#endif
#ifdef TC7
		case ID_TC7 :
			dev->mmio = TC7;
                        dev->irqn = TC7_IRQn;
                        dev->apb_bus_ins = APB_BUS_INST_C;
                        dev->apb_mask = PM_APBCMASK_TC7;
                        dev->clk_chn = GCLK_CLKCTRL_ID_TC6_TC7_Val;
			break;
#endif
		default     :
			crit_err_exit(BAD_PARAMETER);
			break;
		}
	}
	dev->reg_ctrla = TC_CTRLA_PRESCSYNC(dev->cnt_sync) | ((dev->runstdby) ? TC_CTRLA_RUNSTDBY : 0) |
	                 TC_CTRLA_PRESCALER(dev->prescaler) | TC_CTRLA_WAVEGEN(dev->wavegen) |
                         TC_CTRLA_MODE(dev->cnt_size);
	dev->reg_ctrlc = ((dev->chan_1_capt) ? TC_CTRLC_CPTEN1 : 0) | ((dev->chan_0_capt) ? TC_CTRLC_CPTEN0 : 0) |
			  ((dev->chan_1_wave_inv) ? TC_CTRLC_INVEN1 : 0) | ((dev->chan_0_wave_inv) ? TC_CTRLC_INVEN0 : 0);
	NVIC_DisableIRQ(dev->irqn);
        enable_clk_channel(dev->clk_chn, dev->clk_gen);
        enable_per_apb_clk(dev->apb_bus_ins, dev->apb_mask);
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		enable_per_apb_clk(dev->apb_bus_ins_2, dev->apb_mask_2);
	}
        reg_tc_isr_clbk(dev->id, dev->isr_clbk, dev);
	dev->mmio->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
	while (!sync(dev) || dev->mmio->COUNT16.CTRLA.reg & TC_CTRLA_SWRST);
	dev->mmio->COUNT16.CTRLC.reg = dev->reg_ctrlc;
        while (!sync(dev));
	dev->mmio->COUNT16.CTRLBCLR.reg = TC_CTRLBCLR_ONESHOT | TC_CTRLBCLR_DIR;
	while (!sync(dev));
	uint8_t bset = ((dev->oneshot_mode) ? TC_CTRLBSET_ONESHOT : 0) |
	               ((dev->direction == TC_DIRECTION_DOWN) ? TC_CTRLBSET_DIR : 0);
	if (bset) {
		dev->mmio->COUNT16.CTRLBSET.reg = bset;
                while (!sync(dev));
	}
	dev->mmio->COUNT16.EVCTRL.reg = TC_EVCTRL_EVACT(dev->capmode);
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		dev->mmio->COUNT32.CC[0].reg = dev->cc0;
		dev->mmio->COUNT32.CC[1].reg = dev->cc1;
	} else {
		dev->mmio->COUNT16.CC[0].reg = dev->cc0;
		dev->mmio->COUNT16.CC[1].reg = dev->cc1;
	}
        while (!sync(dev));
        NVIC_SetPriority(dev->irqn, configLIBRARY_API_CALL_INTERRUPT_PRIORITY);
        NVIC_ClearPendingIRQ(dev->irqn);
	NVIC_EnableIRQ(dev->irqn);
	if (dev->int_enable_mask) {
		dev->mmio->COUNT16.INTENSET.reg = dev->int_enable_mask;
	}
	if (dev->conf_pins) {
		dev->conf_pins(TC_CONF_PINS);
	}
	dev->mmio->COUNT16.CTRLA.reg = dev->reg_ctrla | TC_CTRLA_ENABLE;
	while (!sync(dev));
}

/**
 * enable_tc
 */
void enable_tc(tc_timer dev)
{
        re_enable_clk_channel(dev->clk_chn);
        enable_per_apb_clk(dev->apb_bus_ins, dev->apb_mask);
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		enable_per_apb_clk(dev->apb_bus_ins_2, dev->apb_mask_2);
	}
        NVIC_ClearPendingIRQ(dev->irqn);
	NVIC_EnableIRQ(dev->irqn);
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		dev->mmio->COUNT32.COUNT.reg = 0;
	} else {
		dev->mmio->COUNT16.COUNT.reg = 0;
	}
        while (!sync(dev));
	if (dev->conf_pins) {
		dev->conf_pins(TC_CONF_PINS);
	}
	dev->mmio->COUNT16.CTRLA.reg = dev->reg_ctrla | TC_CTRLA_ENABLE;
	while (!sync(dev));
}

/**
 * disable_tc
 */
void disable_tc(tc_timer dev)
{
	NVIC_DisableIRQ(dev->irqn);
	tc_disable_all_intr(dev);
	tc_stop(dev);
        while (!sync(dev));
	dev->mmio->COUNT16.CTRLA.reg = 0;
        while (!sync(dev));
	if (dev->conf_pins) {
		dev->conf_pins(TC_PINS_TO_PORT);
	}
        disable_per_apb_clk(dev->apb_bus_ins, dev->apb_mask);
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		disable_per_apb_clk(dev->apb_bus_ins_2, dev->apb_mask_2);
	}
}

/**
 * disable_tc_clk_chn
 */
void disable_tc_clk_chn(tc_timer dev)
{
	disable_clk_channel(dev->clk_chn);
}

/**
 * tc_trigger
 */
void tc_trigger(tc_timer dev)
{
	dev->mmio->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_RETRIGGER;
        while (!sync(dev));
}

/**
 * tc_stop
 */
void tc_stop(tc_timer dev)
{
	dev->mmio->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_STOP;
        while (!sync(dev));
}

/**
 * tc_set_direction
 */
void tc_set_direction(tc_timer dev, enum tc_direction dir)
{
	if (dir == TC_DIRECTION_DOWN) {
		dev->mmio->COUNT16.CTRLBSET.reg = TC_CTRLBSET_DIR;
	} else {
		dev->mmio->COUNT16.CTRLBCLR.reg = TC_CTRLBCLR_DIR;
	}
        while (!sync(dev));
}

/**
 * tc_set_cc0
 */
void tc_set_cc0(tc_timer dev, unsigned int v)
{
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		dev->mmio->COUNT32.CC[0].reg = v;
	} else {
		dev->mmio->COUNT16.CC[0].reg = v;
	}
        while (!sync(dev));
}

/**
 * tc_set_cc1
 */
void tc_set_cc1(tc_timer dev, unsigned int v)
{
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		dev->mmio->COUNT32.CC[1].reg = v;
	} else {
		dev->mmio->COUNT16.CC[1].reg = v;
	}
        while (!sync(dev));
}

/**
 * tc_set_cnt
 */
void tc_set_cnt(tc_timer dev, unsigned int v)
{
	if (dev->cnt_size == TC_CNT_SIZE_32_BIT) {
		dev->mmio->COUNT32.COUNT.reg = v;
	} else {
		dev->mmio->COUNT16.COUNT.reg = v;
	}
        while (!sync(dev));
}

/**
 * sync
 */
static inline boolean_t sync(tc_timer dev)
{
	return ((dev->mmio->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY) ? FALSE : TRUE);
}
#endif
