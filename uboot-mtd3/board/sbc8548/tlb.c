/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/mmu.h>

struct fsl_e_tlb_entry tlb_table[] = {
	/* TLB 0 - for temp stack in cache */
	SET_TLB_ENTRY(0, CONFIG_SYS_INIT_RAM_ADDR, CONFIG_SYS_INIT_RAM_ADDR,
		      MAS3_SX|MAS3_SW|MAS3_SR, 0,
		      0, 0, BOOKE_PAGESZ_4K, 0),
	SET_TLB_ENTRY(0, CONFIG_SYS_INIT_RAM_ADDR + 4 * 1024,
		      CONFIG_SYS_INIT_RAM_ADDR + 4 * 1024,
		      MAS3_SX|MAS3_SW|MAS3_SR, 0,
		      0, 0, BOOKE_PAGESZ_4K, 0),
	SET_TLB_ENTRY(0, CONFIG_SYS_INIT_RAM_ADDR + 8 * 1024,
		      CONFIG_SYS_INIT_RAM_ADDR + 8 * 1024,
		      MAS3_SX|MAS3_SW|MAS3_SR, 0,
		      0, 0, BOOKE_PAGESZ_4K, 0),
	SET_TLB_ENTRY(0, CONFIG_SYS_INIT_RAM_ADDR + 12 * 1024,
		      CONFIG_SYS_INIT_RAM_ADDR + 12 * 1024,
		      MAS3_SX|MAS3_SW|MAS3_SR, 0,
		      0, 0, BOOKE_PAGESZ_4K, 0),

	/*
	 * TLB 0:	64M	Non-cacheable, guarded
	 * 0xfc000000	56M	8MB -> 64MB of user flash
	 * 0xff800000	8M	boot FLASH
	 * Out of reset this entry is only 4K.
	 */
	SET_TLB_ENTRY(1, CONFIG_SYS_ALT_FLASH + 0x800000,
		      CONFIG_SYS_ALT_FLASH + 0x800000,
		      MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		      0, 0, BOOKE_PAGESZ_64M, 1),

	/*
	 * TLB 1:	1G	Non-cacheable, guarded
	 * 0x80000000	512M	PCI1 MEM
	 * 0xa0000000	512M	PCIe MEM
	 */
	SET_TLB_ENTRY(1, CONFIG_SYS_PCI1_MEM_VIRT, CONFIG_SYS_PCI1_MEM_PHYS,
		      MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		      0, 1, BOOKE_PAGESZ_1G, 1),

	/*
	 * TLB 2:	256M Cacheable, non-guarded
	 * 0x0		256M DDR SDRAM
	 */
#if !defined(CONFIG_SPD_EEPROM)
	SET_TLB_ENTRY(1, CONFIG_SYS_DDR_SDRAM_BASE, CONFIG_SYS_DDR_SDRAM_BASE,
		      MAS3_SX|MAS3_SW|MAS3_SR, 0,
		      0, 2, BOOKE_PAGESZ_256M, 1),
#endif

	/*
	 * TLB 3:	64M	Non-cacheable, guarded
	 * 0xe0000000	1M	CCSRBAR
	 * 0xe2000000	8M	PCI1 IO
	 * 0xe2800000	8M	PCIe IO
	 */
	SET_TLB_ENTRY(1, CONFIG_SYS_CCSRBAR, CONFIG_SYS_CCSRBAR_PHYS,
		      MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		      0, 3, BOOKE_PAGESZ_64M, 1),

	/*
	 * TLB 4:	64M	Cacheable, non-guarded
	 * 0xf0000000	64M	LBC SDRAM First half
	 */
	SET_TLB_ENTRY(1, CONFIG_SYS_LBC_SDRAM_BASE, CONFIG_SYS_LBC_SDRAM_BASE,
		      MAS3_SX|MAS3_SW|MAS3_SR, 0,
		      0, 4, BOOKE_PAGESZ_64M, 1),

	/*
	 * TLB 5:	64M	Cacheable, non-guarded
	 * 0xf4000000	64M	LBC SDRAM Second half
	 */
	SET_TLB_ENTRY(1, CONFIG_SYS_LBC_SDRAM_BASE + 0x4000000,
		      CONFIG_SYS_LBC_SDRAM_BASE + 0x4000000,
		      MAS3_SX|MAS3_SW|MAS3_SR, 0,
		      0, 5, BOOKE_PAGESZ_64M, 1),

	/*
	 * TLB 6:	16M	Cacheable, non-guarded
	 * 0xf8000000	1M	7-segment LED display
	 * 0xf8100000	1M	User switches
	 * 0xf8300000	1M	Board revision
	 * 0xf8b00000	1M	EEPROM
	 */
	SET_TLB_ENTRY(1, CONFIG_SYS_EPLD_BASE, CONFIG_SYS_EPLD_BASE,
		      MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		      0, 6, BOOKE_PAGESZ_16M, 1),

	/*
	 * TLB 7:	4M	Non-cacheable, guarded
	 * 0xfb800000	4M	1st 4MB block of 64MB user FLASH
	 */
	SET_TLB_ENTRY(1, CONFIG_SYS_ALT_FLASH, CONFIG_SYS_ALT_FLASH,
		      MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		      0, 7, BOOKE_PAGESZ_4M, 1),

	/*
	 * TLB 8:	4M	Non-cacheable, guarded
	 * 0xfbc00000	4M	2nd 4MB block of 64MB user FLASH
	 */
	SET_TLB_ENTRY(1, CONFIG_SYS_ALT_FLASH + 0x400000,
		      CONFIG_SYS_ALT_FLASH + 0x400000,
		      MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		      0, 8, BOOKE_PAGESZ_4M, 1),

};

int num_tlb_entries = ARRAY_SIZE(tlb_table);
