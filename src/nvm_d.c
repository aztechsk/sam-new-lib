/*
 * nvm_d.c
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
#include "fmalloc.h"
#include "hwerr.h"
#include "tools.h"
#include "pm.h"
#include "dsu.h"
#include "nvm.h"

#define FLASH_ROW_SZ (FLASH_PAGE_SIZE * 4)

struct nvm_user_row nvm_user_row;
struct nvm_calib_row nvm_calib_row;
union uc_ser_num uc_ser_num;

static struct nvm_arch {
	int page_size;
	int pages_num;
	int rww_eeprom;
} nvm_arch;

static inline boolean_t nvm_ready(void);

/**
 * init_nvm
 */
void init_nvm(void)
{
	uint32_t row[2];

	enable_per_apb_clk(APB_BUS_INST_B, PM_APBBMASK_NVMCTRL);
	while (!nvm_ready());
	((uint16_t *) &row)[0] = *((volatile uint16_t *) NVMCTRL_USER);
	((uint16_t *) &row)[1] = *((volatile uint16_t *) NVMCTRL_USER + 1);
	((uint16_t *) &row)[2] = *((volatile uint16_t *) NVMCTRL_USER + 2);
	((uint16_t *) &row)[3] = *((volatile uint16_t *) NVMCTRL_USER + 3);
        nvm_user_row.boot_size = (row[0] & NVMCTRL_FUSES_BOOTPROT_Msk) >> NVMCTRL_FUSES_BOOTPROT_Pos;
        nvm_user_row.eeprom_size = (row[0] & NVMCTRL_FUSES_EEPROM_SIZE_Msk) >> NVMCTRL_FUSES_EEPROM_SIZE_Pos;
        nvm_user_row.bod33_level = (row[0] & FUSES_BOD33USERLEVEL_Msk) >> FUSES_BOD33USERLEVEL_Pos;
	nvm_user_row.bod33_enable = (row[0] & FUSES_BOD33_EN_Msk) >> FUSES_BOD33_EN_Pos;
	nvm_user_row.bod33_action = (row[0] & FUSES_BOD33_ACTION_Msk) >> FUSES_BOD33_ACTION_Pos;
	nvm_user_row.wdt_enable = (row[0] & WDT_FUSES_ENABLE_Msk) >> WDT_FUSES_ENABLE_Pos;
	nvm_user_row.wdt_always_on = (row[0] & WDT_FUSES_ALWAYSON_Msk) >> WDT_FUSES_ALWAYSON_Pos;
	nvm_user_row.wdt_period = (row[0] & WDT_FUSES_PER_Msk) >> WDT_FUSES_PER_Pos;
	nvm_user_row.wdt_window = ((row[0] & WDT_FUSES_WINDOW_0_Msk) >> WDT_FUSES_WINDOW_0_Pos) |
			          ((row[1] & WDT_FUSES_WINDOW_1_Msk) << 1);
	nvm_user_row.wdt_ewoffset = (row[1] & WDT_FUSES_EWOFFSET_Msk) >> WDT_FUSES_EWOFFSET_Pos;
	nvm_user_row.wdt_wen = (row[1] & WDT_FUSES_WEN_Msk) >> WDT_FUSES_WEN_Pos;
	nvm_user_row.bod33_hyst = (row[1] & FUSES_BOD33_HYST_Msk) >> FUSES_BOD33_HYST_Pos;
        nvm_user_row.lockbits = (row[1] & NVMCTRL_FUSES_REGION_LOCKS_Msk) >> NVMCTRL_FUSES_REGION_LOCKS_Pos;
	((uint16_t *) &row)[0] = *((volatile uint16_t *) NVMCTRL_OTP4);
	((uint16_t *) &row)[1] = *((volatile uint16_t *) NVMCTRL_OTP4 + 1);
	((uint16_t *) &row)[2] = *((volatile uint16_t *) NVMCTRL_OTP4 + 2);
	((uint16_t *) &row)[3] = *((volatile uint16_t *) NVMCTRL_OTP4 + 3);
	nvm_calib_row.adc_linearity = ((row[0] & ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos) |
	                              ((row[1] & ADC_FUSES_LINEARITY_1_Msk) << 5);
	nvm_calib_row.adc_biascal = (row[1] & ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;
	nvm_calib_row.osc32k_cal = (row[1] & SYSCTRL_FUSES_OSC32K_CAL_Msk) >> SYSCTRL_FUSES_OSC32K_CAL_Pos;
	nvm_calib_row.usb_transn = (row[1] & USB_FUSES_TRANSN_Msk) >> USB_FUSES_TRANSN_Pos;
	nvm_calib_row.usb_transp = (row[1] & USB_FUSES_TRANSP_Msk) >> USB_FUSES_TRANSP_Pos;
        nvm_calib_row.usb_trim = (row[1] & USB_FUSES_TRIM_Msk) >> USB_FUSES_TRIM_Pos;
        nvm_calib_row.dfll48m_coarse_cal = (row[1] & FUSES_DFLL48M_COARSE_CAL_Msk) >> FUSES_DFLL48M_COARSE_CAL_Pos;
	uc_ser_num.w16[0] = *((volatile uint16_t *) 0x0080A00C);
	uc_ser_num.w16[1] = *((volatile uint16_t *) 0x0080A00C + 1);
	uc_ser_num.w16[2] = *((volatile uint16_t *) 0x0080A040);
	uc_ser_num.w16[3] = *((volatile uint16_t *) 0x0080A040 + 1);
        uc_ser_num.w16[4] = *((volatile uint16_t *) 0x0080A044);
	uc_ser_num.w16[5] = *((volatile uint16_t *) 0x0080A044 + 1);
	uc_ser_num.w16[6] = *((volatile uint16_t *) 0x0080A048);
	uc_ser_num.w16[7] = *((volatile uint16_t *) 0x0080A048 + 1);
	nvm_arch.page_size = 8 << NVMCTRL->PARAM.bit.PSZ;
        nvm_arch.pages_num = NVMCTRL->PARAM.bit.NVMP;
        nvm_arch.rww_eeprom = NVMCTRL->PARAM.bit.RWWEEP;
#if defined(FAM_SAMD21) // NVM errata: power-down modes and wake-up from sleep.
	if (dev_id.rev < 'E')  {
		NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY | NVMCTRL_CTRLB_SLEEPPRM_DISABLED |
		                     NVMCTRL_CTRLB_MANW;
	} else {
		NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY | NVMCTRL_CTRLB_MANW;
	}
#else
 #error "Check NVM errata"
#endif
}

/**
 * nvm_set_wait_states
 */
void nvm_set_wait_states(unsigned int f_cpu)
{
	int ws = 0;
	unsigned int reg;

#if VDD_HIGH_THEN_2_7V == 1
	if (f_cpu > 24000000) {
		ws = 1;
	}
#else
	if (f_cpu <= 14000000) {
		ws = 0;
	} else if (f_cpu <= 28000000) {
		ws = 1;
	} else if (f_cpu <= 42000000) {
		ws = 2;
	} else {
		ws = 3;
	}
#endif
	reg = NVMCTRL->CTRLB.reg;
	reg &= ~(NVMCTRL_CTRLB_RWS_Msk | NVMCTRL_CTRLB_READMODE_Msk);
	reg |= NVMCTRL_CTRLB_RWS(ws);
	reg |= (ws == 0) ? NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY : NVMCTRL_CTRLB_READMODE_DETERMINISTIC;
        NVMCTRL->CTRLB.reg = reg;
}

/**
 * nvm_cmd
 */
int nvm_cmd(enum nvm_cmd_type cmd, int addr)
{
	unsigned int ctrlb_bk;

	if ((addr > nvm_arch.page_size * nvm_arch.pages_num &&
	    !(addr >= NVMCTRL_AUX0_ADDRESS && addr <= NVMCTRL_AUX1_ADDRESS)) ||
	    addr % nvm_arch.page_size) {
		crit_err_exit(BAD_PARAMETER);
	}
        taskENTER_CRITICAL();
	if (!nvm_ready()) {
		taskEXIT_CRITICAL();
		crit_err_exit(UNEXP_PROG_STATE);
	}
        ctrlb_bk = NVMCTRL->CTRLB.reg;
        NVMCTRL->CTRLB.reg |= NVMCTRL_CTRLB_CACHEDIS;
        NVMCTRL->CTRLB.reg;
	switch (cmd) {
	case NVM_CMD_ERASE_AUX_ROW        :
		/* FALLTHRU */
	case NVM_CMD_WRITE_AUX_PAGE       :
		if (NVMCTRL->STATUS.reg & NVMCTRL_STATUS_SB) {
			NVMCTRL->CTRLB.reg = ctrlb_bk;
			NVMCTRL->CTRLB.reg;
                        taskEXIT_CRITICAL();
			return (-EACC);
		}
                NVMCTRL->ADDR.reg = addr / 2;
		break;
        case NVM_CMD_ERASE_ROW            :
		/* FALLTHRU */
	case NVM_CMD_WRITE_PAGE           :
		/* FALLTHRU */
        case NVM_CMD_ERASE_EEPR_ROW       :
		/* FALLTHRU */
        case NVM_CMD_WRITE_EEPR_PAGE      :
		/* FALLTHRU */
	case NVM_CMD_LOCK_REGION          :
		/* FALLTHRU */
	case NVM_CMD_UNLOCK_REGION        :
		NVMCTRL->ADDR.reg = addr / 2;
		break;
	case NVM_CMD_PAGE_BUFFER_CLEAR    :
		/* FALLTHRU */
	case NVM_CMD_SET_SECURITY_BIT     :
		/* FALLTHRU */
	case NVM_CMD_ENTER_LOW_POWER_MODE :
		/* FALLTHRU */
	case NVM_CMD_EXIT_LOW_POWER_MODE  :
		/* FALLTHRU */
        case NVM_CMD_INVALL_CACHE         :
		break;
	default                           :
		NVMCTRL->CTRLB.reg = ctrlb_bk;
		NVMCTRL->CTRLB.reg;
                taskEXIT_CRITICAL();
		crit_err_exit(BAD_PARAMETER);
		break;
	}
	NVMCTRL->INTFLAG.reg = NVMCTRL_INTFLAG_ERROR;
	NVMCTRL->STATUS.reg = NVMCTRL_STATUS_MASK;
        NVMCTRL->CTRLA.reg = cmd | NVMCTRL_CTRLA_CMDEX_KEY;
	while (!nvm_ready());
	unsigned short stat = NVMCTRL->STATUS.reg;
        NVMCTRL->CTRLB.reg = ctrlb_bk;
        NVMCTRL->CTRLB.reg;
	if (stat & NVMCTRL_STATUS_LOCKE) {
		taskEXIT_CRITICAL();
		return (-EACC);
	} else if (stat & NVMCTRL_STATUS_PROGE ||
		   stat & NVMCTRL_STATUS_NVME) {
		taskEXIT_CRITICAL();
		return (-EGEN);
	}
        taskEXIT_CRITICAL();
	return (0);
}

/**
 * upd_nvm_user_row
 */
int upd_nvm_user_row(void)
{
	uint32_t row[2];

	((uint16_t *) &row)[0] = *((volatile uint16_t *) NVMCTRL_USER);
	((uint16_t *) &row)[1] = *((volatile uint16_t *) NVMCTRL_USER + 1);
	((uint16_t *) &row)[2] = *((volatile uint16_t *) NVMCTRL_USER + 2);
	((uint16_t *) &row)[3] = *((volatile uint16_t *) NVMCTRL_USER + 3);
	row[0] &= ~NVMCTRL_FUSES_BOOTPROT_Msk;
	row[0] |= NVMCTRL_FUSES_BOOTPROT(nvm_user_row.boot_size);
	row[0] &= ~NVMCTRL_FUSES_EEPROM_SIZE_Msk;
        row[0] |= NVMCTRL_FUSES_EEPROM_SIZE(nvm_user_row.eeprom_size);
	row[0] &= ~FUSES_BOD33USERLEVEL_Msk;
	row[0] |= FUSES_BOD33USERLEVEL(nvm_user_row.bod33_level);
	row[0] &= ~FUSES_BOD33_EN_Msk;
	row[0] |= nvm_user_row.bod33_enable << FUSES_BOD33_EN_Pos;
        row[0] &= ~FUSES_BOD33_ACTION_Msk;
        row[0] |= FUSES_BOD33_ACTION(nvm_user_row.bod33_action);
	row[0] &= ~WDT_FUSES_ENABLE_Msk;
	row[0] |= nvm_user_row.wdt_enable << WDT_FUSES_ENABLE_Pos;
	row[0] &= ~WDT_FUSES_ALWAYSON_Msk;
	row[0] |= nvm_user_row.wdt_always_on << WDT_FUSES_ALWAYSON_Pos;
	row[0] &= ~WDT_FUSES_PER_Msk;
	row[0] |= WDT_FUSES_PER(nvm_user_row.wdt_period);
        row[0] &= ~WDT_FUSES_WINDOW_0_Msk;
	row[0] |= (nvm_user_row.wdt_window & 0x01) << WDT_FUSES_WINDOW_0_Pos;
        row[1] &= ~WDT_FUSES_WINDOW_1_Msk;
	row[1] |= (nvm_user_row.wdt_window >> 1) << WDT_FUSES_WINDOW_1_Pos;
	row[1] &= ~WDT_FUSES_EWOFFSET_Msk;
	row[1] |= WDT_FUSES_EWOFFSET(nvm_user_row.wdt_ewoffset);
	row[1] &= ~WDT_FUSES_WEN_Msk;
	row[1] |= nvm_user_row.wdt_wen << WDT_FUSES_WEN_Pos;
	row[1] &= ~FUSES_BOD33_HYST_Msk;
	row[1] |= nvm_user_row.bod33_hyst << FUSES_BOD33_HYST_Pos;
	row[1] &= ~NVMCTRL_FUSES_REGION_LOCKS_Msk;
	row[1] |= NVMCTRL_FUSES_REGION_LOCKS(nvm_user_row.lockbits);
	int ret = nvm_cmd(NVM_CMD_ERASE_AUX_ROW, NVMCTRL_USER);
	if (ret) {
		return (ret);
	}
        ret = nvm_cmd(NVM_CMD_PAGE_BUFFER_CLEAR, NVMCTRL_USER);
	if (ret) {
		return (ret);
	}
        *((volatile uint32_t *) NVMCTRL_USER) = row[0];
        *((volatile uint32_t *) NVMCTRL_USER + 1) = row[1];
	return (nvm_cmd(NVM_CMD_WRITE_AUX_PAGE, NVMCTRL_USER));
}

/**
 * check_nvm_user_row
 */
boolean_t check_nvm_user_row(void)
{
	boolean_t ret = TRUE;

	if (nvm_user_row.boot_size != FUSE_BOOT_SIZE) {
		nvm_user_row.boot_size = FUSE_BOOT_SIZE;
		ret = FALSE;
	}
	if (nvm_user_row.eeprom_size != FUSE_EEPROM_SIZE) {
		nvm_user_row.eeprom_size = FUSE_EEPROM_SIZE;
		ret = FALSE;
	}
	if (nvm_user_row.bod33_level != FUSE_BOD33_LEVEL) {
		nvm_user_row.bod33_level = FUSE_BOD33_LEVEL;
		ret = FALSE;
	}
       	if (nvm_user_row.bod33_enable != FUSE_BOD33_ENABLE) {
		nvm_user_row.bod33_enable = FUSE_BOD33_ENABLE;
		ret = FALSE;
	}
	if (nvm_user_row.bod33_action != FUSE_BOD33_ACTION) {
		nvm_user_row.bod33_action = FUSE_BOD33_ACTION;
		ret = FALSE;
	}
	if (nvm_user_row.wdt_enable != FUSE_WDT_ENABLE) {
		nvm_user_row.wdt_enable = FUSE_WDT_ENABLE;
		ret = FALSE;
	}
	if (nvm_user_row.wdt_always_on != FUSE_WDT_ALWAYS_ON) {
		nvm_user_row.wdt_always_on = FUSE_WDT_ALWAYS_ON;
		ret = FALSE;
	}
	if (nvm_user_row.wdt_period != WDT_CFG_PERIOD) {
		nvm_user_row.wdt_period = WDT_CFG_PERIOD;
		ret = FALSE;
	}
	if (nvm_user_row.wdt_window != WDT_CFG_WINDOW) {
		nvm_user_row.wdt_window = WDT_CFG_WINDOW;
		ret = FALSE;
	}
	if (nvm_user_row.wdt_ewoffset != WDT_CFG_EWOFFSET) {
		nvm_user_row.wdt_ewoffset = WDT_CFG_EWOFFSET;
		ret = FALSE;
	}
	if (nvm_user_row.wdt_wen != WDT_CFG_WIND_EN) {
		nvm_user_row.wdt_wen = WDT_CFG_WIND_EN;
		ret = FALSE;
	}
	if (nvm_user_row.bod33_hyst != FUSE_BOD33_HYST) {
		nvm_user_row.bod33_hyst = FUSE_BOD33_HYST;
		ret = FALSE;
	}
	return (ret);
}

#if TERMOUT == 1
/**
 * log_nvm_user_row
 */
void log_nvm_user_row(void)
{
	UBaseType_t prio;
        char *s;

        prio = uxTaskPriorityGet(NULL);
        vTaskPrioritySet(NULL, TASK_PRIO_HIGH);
	msg(INF, "# fuses:\n");
	msg(INF, "boot_size: %d\n", nvm_user_row.boot_size);
        msg(INF, "eeprom_size: %d\n", nvm_user_row.eeprom_size);
        msg(INF, "bod33_level: %d\n", nvm_user_row.bod33_level);
        msg(INF, "bod33_enable: %d\n", nvm_user_row.bod33_enable);
        msg(INF, "bod33_action: %d\n", nvm_user_row.bod33_action);
        msg(INF, "wdt_enable: %d\n", nvm_user_row.wdt_enable);
        msg(INF, "wdt_always_on: %d\n", nvm_user_row.wdt_always_on);
        msg(INF, "wdt_period: 0x%02X\n", nvm_user_row.wdt_period);
        msg(INF, "wdt_window: 0x%02X\n", nvm_user_row.wdt_window);
        msg(INF, "wdt_ewoffset: 0x%02X\n", nvm_user_row.wdt_ewoffset);
        msg(INF, "wdt_wen: %d\n", nvm_user_row.wdt_wen);
        msg(INF, "bod33_hyst: %d\n", nvm_user_row.bod33_hyst);
	if (NULL == (s = pvPortMalloc(64))) {
		crit_err_exit(MALLOC_ERROR);
	}
	prn_bv_str(s, nvm_user_row.lockbits, 16);
	msg(INF, "lockbits: %s\n", s);
        vPortFree(s);
        msg(INF, "#\n");
	vTaskPrioritySet(NULL, prio);
}

/**
 * log_nvm_calib_row
 */
void log_nvm_calib_row(void)
{
	UBaseType_t prio;

        prio = uxTaskPriorityGet(NULL);
        vTaskPrioritySet(NULL, TASK_PRIO_HIGH);
	msg(INF, "# calib:\n");
	msg(INF, "adc_linearity: 0x%02X\n", nvm_calib_row.adc_linearity);
	msg(INF, "adc_biascal: 0x%02X\n", nvm_calib_row.adc_biascal);
	msg(INF, "osc32k_cal: 0x%02X\n", nvm_calib_row.osc32k_cal);
	msg(INF, "usb_transn: 0x%02X\n", nvm_calib_row.usb_transn);
	msg(INF, "usb_transp: 0x%02X\n", nvm_calib_row.usb_transp);
	msg(INF, "usb_trim: 0x%02X\n", nvm_calib_row.usb_trim);
        msg(INF, "dfll48m_coarse_cal: 0x%02X\n", nvm_calib_row.dfll48m_coarse_cal);
        msg(INF, "#\n");
	vTaskPrioritySet(NULL, prio);
}

/**
 * log_sn
 */
void log_sn(void)
{
	msg(INF, "sn: ");
	for (int i = 0; i < 8; i++) {
		msg(INF, "%04hX", uc_ser_num.w16[i]);
	}
	msg(INF, "\n");
}

enum nvm_read_mode {
	NVM_RM_NO_MISS_PENALTY,
        NVM_RM_LOW_POWER,
        NVM_RM_DETERMINISTIC
};

static const struct txt_item nvm_read_mode_txt[] = {
	{NVM_RM_NO_MISS_PENALTY, "no_miss_penalty"},
        {NVM_RM_LOW_POWER, "low_power"},
        {NVM_RM_DETERMINISTIC, "deterministic"},
	{0, NULL}
};

enum nvm_pow_mode {
	NVM_PM_WAKEUPACCESS,
        NVM_PM_WAKEUPINSTANT,
        NVM_PM_DISABLED = 3
};

static const struct txt_item nvm_pow_mode_txt[] = {
	{NVM_PM_WAKEUPACCESS, "wake_access"},
        {NVM_PM_WAKEUPINSTANT, "wake_instant"},
        {NVM_PM_DISABLED, "pm_disabled"},
	{0, NULL}
};

/**
 * log_nvm_arch
 */
void log_nvm_arch(void)
{
	unsigned int ctrlb = NVMCTRL->CTRLB.reg;

	msg(INF, "FLASH page_size=%d pages_num=%d rww_eeprom_pages=%d\n",
	    nvm_arch.page_size, nvm_arch.pages_num, nvm_arch.rww_eeprom);
	msg(INF, "FLASH ws=%d read_mode=%s pow_mode=%s\n",
	    (ctrlb & NVMCTRL_CTRLB_RWS_Msk) >> NVMCTRL_CTRLB_RWS_Pos,
	    find_txt_item((ctrlb & NVMCTRL_CTRLB_READMODE_Msk) >> NVMCTRL_CTRLB_READMODE_Pos, nvm_read_mode_txt, "err"),
	    find_txt_item((ctrlb & NVMCTRL_CTRLB_SLEEPPRM_Msk) >> NVMCTRL_CTRLB_SLEEPPRM_Pos, nvm_pow_mode_txt, "err"));
}
#endif

#if FUSE_EEPROM_SIZE != 7
/**
 * eeprom_read
 */
int eeprom_read(uint8_t *p_buf, int adr, int size)
{
	volatile uint8_t *pee = (uint8_t *) 0x00400000;
	int ee_sz = nvm_arch.rww_eeprom * nvm_arch.page_size;

	if (nvm_arch.rww_eeprom == 0) {
		return (-EHW);
	}
	if (adr + size > ee_sz) {
		return (-EADDR);
	}
	for (int i = 0; i < size; i++) {
		*(p_buf + i) = *(pee + adr + i);
	}
	return (0);
}

static union {
	uint8_t row8[FLASH_ROW_SZ];
	uint32_t row32[FLASH_ROW_SZ / 4];
} row;

/**
 * eeprom_write
 */
int eeprom_write(uint8_t *p_buf, int adr, int size)
{
	volatile uint32_t *pee = (uint32_t *) 0x00400000;
	int ee_sz = nvm_arch.rww_eeprom * nvm_arch.page_size;
	int rows = ee_sz / FLASH_ROW_SZ;
	boolean_t fst = TRUE;

	if (nvm_arch.rww_eeprom == 0) {
		return (-EHW);
	}
	if (adr + size > ee_sz) {
		return (-EADDR);
	}
	for (int r = 0; r < rows; r++) {
		if (fst && adr > FLASH_ROW_SZ - 1 + r * FLASH_ROW_SZ) {
			continue;
		}
		(void) eeprom_read(row.row8, FLASH_ROW_SZ * r, FLASH_ROW_SZ);
		int idx;
		if (fst) {
			fst = FALSE;
			idx = adr - r * FLASH_ROW_SZ;
		} else {
			idx = 0;
		}
		do {
			row.row8[idx++] = *p_buf++;
		} while (--size && idx < FLASH_ROW_SZ);
		*(pee + FLASH_ROW_SZ / 4 * r) = 0xFFFF; // NVM errata: INTFLAG.READY bit is not updated after a RWWEEER.
		if (0 != nvm_cmd(NVM_CMD_ERASE_EEPR_ROW, (unsigned int) (pee + FLASH_ROW_SZ / 4 * r))) {
			return (-EERASE);
		}
		for (int p = 0; p < 4; p++) {
			if (0 != nvm_cmd(NVM_CMD_PAGE_BUFFER_CLEAR, 0)) {
				return (-EERASE);
			}
			for (int j = 0; j < FLASH_PAGE_SIZE / 4; j++) {
				*(pee + (FLASH_ROW_SZ / 4 * r) + (FLASH_PAGE_SIZE / 4 * p) + j) =
				row.row32[(FLASH_PAGE_SIZE / 4 * p) + j];
			}
			if (0 != nvm_cmd(NVM_CMD_WRITE_EEPR_PAGE,
			                (unsigned int) (pee + (FLASH_ROW_SZ / 4 * r) + (FLASH_PAGE_SIZE / 4 * p)))) {
				return (-EWRITE);
			}
		}
	}
	return (0);
}
#endif

/**
 * nvm_ready
 */
static inline boolean_t nvm_ready(void)
{
	return ((NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) ? TRUE : FALSE);
}
