/*
 * dmac.c
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
#include "dmac.h"
#include <string.h>

#if DMAC_ON_CHIP == 1

__attribute__((aligned(16))) static DmacDescriptor descriptors[DMAC_CHANNELS_NUM];
__attribute__((aligned(16))) static DmacDescriptor writebacks[DMAC_CHANNELS_NUM];
static struct dmac_channel_desc channels[DMAC_CHANNELS_NUM];

/**
 * init_dmac
 */
void init_dmac(void)
{
	for (int i = 0; i < DMAC_CHANNELS_NUM; i++) {
		channels[i].id = i;
	}
        NVIC_DisableIRQ(DMAC_IRQn);
	enable_per_ahb_clk(PM_AHBMASK_DMAC);
	enable_per_apb_clk(APB_BUS_INST_B, PM_APBBMASK_DMAC);
        DMAC->CTRL.reg &= ~(DMAC_CTRL_CRCENABLE | DMAC_CTRL_DMAENABLE);
        while (DMAC->CTRL.reg & (DMAC_CTRL_CRCENABLE | DMAC_CTRL_DMAENABLE));
        DMAC->CTRL.reg = DMAC_CTRL_SWRST;
        while (DMAC->CTRL.reg & DMAC_CTRL_SWRST);
	DMAC->BASEADDR.reg = (unsigned int) descriptors;
	DMAC->WRBADDR.reg = (unsigned int) writebacks;
	DMAC->PRICTRL0.reg = DMAC_PRICTRL0_RRLVLEN3 | DMAC_PRICTRL0_RRLVLEN2 |
	                     DMAC_PRICTRL0_RRLVLEN1 | DMAC_PRICTRL0_RRLVLEN0;
        DMAC->CTRL.reg = DMAC_CTRL_LVLEN(0x0F);
        DMAC->CTRL.reg |= DMAC_CTRL_DMAENABLE;
        while (!(DMAC->CTRL.reg & DMAC_CTRL_DMAENABLE));
        NVIC_SetPriority(DMAC_IRQn, configLIBRARY_API_CALL_INTERRUPT_PRIORITY);
        NVIC_ClearPendingIRQ(DMAC_IRQn);
	NVIC_EnableIRQ(DMAC_IRQn);
}

/**
 * enable_dmac
 */
void enable_dmac(void)
{
	enable_per_ahb_clk(PM_AHBMASK_DMAC);
	enable_per_apb_clk(APB_BUS_INST_B, PM_APBBMASK_DMAC);
	DMAC->CTRL.reg |= DMAC_CTRL_DMAENABLE;
	while (!(DMAC->CTRL.reg & DMAC_CTRL_DMAENABLE));
        NVIC_ClearPendingIRQ(DMAC_IRQn);
	NVIC_EnableIRQ(DMAC_IRQn);
}

/**
 * disable_dmac
 */
void disable_dmac(void)
{
	NVIC_DisableIRQ(DMAC_IRQn);
        DMAC->CTRL.reg &= ~(DMAC_CTRL_CRCENABLE | DMAC_CTRL_DMAENABLE);
        while (DMAC->CTRL.reg & (DMAC_CTRL_CRCENABLE | DMAC_CTRL_DMAENABLE));
	disable_per_ahb_clk(PM_AHBMASK_DMAC);
	disable_per_apb_clk(APB_BUS_INST_B, PM_APBBMASK_DMAC);
}

/**
 * is_dmac_enabled
 */
boolean_t is_dmac_enabled(void)
{
	if (DMAC->CTRL.reg & DMAC_CTRL_DMAENABLE) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}

/**
 * alloc_dmac_channel
 */
dmac_channel alloc_dmac_channel(void)
{
	taskENTER_CRITICAL();
	for (int i = 0; i < DMAC_CHANNELS_NUM; i++) {
		if (channels[i].used) {
			continue;
		} else {
			channels[i].used = TRUE;
			channels[i].trans_desc = &descriptors[i];
                        taskEXIT_CRITICAL();
			return (&channels[i]);
		}
	}
        taskEXIT_CRITICAL();
	return (NULL);
}

/**
 * reset_dmac_channel
 */
void reset_dmac_channel(dmac_channel channel)
{
	taskENTER_CRITICAL();
	DMAC->CHID.reg = channel->id;
	DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
	while (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_ENABLE);
        DMAC->CHID.reg = channel->id;
	DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
        DMAC->CHID.reg = channel->id;
        while (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_SWRST);
	taskEXIT_CRITICAL();
}

/**
 * enable_dmac_transfer
 */
void enable_dmac_transfer(dmac_channel channel)
{
        taskENTER_CRITICAL();
	DMAC->CHID.reg = channel->id;
        DMAC->CHCTRLB.reg = DMAC_CHCTRLB_TRIGACT(channel->trg_action) |
	                    DMAC_CHCTRLB_TRIGSRC(channel->trg_source) |
			    DMAC_CHCTRLB_LVL(channel->prio_level);
	DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL | DMAC_CHINTENSET_TERR;
        DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
	taskEXIT_CRITICAL();
}

/**
 * disable_dmac_channel
 */
void disable_dmac_channel(dmac_channel channel)
{
	taskENTER_CRITICAL();
	disable_dmac_channel_intr(channel);
	DMAC->CHID.reg = channel->id;
	DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
	while (DMAC->CHCTRLA.reg & DMAC_CHCTRLA_ENABLE);
        taskEXIT_CRITICAL();
}

/**
 * disable_dmac_channel_intr
 */
void disable_dmac_channel_intr(dmac_channel channel)
{
	DMAC->CHID.reg = channel->id;
	DMAC->CHINTENCLR.reg = DMAC_CHINTENCLR_TCMPL | DMAC_CHINTENCLR_TERR;
	DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL | DMAC_CHINTFLAG_TERR;
}

/**
 * DMAC_Handler
 */
void DMAC_Handler(void)
{
	unsigned short pend;
	uint8_t flags;
        BaseType_t tsk_wkn = pdFALSE;

	pend = DMAC->INTPEND.reg & DMAC_INTPEND_ID_Msk;
        DMAC->CHID.reg = pend;
	flags = DMAC->CHINTFLAG.reg;
	if (flags & DMAC_CHINTFLAG_TERR) {
		tsk_wkn = (*channels[pend].hndlr)(channels[pend].dev, DMAC_TERR_INTR);
	} else if (flags & DMAC_CHINTFLAG_TCMPL) {
		tsk_wkn = (*channels[pend].hndlr)(channels[pend].dev, DMAC_TCMPL_INTR);
	}
        portEND_SWITCHING_ISR(tsk_wkn);
}
#endif
