/*
 * Copyright 2004, 2007, 200 Freescale Semiconductor, Inc.
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
#include <pci.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/immap_85xx.h>
#include <asm/fsl_pci.h>
#include <asm/fsl_ddr_sdram.h>
#include <spd_sdram.h>
#include <miiphy.h>
#include <libfdt.h>
#include <fdt_support.h>

#include "../common/cadmus.h"
#include "../common/eeprom.h"
#include "../common/via.h"

DECLARE_GLOBAL_DATA_PTR;

void local_bus_init(void);
void sdram_init(void);

int checkboard (void)
{
	volatile ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	volatile ccsr_local_ecm_t *ecm = (void *)(CONFIG_SYS_MPC85xx_ECM_ADDR);

	/* PCI slot in USER bits CSR[6:7] by convention. */
	uint pci_slot = get_pci_slot ();

	uint cpu_board_rev = get_cpu_board_revision ();

	printf ("Board: CDS Version 0x%02x, PCI Slot %d\n",
		get_board_version (), pci_slot);

	printf ("CPU Board Revision %d.%d (0x%04x)\n",
		MPC85XX_CPU_BOARD_MAJOR (cpu_board_rev),
		MPC85XX_CPU_BOARD_MINOR (cpu_board_rev), cpu_board_rev);
	/*
	 * Initialize local bus.
	 */
	local_bus_init ();

	/*
	 * Hack TSEC 3 and 4 IO voltages.
	 */
	gur->tsec34ioovcr = 0xe7e0;	/*  1110 0111 1110 0xxx */

	ecm->eedr = 0xffffffff;		/* clear ecm errors */
	ecm->eeer = 0xffffffff;		/* enable ecm errors */
	return 0;
}

phys_size_t
initdram(int board_type)
{
	long dram_size = 0;

	puts("Initializing\n");

#if defined(CONFIG_DDR_DLL)
	{
		/*
		 * Work around to stabilize DDR DLL MSYNC_IN.
		 * Errata DDR9 seems to have been fixed.
		 * This is now the workaround for Errata DDR11:
		 *    Override DLL = 1, Course Adj = 1, Tap Select = 0
		 */

		volatile ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);

		gur->ddrdllcr = 0x81000000;
		asm("sync;isync;msync");
		udelay(200);
	}
#endif

	dram_size = fsl_ddr_sdram();
	dram_size = setup_ddr_tlbs(dram_size / 0x100000);
	dram_size *= 0x100000;

	/*
	 * SDRAM Initialization
	 */
	sdram_init();

	puts("    DDR: ");
	return dram_size;
}

/*
 * Initialize Local Bus
 */
void
local_bus_init(void)
{
	volatile ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	volatile ccsr_lbc_t *lbc = (void *)(CONFIG_SYS_MPC85xx_LBC_ADDR);

	uint clkdiv;
	uint lbc_hz;
	sys_info_t sysinfo;

	get_sys_info(&sysinfo);
	clkdiv = (lbc->lcrr & LCRR_CLKDIV) * 2;
	lbc_hz = sysinfo.freqSystemBus / 1000000 / clkdiv;

	gur->lbiuiplldcr1 = 0x00078080;
	if (clkdiv == 16) {
		gur->lbiuiplldcr0 = 0x7c0f1bf0;
	} else if (clkdiv == 8) {
		gur->lbiuiplldcr0 = 0x6c0f1bf0;
	} else if (clkdiv == 4) {
		gur->lbiuiplldcr0 = 0x5c0f1bf0;
	}

	lbc->lcrr |= 0x00030000;

	asm("sync;isync;msync");

	lbc->ltesr = 0xffffffff;	/* Clear LBC error interrupts */
	lbc->lteir = 0xffffffff;	/* Enable LBC error interrupts */
}

/*
 * Initialize SDRAM memory on the Local Bus.
 */
void
sdram_init(void)
{
#if defined(CONFIG_SYS_OR2_PRELIM) && defined(CONFIG_SYS_BR2_PRELIM)

	uint idx;
	volatile ccsr_lbc_t *lbc = (void *)(CONFIG_SYS_MPC85xx_LBC_ADDR);
	uint *sdram_addr = (uint *)CONFIG_SYS_LBC_SDRAM_BASE;
	uint cpu_board_rev;
	uint lsdmr_common;

	puts("    SDRAM: ");

	print_size (CONFIG_SYS_LBC_SDRAM_SIZE * 1024 * 1024, "\n");

	/*
	 * Setup SDRAM Base and Option Registers
	 */
	lbc->or2 = CONFIG_SYS_OR2_PRELIM;
	asm("msync");

	lbc->br2 = CONFIG_SYS_BR2_PRELIM;
	asm("msync");

	lbc->lbcr = CONFIG_SYS_LBC_LBCR;
	asm("msync");


	lbc->lsrt = CONFIG_SYS_LBC_LSRT;
	lbc->mrtpr = CONFIG_SYS_LBC_MRTPR;
	asm("msync");

	/*
	 * MPC8548 uses "new" 15-16 style addressing.
	 */
	cpu_board_rev = get_cpu_board_revision();
	lsdmr_common = CONFIG_SYS_LBC_LSDMR_COMMON;
	lsdmr_common |= LSDMR_BSMA1516;

	/*
	 * Issue PRECHARGE ALL command.
	 */
	lbc->lsdmr = lsdmr_common | LSDMR_OP_PCHALL;
	asm("sync;msync");
	*sdram_addr = 0xff;
	ppcDcbf((unsigned long) sdram_addr);
	udelay(100);

	/*
	 * Issue 8 AUTO REFRESH commands.
	 */
	for (idx = 0; idx < 8; idx++) {
		lbc->lsdmr = lsdmr_common | LSDMR_OP_ARFRSH;
		asm("sync;msync");
		*sdram_addr = 0xff;
		ppcDcbf((unsigned long) sdram_addr);
		udelay(100);
	}

	/*
	 * Issue 8 MODE-set command.
	 */
	lbc->lsdmr = lsdmr_common | LSDMR_OP_MRW;
	asm("sync;msync");
	*sdram_addr = 0xff;
	ppcDcbf((unsigned long) sdram_addr);
	udelay(100);

	/*
	 * Issue NORMAL OP command.
	 */
	lbc->lsdmr = lsdmr_common | LSDMR_OP_NORMAL;
	asm("sync;msync");
	*sdram_addr = 0xff;
	ppcDcbf((unsigned long) sdram_addr);
	udelay(200);    /* Overkill. Must wait > 200 bus cycles */

#endif	/* enable SDRAM init */
}

#if defined(CONFIG_PCI) || defined(CONFIG_PCI1)
/* For some reason the Tundra PCI bridge shows up on itself as a
 * different device.  Work around that by refusing to configure it.
 */
void dummy_func(struct pci_controller* hose, pci_dev_t dev, struct pci_config_table *tab) { }

static struct pci_config_table pci_mpc85xxcds_config_table[] = {
	{0x10e3, 0x0513, PCI_ANY_ID, 1, 3, PCI_ANY_ID, dummy_func, {0,0,0}},
	{0x1106, 0x0686, PCI_ANY_ID, 1, VIA_ID, 0, mpc85xx_config_via, {0,0,0}},
	{0x1106, 0x0571, PCI_ANY_ID, 1, VIA_ID, 1,
		mpc85xx_config_via_usbide, {0,0,0}},
	{0x1105, 0x3038, PCI_ANY_ID, 1, VIA_ID, 2,
		mpc85xx_config_via_usb, {0,0,0}},
	{0x1106, 0x3038, PCI_ANY_ID, 1, VIA_ID, 3,
		mpc85xx_config_via_usb2, {0,0,0}},
	{0x1106, 0x3058, PCI_ANY_ID, 1, VIA_ID, 5,
		mpc85xx_config_via_power, {0,0,0}},
	{0x1106, 0x3068, PCI_ANY_ID, 1, VIA_ID, 6,
		mpc85xx_config_via_ac97, {0,0,0}},
	{},
};

static struct pci_controller pci1_hose = {
	config_table: pci_mpc85xxcds_config_table};
#endif	/* CONFIG_PCI */

#ifdef CONFIG_PCI2
static struct pci_controller pci2_hose;
#endif	/* CONFIG_PCI2 */

#ifdef CONFIG_PCIE1
static struct pci_controller pcie1_hose;
#endif	/* CONFIG_PCIE1 */

void pci_init_board(void)
{
	volatile ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	struct fsl_pci_info pci_info[4];
	u32 devdisr, pordevsr, io_sel;
	u32 porpllsr, pci_agent, pci_speed, pci_32, pci_arb, pci_clk_sel;
	int first_free_busno = 0;
	int num = 0;

	int pcie_ep, pcie_configured;

	devdisr = in_be32(&gur->devdisr);
	pordevsr = in_be32(&gur->pordevsr);
	porpllsr = in_be32(&gur->porpllsr);
	io_sel = (pordevsr & MPC85xx_PORDEVSR_IO_SEL) >> 19;

	debug ("   pci_init_board: devdisr=%x, io_sel=%x\n", devdisr, io_sel);

#ifdef CONFIG_PCI1
	pci_speed = get_clock_freq ();	/* PCI PSPEED in [4:5] */
	pci_32 = pordevsr & MPC85xx_PORDEVSR_PCI1_PCI32;	/* PORDEVSR[15] */
	pci_arb = pordevsr & MPC85xx_PORDEVSR_PCI1_ARB;
	pci_clk_sel = porpllsr & MPC85xx_PORDEVSR_PCI1_SPD;

	if (!(devdisr & MPC85xx_DEVDISR_PCI1)) {
		SET_STD_PCI_INFO(pci_info[num], 1);
		pci_agent = fsl_setup_hose(&pci1_hose, pci_info[num].regs);
		printf ("\n    PCI: %d bit, %s MHz, %s, %s, %s (base address %lx)\n",
			(pci_32) ? 32 : 64,
			(pci_speed == 33333000) ? "33" :
			(pci_speed == 66666000) ? "66" : "unknown",
			pci_clk_sel ? "sync" : "async",
			pci_agent ? "agent" : "host",
			pci_arb ? "arbiter" : "external-arbiter",
			pci_info[num].regs);

		first_free_busno = fsl_pci_init_port(&pci_info[num++],
					&pci1_hose, first_free_busno);

#ifdef CONFIG_PCIX_CHECK
		if (!(pordevsr & MPC85xx_PORDEVSR_PCI1)) {
			/* PCI-X init */
			if (CONFIG_SYS_CLK_FREQ < 66000000)
				printf("PCI-X will only work at 66 MHz\n");

			reg16 = PCI_X_CMD_MAX_SPLIT | PCI_X_CMD_MAX_READ
				| PCI_X_CMD_ERO | PCI_X_CMD_DPERR_E;
			pci_hose_write_config_word(hose, bus, PCIX_COMMAND, reg16);
		}
#endif
	} else {
		printf ("    PCI: disabled\n");
	}

	puts("\n");
#else
	setbits_be32(&gur->devdisr, MPC85xx_DEVDISR_PCI1); /* disable */
#endif

#ifdef CONFIG_PCI2
{
	uint pci2_clk_sel = porpllsr & 0x4000;	/* PORPLLSR[17] */
	uint pci_dual = get_pci_dual ();	/* PCI DUAL in CM_PCI[3] */
	if (pci_dual) {
		printf ("    PCI2: 32 bit, 66 MHz, %s\n",
			pci2_clk_sel ? "sync" : "async");
	} else {
		printf ("    PCI2: disabled\n");
	}
}
#else
	setbits_be32(&gur->devdisr, MPC85xx_DEVDISR_PCI2); /* disable */
#endif /* CONFIG_PCI2 */

#ifdef CONFIG_PCIE1
	pcie_configured = is_fsl_pci_cfg(LAW_TRGT_IF_PCIE_1, io_sel);

	if (pcie_configured && !(devdisr & MPC85xx_DEVDISR_PCIE)){
		SET_STD_PCIE_INFO(pci_info[num], 1);
		pcie_ep = fsl_setup_hose(&pcie1_hose, pci_info[num].regs);
		printf ("    PCIE1 connected to Slot as %s (base addr %lx)\n",
				pcie_ep ? "Endpoint" : "Root Complex",
				pci_info[num].regs);

		first_free_busno = fsl_pci_init_port(&pci_info[num++],
					&pcie1_hose, first_free_busno);
	} else {
		printf ("    PCIE1: disabled\n");
	}

	puts("\n");
#else
	setbits_be32(&gur->devdisr, MPC85xx_DEVDISR_PCIE); /* disable */
#endif
}

int last_stage_init(void)
{
	unsigned short temp;

	/* Change the resistors for the PHY */
	/* This is needed to get the RGMII working for the 1.3+
	 * CDS cards */
	if (get_board_version() ==  0x13) {
		miiphy_write(CONFIG_TSEC1_NAME,
				TSEC1_PHY_ADDR, 29, 18);

		miiphy_read(CONFIG_TSEC1_NAME,
				TSEC1_PHY_ADDR, 30, &temp);

		temp = (temp & 0xf03f);
		temp |= 2 << 9;		/* 36 ohm */
		temp |= 2 << 6;		/* 39 ohm */

		miiphy_write(CONFIG_TSEC1_NAME,
				TSEC1_PHY_ADDR, 30, temp);

		miiphy_write(CONFIG_TSEC1_NAME,
				TSEC1_PHY_ADDR, 29, 3);

		miiphy_write(CONFIG_TSEC1_NAME,
				TSEC1_PHY_ADDR, 30, 0x8000);
	}

	return 0;
}


#if defined(CONFIG_OF_BOARD_SETUP)
void ft_pci_setup(void *blob, bd_t *bd)
{
#ifdef CONFIG_PCI1
	ft_fsl_pci_setup(blob, "pci0", &pci1_hose);
#endif
#ifdef CONFIG_PCIE1
	ft_fsl_pci_setup(blob, "pci1", &pcie1_hose);
#endif
}
#endif
