/*
 * btn.h
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

#ifndef BTN_H
#define BTN_H

#ifndef BTN
 #define BTN 0
#endif

#if BTN == 1

#include "eic.h"
#include "port.h"

typedef struct btn_dsc *btn;

enum btn_mode {
	BTN_REPORT_MODE,
	BTN_EVENT_MODE
};

struct btn_dsc {
	struct eintctl_pin_cfg eintctl_pin; // <SetIt>
        int pin; // <SetIt>
	PortGroup *port; // <SetIt>
	enum btn_mode mode; // <SetIt>
        int evnt_que_size; // <SetIt>
	const char *tsk_nm; // <SetIt>
        QueueHandle_t evnt_que;
	QueueHandle_t intr_que;
	int evnt_que_full_err;
	int intr_que_full_err;
	int pin_lev_err;
	int clbk_num;
        TaskHandle_t tsk_hndl;
        struct btn_dsc *next;
};

enum btn_evnt_type {
	BTN_PRESSED_DOWN,
	BTN_PRESS,
	BTN_RELEASE
};

struct btn_evnt {
	enum btn_evnt_type type;
	TickType_t time;
};

/**
 * add_btn_dev
 *
 * Register new button device.
 *
 * @dev: Button instance.
 */
void add_btn_dev(btn dev);

#if TERMOUT == 1
/**
 * log_btn_stats
 */
void log_btn_stats(btn dev);
#endif

#endif

#endif
