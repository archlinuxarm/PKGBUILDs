/*
 * (C) Copyright 2002,2003, Motorola Inc.
 * Xianghua Xiao, (X.Xiao@motorola.com)
 *
 * (C) Copyright 2002 Scott McNutt <smcnutt@artesyncp.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <netdev.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/immap_85xx.h>
#include <asm/fsl_ddr_sdram.h>
#include <spd_sdram.h>

long int fixed_sdram (void);

int board_pre_init (void)
{
#if defined(CONFIG_PCI)
	volatile ccsr_pcix_t *pci = (void *)(CONFIG_SYS_MPC85xx_PCIX_ADDR);

	pci->peer &= 0xffffffdf; /* disable master abort */
#endif
	return 0;
}

int checkboard (void)
{
	sys_info_t sysinfo;

	get_sys_info (&sysinfo);

	printf ("Board: Freescale MPC8540EVAL Board\n");
	printf ("\tCPU: %lu MHz\n", sysinfo.freqProcessor[0] / 1000000);
	printf ("\tCCB: %lu MHz\n", sysinfo.freqSystemBus / 1000000);
	printf ("\tDDR: %lu MHz\n", sysinfo.freqSystemBus / 2000000);
	if((CONFIG_SYS_LBC_LCRR & 0x0f) == 2 || (CONFIG_SYS_LBC_LCRR & 0x0f) == 4 \
		|| (CONFIG_SYS_LBC_LCRR & 0x0f) == 8) {
		printf ("\tLBC: %lu MHz\n",
			sysinfo.freqSystemBus / 1000000/(CONFIG_SYS_LBC_LCRR & 0x0f));
	} else {
		printf("\tLBC: unknown\n");
	}
	printf("L1 D-cache 32KB, L1 I-cache 32KB enabled.\n");
	return (0);
}

phys_size_t initdram (int board_type)
{
	long dram_size = 0;

#if !defined(CONFIG_RAM_AS_FLASH)
	volatile ccsr_lbc_t *lbc = (void *)(CONFIG_SYS_MPC85xx_LBC_ADDR);
	sys_info_t sysinfo;
	uint temp_lbcdll = 0;
#endif
#if !defined(CONFIG_RAM_AS_FLASH) || defined(CONFIG_DDR_DLL)
	volatile ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
#endif

#if defined(CONFIG_DDR_DLL)
	uint temp_ddrdll = 0;

	/* Work around to stabilize DDR DLL */
	temp_ddrdll = gur->ddrdllcr;
	gur->ddrdllcr = ((temp_ddrdll & 0xff) << 16) | 0x80000000;
	asm("sync;isync;msync");
#endif

#if defined(CONFIG_SPD_EEPROM)
	dram_size = fsl_ddr_sdram();
	dram_size = setup_ddr_tlbs(dram_size / 0x100000);
	dram_size *= 0x100000;
#else
	dram_size = fixed_sdram ();
#endif

#if defined(CONFIG_SYS_RAMBOOT)
	return dram_size;
#endif

#if !defined(CONFIG_RAM_AS_FLASH) /* LocalBus is not emulating flash */
	get_sys_info(&sysinfo);
	/* if localbus freq is less than 66MHz,we use bypass mode,otherwise use DLL */
	if(sysinfo.freqSystemBus/(CONFIG_SYS_LBC_LCRR & LCRR_CLKDIV) < 66000000) {
		lbc->lcrr = (CONFIG_SYS_LBC_LCRR & 0x0fffffff)| 0x80000000;
	} else {
		lbc->lcrr = CONFIG_SYS_LBC_LCRR & 0x7fffffff;
		udelay(200);
		temp_lbcdll = gur->lbcdllcr;
		gur->lbcdllcr = ((temp_lbcdll & 0xff) << 16 ) | 0x80000000;
		asm("sync;isync;msync");
	}
	lbc->or2 = CONFIG_SYS_OR2_PRELIM; /* 64MB SDRAM */
	lbc->br2 = CONFIG_SYS_BR2_PRELIM;
	lbc->lbcr = CONFIG_SYS_LBC_LBCR;
	lbc->lsdmr = CONFIG_SYS_LBC_LSDMR_1;
	asm("sync");
	* (ulong *)0 = 0x000000ff;
	lbc->lsdmr = CONFIG_SYS_LBC_LSDMR_2;
	asm("sync");
	* (ulong *)0 = 0x000000ff;
	lbc->lsdmr = CONFIG_SYS_LBC_LSDMR_3;
	asm("sync");
	* (ulong *)0 = 0x000000ff;
	lbc->lsdmr = CONFIG_SYS_LBC_LSDMR_4;
	asm("sync");
	* (ulong *)0 = 0x000000ff;
	lbc->lsdmr = CONFIG_SYS_LBC_LSDMR_5;
	asm("sync");
	lbc->lsrt = CONFIG_SYS_LBC_LSRT;
	asm("sync");
	lbc->mrtpr = CONFIG_SYS_LBC_MRTPR;
	asm("sync");
#endif

#if defined(CONFIG_DDR_ECC)
	{
		/* Initialize all of memory for ECC, then
		 * enable errors */
		volatile ccsr_ddr_t *ddr= (void *)(CONFIG_SYS_MPC85xx_DDR_ADDR);

		dma_meminit(CONFIG_MEM_INIT_VALUE, dram_size);

		/* Enable errors for ECC */
		ddr->err_disable = 0x00000000;
		asm("sync;isync;msync");
	}
#endif

	return dram_size;
}

#if defined(CONFIG_SYS_DRAM_TEST)
int testdram (void)
{
	uint *pstart = (uint *) CONFIG_SYS_MEMTEST_START;
	uint *pend = (uint *) CONFIG_SYS_MEMTEST_END;
	uint *p;

	printf("SDRAM test phase 1:\n");
	for (p = pstart; p < pend; p++)
		*p = 0xaaaaaaaa;

	for (p = pstart; p < pend; p++) {
		if (*p != 0xaaaaaaaa) {
			printf ("SDRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("SDRAM test phase 2:\n");
	for (p = pstart; p < pend; p++)
		*p = 0x55555555;

	for (p = pstart; p < pend; p++) {
		if (*p != 0x55555555) {
			printf ("SDRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("SDRAM test passed.\n");
	return 0;
}
#endif

#if !defined(CONFIG_SPD_EEPROM)
/*************************************************************************
 *  fixed sdram init -- doesn't use serial presence detect.
 ************************************************************************/
long int fixed_sdram (void)
{
#ifndef CONFIG_SYS_RAMBOOT
	volatile ccsr_ddr_t *ddr= (void *)(CONFIG_SYS_MPC85xx_DDR_ADDR);

	ddr->cs0_bnds = CONFIG_SYS_DDR_CS0_BNDS;
	ddr->cs0_config = CONFIG_SYS_DDR_CS0_CONFIG;
	ddr->timing_cfg_1 = CONFIG_SYS_DDR_TIMING_1;
	ddr->timing_cfg_2 = CONFIG_SYS_DDR_TIMING_2;
	ddr->sdram_mode = CONFIG_SYS_DDR_MODE;
	ddr->sdram_interval = CONFIG_SYS_DDR_INTERVAL;
#if defined (CONFIG_DDR_ECC)
	ddr->err_disable = 0x0000000D;
	ddr->err_sbe = 0x00ff0000;
#endif
	asm("sync;isync;msync");
	udelay(500);
#if defined (CONFIG_DDR_ECC)
	/* Enable ECC checking */
	ddr->sdram_cfg = (CONFIG_SYS_DDR_CONTROL | 0x20000000);
#else
	ddr->sdram_cfg = CONFIG_SYS_DDR_CONTROL;
#endif
	asm("sync; isync; msync");
	udelay(500);
#endif
	return (CONFIG_SYS_SDRAM_SIZE * 1024 * 1024);
}
#endif	/* !defined(CONFIG_SPD_EEPROM) */

int board_eth_init(bd_t *bis)
{
	/*
	 * This board either has PCI NICs or uses the CPU's TSECs
	 * pci_eth_init() will return 0 if no NICs found, so in that case
	 * returning -1 will force cpu_eth_init() to be called.
	 */
	int num = pci_eth_init(bis);
	return (num <= 0 ? -1 : num);
}
