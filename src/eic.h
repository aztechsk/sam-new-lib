/*
 * eic.h
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

#ifndef EIC_H
#define EIC_H

#ifndef EINTCTL
 #define EINTCTL 0
#endif

#if EINTCTL == 1
enum eintctl_intr_mode {
	EINTCTL_INTR_NONE,
	EINTCTL_INTR_RISE,
	EINTCTL_INTR_FALL,
        EINTCTL_INTR_BOTH,
	EINTCTL_INTR_HIGH,
	EINTCTL_INTR_LOW
};

typedef struct eintctl_pin_cfg *eintctl_pin;

struct eintctl_pin_cfg {
	int n;
	boolean_t wake;
	boolean_t filt;
	enum eintctl_intr_mode intr_mode;
};

enum eintctl_gclk {
	EINTCTL_GCLK_ON,
        EINTCTL_GCLK_OFF
};

/**
 * init_eintctl
 *
 * Configure EINTCTL.
 *
 * @gclk: EINTCTL clock channel enabled/disabled (enum eintctl_gclk).
 * @va1: GCLK instance number.
 */
void init_eintctl(enum eintctl_gclk gclk, ...);

/**
 * enable_eintctl
 *
 * Enable EINTCTL (revert disable_eintctl() function effects).
 */
void enable_eintctl(void);

/**
 * disable_eintctl
 *
 * Disable EINTCTL.
 */
void disable_eintctl(void);

/**
 * is_eintctl_enabled
 */
boolean_t is_eintctl_enabled(void);

/**
 * reg_eintctl_isr_clbk
 */
void reg_eintctl_isr_clbk(BaseType_t (*clbk)(unsigned short intflg));
#endif

/**
 * conf_eintctl_pin
 *
 * Configure EINTCTL pin.
 *
 * @pin: Pin instance.
 */
void conf_eintctl_pin(eintctl_pin pin);

/**
 * disable_eintctl_pin
 *
 * Disable EINTCTL pin.
 *
 * @pin: Pin instance.
 */
void disable_eintctl_pin(eintctl_pin pin);

/**
 * eintctl_intr_enable
 */
inline void eintctl_intr_enable(eintctl_pin pin)
{
	EIC->INTENSET.reg = 1 << pin->n;
}

/**
 * eintctl_intr_disable
 */
inline void eintctl_intr_disable(eintctl_pin pin)
{
	EIC->INTENCLR.reg = 1 << pin->n;
}

/**
 * eintctl_intr_clear
 */
inline void eintctl_intr_clear(eintctl_pin pin)
{
	EIC->INTFLAG.reg = 1 << pin->n;
}

/**
 * is_eintctl_intr_enabled
 */
inline boolean_t is_eintctl_intr_enabled(eintctl_pin pin)
{
	if (EIC->INTENSET.reg & (1 << pin->n)) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}

/**
 * eintctl_intr_clear_all
 */
void eintctl_intr_clear_all(void);
#endif
