/*
 * port.h
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

#ifndef PORT_H
#define PORT_H

#if (PORT_GROUPS > 0)
 #define PORTA (&PORT->Group[0])
#endif
#if (PORT_GROUPS > 1)
 #define PORTB (&PORT->Group[1])
#endif
#if (PORT_GROUPS > 2)
 #define PORTC (&PORT->Group[2])
#endif
#if (PORT_GROUPS > 3)
 #error "PORT_GROUPS > 3"
#endif

enum port_pin_func {
	PIN_FUNC_INPUT,
	PIN_FUNC_INPUT_PULL_UP,
        PIN_FUNC_INPUT_PULL_DOWN,
	PIN_FUNC_OUTPUT_HIGH,
        PIN_FUNC_OUTPUT_LOW,
	PIN_FUNC_PULL_UP,
        PIN_FUNC_PULL_DOWN,
	PIN_FUNC_DIGITAL_OFF,
        PIN_FUNC_PERIPHERAL_A,
        PIN_FUNC_PERIPHERAL_B,
        PIN_FUNC_PERIPHERAL_C,
        PIN_FUNC_PERIPHERAL_D,
        PIN_FUNC_PERIPHERAL_E,
        PIN_FUNC_PERIPHERAL_F,
        PIN_FUNC_PERIPHERAL_G,
        PIN_FUNC_PERIPHERAL_H,
        PIN_FUNC_PERIPHERAL_I
};

enum port_pin_feat {
	PIN_FEAT_INPUT_BUFFER,
        PIN_FEAT_CONT_INPUT_READ,
        PIN_FEAT_STRONG_DRIVER,
        PIN_FEAT_PULL_UP,
	PIN_FEAT_PULL_DOWN,
        PIN_FEAT_END
};

/**
 * init_port
 *
 * Initialize PORT controller.
 */
void init_port(void);

/**
 * conf_pin
 *
 * Configure PORT pin.
 *
 * PIN_FUNC_INPUT
 *  = standard input.
 * PIN_FUNC_INPUT_PULL_UP
 *  = input with pullup.
 * PIN_FUNC_INPUT_PULL_DOWN
 *  = input with pulldown.
 * PIN_FUNC_OUTPUT_HIGH + [PIN_FEAT_STRONG_DRIVER]
 *  = totem-pole output with disabled input buffer.
 * PIN_FUNC_OUTPUT_LOW + [PIN_FEAT_STRONG_DRIVER]
 *  = totem-pole output with disabled input buffer.
 * PIN_FUNC_OUTPUT_HIGH + PIN_FEAT_INPUT_BUFFER + [PIN_FEAT_STRONG_DRIVER]
 *  = totem-pole output with enabled input buffer.
 * PIN_FUNC_OUTPUT_LOW + PIN_FEAT_INPUT_BUFFER + [PIN_FEAT_STRONG_DRIVER]
 *  = totem-pole output with enabled input buffer.
 * PIN_FUNC_PULL_UP
 *  = output with pull.
 * PIN_FUNC_PULL_DOWN
 *  = output with pull.
 * PIN_FUNC_DIGITAL_OFF
 *  = digital functionality disabled.
 * PIN_FUNC_PERIPHERAL_x + [PIN_FEAT_INPUT_BUFFER | PIN_FEAT_STRONG_DRIVER | PIN_FEAT_PULL_UP | PIN_FEAT_PULL_DOWN]
 *  = pin routed to peripheral.
 *
 * @pin: Pin position number.
 * @grp: PORT pins group.
 * @func: Pin function definition (enum port_pin_func).
 * @va: List of additional pin features terminated with PIN_FEAT_END (enum port_pin_feat).
 */
void conf_pin(int pin, PortGroup *grp, enum port_pin_func func, ...);

/**
 * get_pin_lev
 *
 * Get PORT pin input logical level.
 *
 * @pin: Pin position number.
 * @grp: PORT pins group.
 *
 * Returns: HIGH; LOW.
 */
inline boolean_t get_pin_lev(int pin, PortGroup *grp)
{
	return ((grp->IN.reg & (1 << pin)) ? HIGH : LOW);
}

/**
 * set_pin_lev
 *
 * Set PORT pin logical level.
 *
 * @pin: Pin position number.
 * @grp: PORT pins group.
 * @lev: Logical level.
 */
inline void set_pin_lev(int pin, PortGroup *grp, boolean_t lev)
{
	if (lev == HIGH) {
		grp->OUTSET.reg = 1 << pin;
	} else {
		grp->OUTCLR.reg = 1 << pin;
	}
}

/**
 * get_pin_out
 *
 * Get PORT pin output logical level setting.
 *
 * @pin: Pin position number.
 * @grp: PORT pins group.
 *
 * Returns: HIGH; LOW.
 */
inline boolean_t get_pin_out(int pin, PortGroup *grp)
{
	return ((grp->OUT.reg & (1 << pin)) ? HIGH : LOW);
}
#endif
