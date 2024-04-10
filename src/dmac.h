/*
 * dmac.h
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

#ifndef DMAC_H
#define DMAC_H

#if DMAC_ON_CHIP == 1

enum dmac_intr {
	DMAC_SUSP_INTR,
        DMAC_TCMPL_INTR,
	DMAC_TERR_INTR
};

enum dmac_trg_action {
	DMAC_TRG_ACTION_BLOCK,
	DMAC_TRG_ACTION_BEAT  = 2,
        DMAC_TRG_ACTION_TRANS = 3
};

enum dmac_chan_prio_level {
	DMAC_CHAN_PRIO_LEVEL0,
	DMAC_CHAN_PRIO_LEVEL1,
        DMAC_CHAN_PRIO_LEVEL2,
        DMAC_CHAN_PRIO_LEVEL3
};

struct dmac_channel_desc {
	void *dev; // <SetIt>
	BaseType_t (*hndlr)(void *dev, enum dmac_intr intr); // <SetIt>
        enum dmac_trg_action trg_action; // <SetIt>
	int trg_source; // <SetIt>
	enum dmac_chan_prio_level prio_level; // <SetIt>
        DmacDescriptor *trans_desc; // <SetIt>
	boolean_t used;
	int id;
};

typedef struct dmac_channel_desc *dmac_channel;

/**
 * init_dmac
 */
void init_dmac(void);

/**
 * enable_dmac
 */
void enable_dmac(void);

/**
 * disable_dmac
 */
void disable_dmac(void);

/**
 * is_dmac_enabled
 */
boolean_t is_dmac_enabled(void);

/**
 * alloc_dmac_channel
 */
dmac_channel alloc_dmac_channel(void);

/**
 * reset_dmac_channel
 */
void reset_dmac_channel(dmac_channel channel);

/**
 * enable_dmac_transfer
 */
void enable_dmac_transfer(dmac_channel channel);

/**
 * disable_dmac_channel
 */
void disable_dmac_channel(dmac_channel channel);

/**
 * disable_dmac_channel_intr
 */
void disable_dmac_channel_intr(dmac_channel channel);
#endif

#endif
