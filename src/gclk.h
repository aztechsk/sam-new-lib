/*
 * gclk_d.h
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

#ifndef GCLK_D_H
#define GCLK_D_H

/**
 * init_gclk
 */
void init_gclk(void);

/**
 * reset_gclk
 *
 * SW reset of gclk module.
 */
void reset_gclk(void);

/**
 * enable_clk_gen
 *
 * Configures and enables clock generator instance.
 *
 * @ins: Generator instance.
 * @src: Clock source.
 * @div: Clock divider.
 * @flags: GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_OE | GCLK_GENCTRL_OOV.
 */
void enable_clk_gen(int ins, int src, int div, unsigned int flags);

/**
 * disable_clk_gen
 *
 * Disables clock generator instance.
 *
 * @ins: Generator instance.
 */
void disable_clk_gen(int ins);

/**
 * re_enable_clk_gen
 *
 * Re-enables clock generator instance.
 *
 * @ins: Generator instance.
 */
void re_enable_clk_gen(int ins);

/**
 * change_clk_gen_src
 *
 * Changes clock source of clock generator.
 *
 * @ins: Generator instance.
 * @src: Clock source.
 */
void change_clk_gen_src(int ins, int src);

/**
 * enable_clk_channel
 *
 * Configures and enables clock channel.
 *
 * @chn: Clock channel.
 * @gen: Generator instance.
 */
void enable_clk_channel(int chn, int gen);

/**
 * re_enable_clk_channel
 *
 * Enables clock channel (reverts disable_clk_channel() effects).
 *
 * @chn: Clock channel.
 */
void re_enable_clk_channel(int chn);

/**
 * disable_clk_channel
 *
 * Disables clock channel.
 *
 * @chn: Clock channel.
 */
void disable_clk_channel(int chn);

/**
 * get_clk_channel_gen
 *
 * Returns generator instance number configured for clock channel chn.
 *
 * @chn: Clock channel.
 */
int get_clk_channel_gen(int chn);
#endif
