/*
 * nvm_d.h
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

#ifndef NVM_D_H
#define NVM_D_H

struct nvm_user_row {
	int boot_size;
	int eeprom_size;
	int bod33_level;
	boolean_t bod33_enable;
	int bod33_action;
	boolean_t wdt_enable;
	boolean_t wdt_always_on;
	int wdt_period;
	int wdt_window;
	int wdt_ewoffset;
	boolean_t wdt_wen;
	boolean_t bod33_hyst;
	uint16_t lockbits;
};

extern struct nvm_user_row nvm_user_row;

struct nvm_calib_row {
	int adc_linearity;
	int adc_biascal;
	int osc32k_cal;
	int usb_transn;
	int usb_transp;
        int usb_trim;
	int dfll48m_coarse_cal;
};

extern struct nvm_calib_row nvm_calib_row;

union uc_ser_num {
	uint16_t w16[8];
	uint8_t b[16];
};

extern union uc_ser_num uc_ser_num;

/**
 * init_nvm
 *
 * Initialize NVM module.
 */
void init_nvm(void);

enum nvm_cmd_type {
	NVM_CMD_ERASE_ROW            = NVMCTRL_CTRLA_CMD_ER_Val,
	NVM_CMD_WRITE_PAGE           = NVMCTRL_CTRLA_CMD_WP_Val,
	NVM_CMD_ERASE_AUX_ROW        = NVMCTRL_CTRLA_CMD_EAR_Val,
	NVM_CMD_WRITE_AUX_PAGE       = NVMCTRL_CTRLA_CMD_WAP_Val,
        NVM_CMD_ERASE_EEPR_ROW       = NVMCTRL_CTRLA_CMD_RWWEEER_Val,
	NVM_CMD_WRITE_EEPR_PAGE      = NVMCTRL_CTRLA_CMD_RWWEEWP_Val,
	NVM_CMD_LOCK_REGION          = NVMCTRL_CTRLA_CMD_LR_Val,
	NVM_CMD_UNLOCK_REGION        = NVMCTRL_CTRLA_CMD_UR_Val,
	NVM_CMD_ENTER_LOW_POWER_MODE = NVMCTRL_CTRLA_CMD_SPRM_Val,
	NVM_CMD_EXIT_LOW_POWER_MODE  = NVMCTRL_CTRLA_CMD_CPRM_Val,
	NVM_CMD_PAGE_BUFFER_CLEAR    = NVMCTRL_CTRLA_CMD_PBC_Val,
	NVM_CMD_SET_SECURITY_BIT     = NVMCTRL_CTRLA_CMD_SSB_Val,
	NVM_CMD_INVALL_CACHE         = NVMCTRL_CTRLA_CMD_INVALL_Val
};

/**
 * nvm_cmd
 *
 * Exec NVM command.
 *
 * @cmd: Command type.
 * @addr: Address in NVM memory.
 *
 * Returns: 0 - success; -EACC - NVM locked; -EGEN - NVM generic error.
 */
int nvm_cmd(enum nvm_cmd_type cmd, int addr);

/**
 * nvm_set_wait_states
 *
 * Set FLASH wait states.
 *
 * @f_cpu: F_CPU.
 */
void nvm_set_wait_states(unsigned int f_cpu);

/**
 * upd_nvm_user_row
 *
 * Update NVM user row from RAM struct nvm_user_row.
 *
 * Returns: 0 - success; -EACC - NVM locked; -EGEN - NVM generic error.
 */
int upd_nvm_user_row(void);

/**
 * check_nvm_user_row
 *
 * Check (update) RAM struct nvm_user_row with FUSES definition.
 *
 * Returns: TRUE - struct nvm_user_row is up to date; FALSE - struct nvm_user_row
 *          was updated.
 */
boolean_t check_nvm_user_row(void);

#if TERMOUT == 1
/**
 * log_nvm_user_row
 */
void log_nvm_user_row(void);

/**
 * log_nvm_calib_row
 */
void log_nvm_calib_row(void);

/**
 * log_sn
 */
void log_sn(void);

/**
 * log_nvm_arch
 */
void log_nvm_arch(void);
#endif

#if FUSE_EEPROM_SIZE != 7
/**
 * eeprom_read
 *
 * Load bytes from EEPROM to buffer.
 *
 * @p_buf: Pointer to data buffer.
 * @adr: Data address in EEPROM.
 * @size: Number of bytes to load.
 *
 * Returns: 0 - success; -EHW - Eeprom not implemented on device; -EADDR - bad
 * address or data size.
 */
int eeprom_read(uint8_t *p_buf, int adr, int size);

/**
 * eeprom_write
 *
 * Store bytes from buffer to EEPROM.
 *
 * @p_buf: Pointer to data buffer.
 * @adr: Data address in EEPROM.
 * @size: Number of bytes to store.
 *
 * Returns: 0 - success; -EHW - Eeprom not implemented; -EADDR - bad address or
 *          data size; -EERASE, -EWRITE - Eeprom write error.
 */
int eeprom_write(uint8_t *p_buf, int adr, int size);
#endif

#endif
