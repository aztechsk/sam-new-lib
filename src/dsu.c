/*
 * dsu.c
 *
 * Copyright (c) 2020 Jan Rusnak <jan@rusnak.sk>
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
#include "dsu.h"

enum samd21_devices {
	ATSAMD21J18A  = 0x00,
        ATSAMD21J17A  = 0x01,
        ATSAMD21J16A  = 0x02,
        ATSAMD21J15A  = 0x03,
        ATSAMD21G18A  = 0x05,
	ATSAMD21G17A  = 0x06,
	ATSAMD21G16A  = 0x07,
	ATSAMD21G15A  = 0x08,
        ATSAMD21E18A  = 0x0A,
        ATSAMD21E17A  = 0x0B,
        ATSAMD21E16A  = 0x0C,
        ATSAMD21E15A  = 0x0D,
        ATSAMD21G18AU = 0x0F,
	ATSAMD21G17AU = 0x10,
	ATSAMD21J16B  = 0x20,
        ATSAMD21J15B  = 0x21,
        ATSAMD21G16B  = 0x23,
        ATSAMD21G15B  = 0x24,
        ATSAMD21E16B  = 0x26,
	ATSAMD21E15B  = 0x27,
	ATSAMD21E16L  = 0x3E,
	ATSAMD21E15L  = 0x3F,
	ATSAMD21E16BU = 0x55,
	ATSAMD21E15BU = 0x56,
	ATSAMD21G16L  = 0x57,
	ATSAMD21E16CU = 0x62,
        ATSAMD21E15CU = 0x63,
        ATSAMD21J17D  = 0x92,
        ATSAMD21G17D  = 0x93,
        ATSAMD21E17D  = 0x94,
        ATSAMD21E17DU = 0x95,
        ATSAMD21G17L  = 0x96,
        ATSAMD21E17L  = 0x97
};

#if TERMOUT == 1
static const struct txt_item samd21_devices[] = {
	{ATSAMD21J18A, "J18A"},
        {ATSAMD21J17A, "J17A"},
        {ATSAMD21J16A, "J16A"},
        {ATSAMD21J15A, "J15A"},
        {ATSAMD21G18A, "G18A"},
	{ATSAMD21G17A, "G17A"},
	{ATSAMD21G16A, "G16A"},
	{ATSAMD21G15A, "G15A"},
        {ATSAMD21E18A, "E18A"},
        {ATSAMD21E17A, "E17A"},
        {ATSAMD21E16A, "E16A"},
        {ATSAMD21E15A, "E15A"},
        {ATSAMD21G18AU, "G18AU"},
	{ATSAMD21G17AU, "G17AU"},
	{ATSAMD21J16B, "J16B"},
        {ATSAMD21J15B, "J15B"},
        {ATSAMD21G16B, "G16B"},
        {ATSAMD21G15B, "G15B"},
        {ATSAMD21E16B, "E16B"},
	{ATSAMD21E15B, "E15B"},
	{ATSAMD21E16L, "E16L"},
	{ATSAMD21E15L, "E15L"},
	{ATSAMD21E16BU, "E16BU"},
	{ATSAMD21E15BU, "E15BU"},
	{ATSAMD21G16L, "G16L"},
	{ATSAMD21E16CU, "E16CU"},
        {ATSAMD21E15CU, "E15CU"},
        {ATSAMD21J17D, "J17D"},
        {ATSAMD21G17D, "G17D"},
        {ATSAMD21E17D, "E17D"},
        {ATSAMD21E17DU, "E17DU"},
        {ATSAMD21G17L, "G17L"},
        {ATSAMD21E17L, "E17L"},
	{0, NULL}
};
#endif

struct dev_id dev_id;

/**
 * init_dsu
 */
void init_dsu(void)
{
	dev_id.cpu = DSU->DID.bit.PROCESSOR;
        dev_id.family = DSU->DID.bit.FAMILY;
        dev_id.series = DSU->DID.bit.SERIES;
        dev_id.die = DSU->DID.bit.DIE;
        dev_id.rev_num = DSU->DID.bit.REVISION;
        dev_id.rev = DSU->DID.bit.REVISION + 'A';
        dev_id.dev = DSU->DID.bit.DEVSEL;
}

#if TERMOUT == 1
/**
 * log_dev_id
 */
void log_dev_id(void)
{
	msg(INF, "cpu=%d family=%d series=%d die=%d rev=%d dev=%d\n",
	    dev_id.cpu, dev_id.family, dev_id.series,
	    dev_id.die, dev_id.rev_num, dev_id.dev);
}

/**
 * log_dev_name
 */
void log_dev_name(void)
{
	if (dev_id.cpu == 1) {
		if (dev_id.family == 0) {
			if (dev_id.series == 1) {
				msg(INF, "ATSAMD21%s", find_txt_item(dev_id.dev, samd21_devices, " unknown device"));
				msg(INF, " rev %c\n", dev_id.rev);
				return;
			}
		}
	}
	msg(INF, "ATSAM unknown device\n");
}
#endif
