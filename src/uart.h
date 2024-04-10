/*
 * uart.h
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

#ifndef UART_H
#define UART_H

#ifndef UART_RX_CHAR
 #define UART_RX_CHAR 0
#endif

#if UART_RX_CHAR == 1
 #include "dmac.h"
#endif

#if UART_RX_CHAR == 1
enum uart_mode {
	UART_RX_CHAR_MODE,
};

enum uart_baud_generator {
	UART_BAUD_GENERATOR_ARITHMETIC,
	UART_BAUD_GENERATOR_FRACTIONAL
};

enum uart_over_sampling {
	UART_OVER_SAMPLING_3X = 3,
	UART_OVER_SAMPLING_8X = 8,
	UART_OVER_SAMPLING_16X = 16
};

enum uart_bit_order {
	UART_BIT_ORDER_MSB,
	UART_BIT_ORDER_LSB
};

enum uart_parity {
	UART_PARITY_NO,
	UART_PARITY_EVEN,
        UART_PARITY_ODD
};

enum uart_stop_bit {
	UART_1_STOP_BIT,
	UART_2_STOP_BITS
};

enum uart_char_size {
	UART_CHAR_SIZE_5_BITS = 5,
	UART_CHAR_SIZE_6_BITS = 6,
        UART_CHAR_SIZE_7_BITS = 7,
        UART_CHAR_SIZE_8_BITS = 0,
#if UART_RX_CHAR_9_BITS == 1
        UART_CHAR_SIZE_9_BITS = 1
#endif
};

enum uart_standby {
	UART_STANDBY_ONDEMAND,
	UART_STANDBY_ACTIVE
};

enum uart_rx_pad {
	UART_RX_SERCOM_PAD0,
        UART_RX_SERCOM_PAD1,
        UART_RX_SERCOM_PAD2,
        UART_RX_SERCOM_PAD3
};

enum uart_tx_pad {
	UART_TX_SERCOM_PAD0 = 0,
        UART_TX_SERCOM_PAD0_RTS_CTS = 2,
	UART_TX_SERCOM_PAD2 = 1
};

enum uart_conf_pins_cmd {
	UART_CONF_PINS,
	UART_PINS_TO_PORT
};

typedef struct uart_dsc *uart;

struct uart_dsc {
	int id; // <SetIt> [UART_RX_CHAR_MODE]
	int baudrate; // <SetIt> [UART_RX_CHAR_MODE]
	int clk_gen; // <SetIt> - GCLK instance for sercom_clock. [UART_RX_CHAR_MODE]
	int sercom_clock; // <SetIt> - GCLK frequency. [UART_RX_CHAR_MODE]
	void (*conf_pins)(enum uart_conf_pins_cmd); // <SetIt> [UART_RX_CHAR_MODE]
	enum uart_baud_generator generator; // <SetIt> [UART_RX_CHAR_MODE]
	enum uart_over_sampling over_sampling; // <SetIt> [UART_RX_CHAR_MODE]
	enum uart_bit_order bit_order; // <SetIt> [UART_RX_CHAR_MODE]
	enum uart_parity parity; // <SetIt> [UART_RX_CHAR_MODE]
        enum uart_stop_bit stop_bit; // <SetIt> [UART_RX_CHAR_MODE]
	enum uart_char_size char_size; // <SetIt> [UART_RX_CHAR_MODE]
        enum uart_standby standby; // <SetIt> [UART_RX_CHAR_MODE]
        enum uart_rx_pad rx_pad; // <SetIt> [UART_RX_CHAR_MODE]
        enum uart_tx_pad tx_pad; // <SetIt> [UART_RX_CHAR_MODE]
	int rx_que_size; // <SetIt> [UART_RX_CHAR_MODE]
        boolean_t dma; // <SetIt> [UART_RX_CHAR_MODE]
#if DMAC_ON_CHIP == 1
        dmac_channel channel;
	int dmac_rx_trg_num;
	int dmac_tx_trg_num;
#endif
        SercomUsart *mmio;
	unsigned int reg_ctrla;
	unsigned int reg_ctrlb;
	unsigned short reg_baud;
        enum apb_bus_ins apb_bus_ins;
	unsigned int apb_mask;
	int clk_chn;
        IRQn_Type irqn;
        QueueHandle_t sig_que;
        QueueHandle_t rx_que;
	int size;
	void *p_buf;
};
#endif

#if UART_RX_CHAR == 1
/**
 * init_uart
 *
 * Configure SERCOM instance as UART in requested mode.
 *
 * @dev: UART instance.
 * @m: UART mode (enum uart_mode).
 */
void init_uart(uart dev, enum uart_mode m);
#endif

#if UART_RX_CHAR == 1
/**
 * enable_uart
 *
 * Enable UART (revert disable_uart() function effects).
 *
 * @dev: UART instance.
 */
void enable_uart(void *dev);
#endif

#if UART_RX_CHAR == 1
/**
 * disable_uart
 *
 * Disable UART (switch SERCOM block and UART DMAC channel off).
 *
 * @dev: UART instance.
 */
void disable_uart(void *dev);
#endif

#if UART_RX_CHAR == 1
/**
 * uart_tx_buff
 *
 * Transmit data buffer via UART instance.
 * Caller task is blocked during sending data.
 *
 * @dev: UART instance.
 * @p_buf: Pointer to data buffer (bytes or half-words (CHAR9)).
 * @size: Number of units to send.
 *
 * Returns: 0 - success; -EDMA - dma error.
 */
int uart_tx_buff(void *dev, void *p_buf, int size);
#endif

#if UART_RX_CHAR == 1
/**
 * uart_rx_char
 *
 * Receive 5-9 bit char via UART instance.
 * Caller task is blocked until char is not received, timeout is expired
 * or INTR event detected.
 *
 * @dev: UART instance.
 * @p_char: Pointer to byte or half-word (CHAR9) memory for store received char.
 * @tmo: Timeout in tick periods.
 *
 * Returns: 0 - success; -ETMO - no data received in tmo time;
 *          -ERCV - serial line error; -EINTR - receiver interrupted.
 *   In case of ERCV, error bitmap is stored via p_char.
 *   Bit0 - parity error.
 *   Bit1 - frame error.
 *   Bit2 - buffer overflow.
 */
int uart_rx_char(void *dev, void *p_char, TickType_t tmo);
#endif

#if UART_RX_CHAR == 1
/**
 * uart_intr_rx
 *
 * Send INTR event to receiver.
 *
 * @dev: UART instance.
 *
 * Returns: TRUE - event sent successfully; FALSE - rx queue full.
 */
boolean_t uart_intr_rx(void *dev);
#endif

#if UART_RX_CHAR == 1
/**
 * uart_flush_rx
 *
 * Disable receiver, flush receive buffers.
 *
 * @dev: UART instance.
 */
void uart_flush_rx(void *dev);
#endif

#endif
