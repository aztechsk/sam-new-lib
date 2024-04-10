/*
 * uart.c
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
#include "tools.h"
#include "hwerr.h"
#include "pm.h"
#include "gclk.h"
#include "sercom.h"
#include "dmac.h"
#include "uart.h"
#include <string.h>

#if UART_RX_CHAR == 1
static BaseType_t rx_char_hndlr(void *dev);
#endif
#if DMAC_ON_CHIP == 1 && UART_RX_CHAR == 1
static BaseType_t tx_dma_hndlr(void *dev, enum dmac_intr intr);
#endif
#if UART_RX_CHAR == 1
static unsigned short baud_reg(uart dev);
#endif

#if UART_RX_CHAR == 1
/**
 * init_uart
 */
void init_uart(uart dev, enum uart_mode m)
{
	int tmp;

	if (dev->rx_que == NULL) {
		if (NULL == (dev->rx_que = xQueueCreate(dev->rx_que_size, sizeof(uint16_t)))) {
			crit_err_exit(MALLOC_ERROR);
		}
	} else {
		crit_err_exit(UNEXP_PROG_STATE);
	}
	if (dev->sig_que == NULL) {
		if (NULL == (dev->sig_que = xQueueCreate(1, sizeof(uint8_t)))) {
			crit_err_exit(MALLOC_ERROR);
		}
	} else {
		crit_err_exit(UNEXP_PROG_STATE);
	}
	if (dev->bit_order == UART_BIT_ORDER_LSB) {
		dev->reg_ctrla |= SERCOM_USART_CTRLA_DORD;
	}
	if (dev->parity > UART_PARITY_NO) {
		dev->reg_ctrla |= SERCOM_USART_CTRLA_FORM(1);
	}
	dev->reg_ctrla |= SERCOM_USART_CTRLA_RXPO(dev->rx_pad);
	dev->reg_ctrla |= SERCOM_USART_CTRLA_TXPO(dev->tx_pad);
	switch (dev->over_sampling) {
	case UART_OVER_SAMPLING_3X  :
		if (dev->generator == UART_BAUD_GENERATOR_ARITHMETIC) {
			tmp = 4;
		} else {
			crit_err_exit(BAD_PARAMETER);
		}
		break;
	case UART_OVER_SAMPLING_8X  :
		if (dev->generator == UART_BAUD_GENERATOR_ARITHMETIC) {
			tmp = 2;
		} else {
			tmp = 3;
		}
		break;
	case UART_OVER_SAMPLING_16X :
		if (dev->generator == UART_BAUD_GENERATOR_ARITHMETIC) {
			tmp = 0;
		} else {
			tmp = 1;
		}
		break;
	}
	dev->reg_ctrla |= SERCOM_USART_CTRLA_SAMPR(tmp);
        if (dev->standby == UART_STANDBY_ACTIVE) {
		dev->reg_ctrla |= SERCOM_USART_CTRLA_RUNSTDBY;
	}
        dev->reg_ctrla |= SERCOM_USART_CTRLA_MODE_USART_INT_CLK;
	if (dev->parity == UART_PARITY_ODD) {
		dev->reg_ctrlb |= SERCOM_USART_CTRLB_PMODE;
	}
	if (dev->stop_bit == UART_2_STOP_BITS) {
		dev->reg_ctrlb |= SERCOM_USART_CTRLB_SBMODE;
	}
        dev->reg_ctrlb |= SERCOM_USART_CTRLB_CHSIZE(dev->char_size);
	dev->reg_baud = baud_reg(dev);
	switch (dev->id) {
#ifdef SERCOM0
	case ID_SERCOM0 :
		dev->mmio = (SercomUsart *) SERCOM0;
		dev->irqn = SERCOM0_IRQn;
                dev->apb_bus_ins = APB_BUS_INST_C;
		dev->apb_mask = PM_APBCMASK_SERCOM0;
                dev->clk_chn = GCLK_CLKCTRL_ID_SERCOM0_CORE_Val;
#if DMAC_ON_CHIP == 1
		dev->dmac_rx_trg_num = 0x01;
                dev->dmac_tx_trg_num = 0x02;
#endif
		break;
#endif
#ifdef SERCOM1
	case ID_SERCOM1 :
		dev->mmio = (SercomUsart *) SERCOM1;
		dev->irqn = SERCOM1_IRQn;
                dev->apb_bus_ins = APB_BUS_INST_C;
                dev->apb_mask = PM_APBCMASK_SERCOM1;
                dev->clk_chn = GCLK_CLKCTRL_ID_SERCOM1_CORE_Val;
#if DMAC_ON_CHIP == 1
		dev->dmac_rx_trg_num = 0x03;
                dev->dmac_tx_trg_num = 0x04;
#endif
		break;
#endif
#ifdef SERCOM2
	case ID_SERCOM2 :
		dev->mmio = (SercomUsart *) SERCOM2;
		dev->irqn = SERCOM2_IRQn;
                dev->apb_bus_ins = APB_BUS_INST_C;
                dev->apb_mask = PM_APBCMASK_SERCOM2;
                dev->clk_chn = GCLK_CLKCTRL_ID_SERCOM2_CORE_Val;
#if DMAC_ON_CHIP == 1
		dev->dmac_rx_trg_num = 0x05;
                dev->dmac_tx_trg_num = 0x06;
#endif
		break;
#endif
#ifdef SERCOM3
        case ID_SERCOM3 :
		dev->mmio = (SercomUsart *) SERCOM3;
		dev->irqn = SERCOM3_IRQn;
                dev->apb_bus_ins = APB_BUS_INST_C;
                dev->apb_mask = PM_APBCMASK_SERCOM3;
                dev->clk_chn = GCLK_CLKCTRL_ID_SERCOM3_CORE_Val;
#if DMAC_ON_CHIP == 1
		dev->dmac_rx_trg_num = 0x07;
                dev->dmac_tx_trg_num = 0x08;
#endif
		break;
#endif
#ifdef SERCOM4
	case ID_SERCOM4 :
		dev->mmio = (SercomUsart *) SERCOM4;
		dev->irqn = SERCOM4_IRQn;
                dev->apb_bus_ins = APB_BUS_INST_C;
                dev->apb_mask = PM_APBCMASK_SERCOM4;
                dev->clk_chn = GCLK_CLKCTRL_ID_SERCOM4_CORE_Val;
#if DMAC_ON_CHIP == 1
		dev->dmac_rx_trg_num = 0x09;
                dev->dmac_tx_trg_num = 0x0A;
#endif
		break;
#endif
#ifdef SERCOM5
        case ID_SERCOM5 :
		dev->mmio = (SercomUsart *) SERCOM5;
		dev->irqn = SERCOM5_IRQn;
                dev->apb_bus_ins = APB_BUS_INST_C;
                dev->apb_mask = PM_APBCMASK_SERCOM5;
                dev->clk_chn = GCLK_CLKCTRL_ID_SERCOM5_CORE_Val;
#if DMAC_ON_CHIP == 1
		dev->dmac_rx_trg_num = 0x0B;
                dev->dmac_tx_trg_num = 0x0C;
#endif
		break;
#endif
	default         :
		crit_err_exit(BAD_PARAMETER);
		break;
	}
#if DMAC_ON_CHIP == 1
	if (dev->dma) {
		if (NULL == (dev->channel = alloc_dmac_channel())) {
			crit_err_exit(UNEXP_PROG_STATE);
		}
		dev->channel->dev = dev;
		dev->channel->hndlr = tx_dma_hndlr;
		dev->channel->trg_action = DMAC_TRG_ACTION_BEAT;
                dev->channel->trg_source = dev->dmac_tx_trg_num;
                dev->channel->prio_level = DMAC_CHAN_PRIO_LEVEL0;
	}
#endif
	NVIC_DisableIRQ(dev->irqn);
        enable_clk_channel(dev->clk_chn, dev->clk_gen);
        enable_per_apb_clk(dev->apb_bus_ins, dev->apb_mask);
	reg_sercom_isr_clbk(dev->id, rx_char_hndlr, dev);
	dev->mmio->CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
	while (dev->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_SWRST);
        NVIC_SetPriority(dev->irqn, configLIBRARY_API_CALL_INTERRUPT_PRIORITY);
        NVIC_ClearPendingIRQ(dev->irqn);
	NVIC_EnableIRQ(dev->irqn);
	dev->mmio->BAUD.reg = dev->reg_baud;
	dev->mmio->CTRLB.reg = dev->reg_ctrlb;
        while (dev->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);
	dev->mmio->CTRLA.reg = dev->reg_ctrla;
        dev->mmio->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
	while (dev->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE);
        while (dev->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);
	dev->conf_pins(UART_CONF_PINS);
}
#endif

#if UART_RX_CHAR == 1
/**
 * enable_uart
 */
void enable_uart(void *dev)
{
	uint16_t u16;
        uint8_t u8;

        re_enable_clk_channel(((uart) dev)->clk_chn);
        enable_per_apb_clk(((uart) dev)->apb_bus_ins, ((uart) dev)->apb_mask);
	((uart) dev)->mmio->CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
	while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_SWRST);
        NVIC_ClearPendingIRQ(((uart) dev)->irqn);
	NVIC_EnableIRQ(((uart) dev)->irqn);
	((uart) dev)->mmio->BAUD.reg = ((uart) dev)->reg_baud;
	((uart) dev)->mmio->CTRLB.reg = ((uart) dev)->reg_ctrlb;
        while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);
	((uart) dev)->mmio->CTRLA.reg = ((uart) dev)->reg_ctrla;
        ((uart) dev)->mmio->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
	while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE);
        while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);
	while (pdTRUE == xQueueReceive(((uart) dev)->rx_que, &u16, 0));
	while (pdTRUE == xQueueReceive(((uart) dev)->sig_que, &u8, 0));
	((uart) dev)->conf_pins(UART_CONF_PINS);
}
#endif

#if UART_RX_CHAR == 1
/**
 * disable_uart
 */
void disable_uart(void *dev)
{
	NVIC_DisableIRQ(((uart) dev)->irqn);
	((uart) dev)->mmio->CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;
        while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_ENABLE ||
	       ((uart) dev)->mmio->CTRLA.reg & SERCOM_USART_CTRLA_ENABLE);
	((uart) dev)->conf_pins(UART_PINS_TO_PORT);
	disable_clk_channel(((uart) dev)->clk_chn);
	disable_per_apb_clk(((uart) dev)->apb_bus_ins, ((uart) dev)->apb_mask);
#if DMAC_ON_CHIP == 1
	if (((uart) dev)->dma) {
		disable_dmac_channel(((uart) dev)->channel);
	}
#endif
}
#endif

#if UART_RX_CHAR == 1
/**
 * uart_tx_buff
 */
int uart_tx_buff(void *dev, void *p_buf, int size)
{
	uint8_t er;
	int ret = 0;

	if (size < 1) {
                return (0);
        }
	((uart) dev)->mmio->CTRLB.reg |= SERCOM_USART_CTRLB_TXEN;
	while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);
	while (!(((uart) dev)->mmio->INTFLAG.reg & SERCOM_USART_INTFLAG_DRE));
#if DMAC_ON_CHIP == 1
	if (((uart) dev)->dma && size > 2) {
		DmacDescriptor *trans_desc = ((uart) dev)->channel->trans_desc;
		memset(trans_desc, 0, sizeof(trans_desc));
		trans_desc->BTCTRL.reg = DMAC_BTCTRL_STEPSIZE_X1 | DMAC_BTCTRL_STEPSEL_SRC |
		                         DMAC_BTCTRL_SRCINC | DMAC_BTCTRL_VALID;
#if UART_RX_CHAR_9_BITS == 1
		if (((uart) dev)->char_size == UART_CHAR_SIZE_9_BITS) {
			trans_desc->BTCTRL.reg |= DMAC_BTCTRL_BEATSIZE_HWORD;
		}
#endif
		trans_desc->BTCNT.reg = size;
#if UART_RX_CHAR_9_BITS == 1
		if (((uart) dev)->char_size == UART_CHAR_SIZE_9_BITS) {
			trans_desc->SRCADDR.reg = (unsigned int) p_buf + size * 2;
		} else {
#endif
			trans_desc->SRCADDR.reg = (unsigned int) p_buf + size;
#if UART_RX_CHAR_9_BITS == 1
		}
#endif
		trans_desc->DSTADDR.reg = (unsigned int) &(((uart) dev)->mmio->DATA.reg);
                trans_desc->DESCADDR.reg = 0;
		enable_dmac_transfer(((uart) dev)->channel);
                xQueueReceive(((uart) dev)->sig_que, &er, portMAX_DELAY);
		if (er) {
			reset_dmac_channel(((uart) dev)->channel);
			ret = -EDMA;
		}
	} else {
#endif
		((uart) dev)->size = --size;
#if UART_RX_CHAR_9_BITS == 1
		if (((uart) dev)->char_size == UART_CHAR_SIZE_9_BITS) {
			((uart) dev)->mmio->DATA.reg = *((uint16_t *) p_buf);
                        ((uart) dev)->p_buf = (uint16_t *) p_buf + 1;
		} else {
#endif
			((uart) dev)->mmio->DATA.reg = *((uint8_t *) p_buf);
                        ((uart) dev)->p_buf = (uint8_t *) p_buf + 1;
#if UART_RX_CHAR_9_BITS == 1
		}
#endif
                barrier();
		if (size) {
			((uart) dev)->mmio->INTENSET.reg = SERCOM_USART_INTENSET_DRE;
		} else {
			((uart) dev)->mmio->INTENSET.reg = SERCOM_USART_INTENSET_TXC;
		}
                xQueueReceive(((uart) dev)->sig_que, &er, portMAX_DELAY);
#if DMAC_ON_CHIP == 1
	}
#endif
	((uart) dev)->mmio->CTRLB.reg &= ~SERCOM_USART_CTRLB_TXEN;
	while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);
	return (ret);
}
#endif

#if UART_RX_CHAR == 1
/**
 * uart_rx_char
 */
int uart_rx_char(void *dev, void *p_char, TickType_t tmo)
{
	uint16_t d;

	if (!(((uart) dev)->mmio->CTRLB.reg & SERCOM_USART_CTRLB_RXEN)) {
		((uart) dev)->mmio->CTRLB.reg |= SERCOM_USART_CTRLB_RXEN;
                while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB);
		((uart) dev)->mmio->INTENSET.reg = SERCOM_USART_INTENSET_RXC;
	}
	if (pdFALSE == xQueueReceive(((uart) dev)->rx_que, &d, tmo)) {
#if UART_RX_CHAR_9_BITS == 1
		if (((uart) dev)->char_size == UART_CHAR_SIZE_9_BITS) {
			*((uint16_t *) p_char) = '\0';
		} else {
#endif
			*((uint8_t *) p_char) = '\0';
#if UART_RX_CHAR_9_BITS == 1
		}
#endif
		return (-ETMO);
	}
	if (d & 0x8E00) {
		if (d & 0x8000) {
#if UART_RX_CHAR_9_BITS == 1
			if (((uart) dev)->char_size == UART_CHAR_SIZE_9_BITS) {
				*((uint16_t *) p_char) = '\0';
			} else {
#endif
				*((uint8_t *) p_char) = '\0';
#if UART_RX_CHAR_9_BITS == 1
			}
#endif
			return (-EINTR);
		} else {
			d >>= 9;
#if UART_RX_CHAR_9_BITS == 1
			if (((uart) dev)->char_size == UART_CHAR_SIZE_9_BITS) {
				*((uint16_t *) p_char) = d;
			} else {
#endif
				*((uint8_t *) p_char) = d;
#if UART_RX_CHAR_9_BITS == 1
			}
#endif
			return (-ERCV);
		}
	} else {
#if UART_RX_CHAR_9_BITS == 1
		if (((uart) dev)->char_size == UART_CHAR_SIZE_9_BITS) {
			*((uint16_t *) p_char) = d & 0x01FF;
		} else {
#endif
			*((uint8_t *) p_char) = d;
#if UART_RX_CHAR_9_BITS == 1
		}
#endif
		return (0);
	}
}
#endif

#if UART_RX_CHAR == 1
/**
 * uart_intr_rx
 */
boolean_t uart_intr_rx(void *dev)
{
	uint16_t d = 0x8000;

	if (pdTRUE == xQueueSend(((uart) dev)->rx_que, &d, 0)) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}
#endif

#if UART_RX_CHAR == 1
/**
 * uart_flush_rx
 */
void uart_flush_rx(void *dev)
{
	uint16_t d;

	((uart) dev)->mmio->INTENCLR.reg = SERCOM_USART_INTENCLR_RXC;
	((uart) dev)->mmio->CTRLB.reg &= ~SERCOM_USART_CTRLB_RXEN;
	while (((uart) dev)->mmio->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_CTRLB ||
	       ((uart) dev)->mmio->CTRLB.reg & SERCOM_USART_CTRLB_RXEN);
	while (pdTRUE == xQueueReceive(((uart) dev)->rx_que, &d, 0));
}
#endif

#if UART_RX_CHAR == 1
/**
 * rx_char_hndlr
 */
static BaseType_t rx_char_hndlr(void *dev)
{
	BaseType_t tsk_wkn = pdFALSE;

	if (((uart) dev)->mmio->INTENSET.reg & SERCOM_USART_INTENSET_RXC &&
	    ((uart) dev)->mmio->INTFLAG.reg & SERCOM_USART_INTFLAG_RXC) {
		uint16_t st = ((uart) dev)->mmio->STATUS.reg;
		uint16_t d = ((uart) dev)->mmio->DATA.reg;
		d &= 0x01FF;
		if ((st &= 0x07)) {
			((uart) dev)->mmio->STATUS.reg = st;
			d |= st << 9;
		}
                BaseType_t wkn = pdFALSE;
		xQueueSendFromISR(((uart) dev)->rx_que, &d, &wkn);
		if (wkn) {
			tsk_wkn = pdTRUE;
		}
	}
	if (((uart) dev)->mmio->INTENSET.reg & SERCOM_USART_INTENSET_DRE &&
	    ((uart) dev)->mmio->INTFLAG.reg & SERCOM_USART_INTFLAG_DRE) {
#if UART_RX_CHAR_9_BITS == 1
		if (((uart) dev)->char_size == UART_CHAR_SIZE_9_BITS) {
			((uart) dev)->mmio->DATA.reg = *((uint16_t *) ((uart) dev)->p_buf);
			((uart) dev)->p_buf = (uint16_t *) ((uart) dev)->p_buf + 1;
		} else {
#endif
			((uart) dev)->mmio->DATA.reg = *((uint8_t *) ((uart) dev)->p_buf);
			((uart) dev)->p_buf = (uint8_t *) ((uart) dev)->p_buf + 1;
#if UART_RX_CHAR_9_BITS == 1
		}
#endif
		if (--((uart) dev)->size == 0) {
			((uart) dev)->mmio->INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
			((uart) dev)->mmio->INTENSET.reg = SERCOM_USART_INTENSET_TXC;
		}
	} else if (((uart) dev)->mmio->INTENSET.reg & SERCOM_USART_INTENSET_TXC &&
	           ((uart) dev)->mmio->INTFLAG.reg & SERCOM_USART_INTFLAG_TXC) {
		((uart) dev)->mmio->INTENCLR.reg = SERCOM_USART_INTENCLR_TXC;
		uint8_t er = 0;
                BaseType_t wkn = pdFALSE;
                xQueueSendFromISR(((uart) dev)->sig_que, &er, &wkn);
		if (wkn) {
			tsk_wkn = pdTRUE;
		}
	}
        return (tsk_wkn);
}
#endif

#if DMAC_ON_CHIP == 1 && UART_RX_CHAR == 1
/**
 * tx_dma_hndlr
 */
static BaseType_t tx_dma_hndlr(void *dev, enum dmac_intr intr)
{
	BaseType_t tsk_wkn = pdFALSE;
	uint8_t er = 1;

	disable_dmac_channel_intr(((uart) dev)->channel);
	if (intr == DMAC_TCMPL_INTR) {
		((uart) dev)->mmio->INTENSET.reg = SERCOM_USART_INTENSET_TXC;
	} else {
		xQueueSendFromISR(((uart) dev)->sig_que, &er, &tsk_wkn);
	}
	return (tsk_wkn);
}
#endif

#if UART_RX_CHAR == 1
/**
 * baud_reg
 */
static unsigned short baud_reg(uart dev)
{
	unsigned long long scale, tmp;
	unsigned short baud = 0;

	if (dev->over_sampling * dev->baudrate > dev->sercom_clock) {
		crit_err_exit(BAD_PARAMETER);
	}
	if (dev->generator == UART_BAUD_GENERATOR_ARITHMETIC) {
		tmp = ((dev->over_sampling * (unsigned long long) dev->baudrate) << 32);
                scale = ((unsigned long long) 1 << 32) - long_division(tmp, dev->sercom_clock);
                baud = (65536 * scale) >> 32;
	} else if (dev->generator == UART_BAUD_GENERATOR_FRACTIONAL) {
		int baud_fp, baud_int;
		tmp = ((unsigned long long) dev->sercom_clock * 1000) /
		      ((unsigned long long) dev->over_sampling * dev->baudrate);
		baud_int = tmp / 1000;
		if (baud_int > 8192) {
			crit_err_exit(BAD_PARAMETER);
		}
                baud_fp = ((tmp - (baud_int * 1000)) * 8) / 1000;
                baud = (baud_fp << 13) | baud_int;
	} else {
		crit_err_exit(BAD_PARAMETER);
	}
	return (baud);
}
#endif
