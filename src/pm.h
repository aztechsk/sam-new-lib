/*
 * pm_d.h
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

#ifndef PM_D_H
#define PM_D_H

/**
 * init_pm
 *
 * Initializes PM module and loads reset reason.
 */
void init_pm(void);

/**
 * enable_per_ahb_clk
 *
 * Enables peripheral AHB clock.
 *
 * @bmp: Peripheral bitmap.
 */
void enable_per_ahb_clk(unsigned int bmp);

/**
 * disable_per_ahb_clk
 *
 * Disables peripheral AHB clock.
 *
 * @bmp: Peripheral bitmap.
 */
void disable_per_ahb_clk(unsigned int bmp);

/**
 * set_cpu_clk_div
 *
 * Sets CPU clock divider.
 *
 * @div: CPU clock divider.
 */
void set_cpu_clk_div(int div);

enum apb_bus_ins {
	APB_BUS_INST_A,
	APB_BUS_INST_B,
	APB_BUS_INST_C
};

/**
 * enable_per_apb_clk
 *
 * Enables peripheral APB clock.
 *
 * @ins: APB bus instance.
 * @bmp: Peripheral bitmap.
 */
void enable_per_apb_clk(enum apb_bus_ins ins, unsigned int bmp);

/**
 * disable_per_apb_clk
 *
 * Disables peripheral APB clock.
 *
 * @ins: APB bus instance.
 * @bmp: Peripheral bitmap.
 */
void disable_per_apb_clk(enum apb_bus_ins ins, unsigned int bmp);

/**
 * set_apb_clk_div
 *
 * Sets APB clock divider.
 *
 * @ins: APB bus instance.
 * @div: APB clock divider.
 */
void set_apb_clk_div(enum apb_bus_ins ins, int div);

enum reset_cause {
	RESET_CAUSE_SYST,
        RESET_CAUSE_WDT,
        RESET_CAUSE_EXT,
        RESET_CAUSE_BOD33,
        RESET_CAUSE_BOD12,
        RESET_CAUSE_POR,
	RESET_CAUSE_ERR
};

/**
 * reset_cause
 *
 * Returns reason of last reset.
 */
enum reset_cause reset_cause(void);

#if TERMOUT == 1
/**
 * reset_cause_str
 *
 * Returns reason of last reset as string.
 */
const char *reset_cause_str(void);
#endif

#endif
