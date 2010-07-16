/*
 * (C) Copyright 2002
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
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

#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_PXA250			1		/* This is an PXA250 CPU    */
#define CONFIG_XSENGINE			1
#define CONFIG_MMC			1
#define CONFIG_DOS_PARTITION		1
#define BOARD_LATE_INIT			1
#undef  CONFIG_USE_IRQ					/* we don't need IRQ/FIQ stuff */
/* we will never enable dcache, because we have to setup MMU first */
#define CONFIG_SYS_NO_DCACHE

#define CONFIG_SYS_HZ			1000
#define CONFIG_SYS_CPUSPEED			0x161           /* set core clock to 400/200/100 MHz */

#define CONFIG_NR_DRAM_BANKS		1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM_1			0xa0000000	/* SDRAM Bank #1 */
#define PHYS_SDRAM_1_SIZE		0x04000000	/* 64 MB */
#define PHYS_SDRAM_2			0xa4000000	/* SDRAM Bank #2 */
#define PHYS_SDRAM_2_SIZE		0x00000000	/* 0 MB */
#define PHYS_SDRAM_3			0xa8000000	/* SDRAM Bank #3 */
#define PHYS_SDRAM_3_SIZE		0x00000000	/* 0 MB */
#define PHYS_SDRAM_4			0xac000000	/* SDRAM Bank #4 */
#define PHYS_SDRAM_4_SIZE		0x00000000	/* 0 MB */
#define CONFIG_SYS_DRAM_BASE			0xa0000000
#define CONFIG_SYS_DRAM_SIZE			0x04000000

/* FLASH organization */
#define CONFIG_SYS_MAX_FLASH_BANKS		1		/* max number of memory banks           */
#define CONFIG_SYS_MAX_FLASH_SECT		128		/* max number of sectors on one chip    */
#define PHYS_FLASH_1			0x00000000	/* Flash Bank #1 */
#define PHYS_FLASH_2			0x00000000	/* Flash Bank #2 */
#define PHYS_FLASH_SECT_SIZE		0x00020000	/* 127 KB sectors */
#define CONFIG_SYS_FLASH_BASE			PHYS_FLASH_1

/*
 * JFFS2 partitions
 */
/* No command line, one static partition, whole device */
#undef CONFIG_CMD_MTDPARTS
#define CONFIG_JFFS2_DEV		"nor0"
#define CONFIG_JFFS2_PART_SIZE		0xFFFFFFFF
#define CONFIG_JFFS2_PART_OFFSET	0x00000000

/* mtdparts command line support */
/* Note: fake mtd_id used, no linux mtd map file */
/*
#define CONFIG_CMD_MTDPARTS
#define MTDIDS_DEFAULT		"nor0=xsengine-0"
#define MTDPARTS_DEFAULT	"mtdparts=xsengine-0:256k(uboot),1m(kernel1),8m(kernel2)"
*/

/* Environment settings */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_ENV_IS_IN_FLASH             1
#define CONFIG_ENV_ADDR                    (PHYS_FLASH_1 + 0x40000)	/* Addr of Environment Sector (after monitor)*/
#define CONFIG_ENV_SECT_SIZE               PHYS_FLASH_SECT_SIZE		/* Size of the Environment Sector */
#define CONFIG_ENV_SIZE                    0x4000				/* 16kB Total Size of Environment Sector */

/* timeout values are in ticks */
#define CONFIG_SYS_FLASH_ERASE_TOUT		(75*CONFIG_SYS_HZ)	/* Timeout for Flash Erase */
#define CONFIG_SYS_FLASH_WRITE_TOUT		(50*CONFIG_SYS_HZ)	/* Timeout for Flash Write */

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN			(CONFIG_ENV_SIZE + 256*1024)
#define CONFIG_SYS_GBL_DATA_SIZE		128		/* size in bytes reserved for initial data */

/* Hardware drivers */
#define CONFIG_NET_MULTI
#define CONFIG_SMC91111
#define CONFIG_SMC91111_BASE		0x04000300
#define CONFIG_SMC_USE_32_BIT		1

/* select serial console configuration */
#define CONFIG_PXA_SERIAL
#define CONFIG_FFUART			1

/* allow to overwrite serial and ethaddr */
#define CONFIG_BAUDRATE			115200

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME


/*
 * Command line configuration.
 */
#include <config_cmd_default.h>

#define CONFIG_CMD_FAT
#define CONFIG_CMD_PING
#define CONFIG_CMD_JFFS2


#define CONFIG_BOOTDELAY		3
#define CONFIG_ETHADDR			FF:FF:FF:FF:FF:FF
#define CONFIG_NETMASK			255.255.255.0
#define CONFIG_IPADDR			192.168.1.50
#define CONFIG_SERVERIP			192.168.1.2
#define CONFIG_BOOTARGS			"root=/dev/mtdblock2 rootfstype=jffs2 console=ttyS1,115200"
#define CONFIG_CMDLINE_TAG

/* Miscellaneous configurable options */
#define CONFIG_SYS_HUSH_PARSER			1
#define CONFIG_SYS_PROMPT_HUSH_PS2		"> "
#define CONFIG_SYS_LONGHELP								/* undef to save memory	*/
#define CONFIG_SYS_PROMPT			"XS-Engine u-boot> "			/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE			256					/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE			(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS			16					/* max number of command args */
#define CONFIG_SYS_BARGSIZE			CONFIG_SYS_CBSIZE				/* Boot Argument Buffer Size */
#define CONFIG_SYS_MEMTEST_START		0xA0400000				/* memtest works on     */
#define CONFIG_SYS_MEMTEST_END			0xA0800000				/* 4 ... 8 MB in DRAM   */
#define CONFIG_SYS_BAUDRATE_TABLE		{ 9600, 19200, 38400, 57600, 115200 }	/* valid baudrates */
#define CONFIG_SYS_LOAD_ADDR			0xA0000000				/* load kernel to this address   */

#ifdef CONFIG_MMC
#define CONFIG_PXA_MMC
#define CONFIG_CMD_MMC
#define CONFIG_SYS_MMC_BASE			0xF0000000
#endif

/* Stack sizes - The stack sizes are set up in start.S using the settings below */
#define CONFIG_STACKSIZE		(128*1024)	/* regular stack */
#ifdef  CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ		(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ		(4*1024)	/* FIQ stack */
#endif

/* GP set register */
#define CONFIG_SYS_GPSR0_VAL			0x0000A000	/* CS1, PROG(FPGA) */
#define CONFIG_SYS_GPSR1_VAL			0x00020000	/* nPWE */
#define CONFIG_SYS_GPSR2_VAL			0x0000C000	/* CS2, CS3 */

/* GP clear register */
#define CONFIG_SYS_GPCR0_VAL			0x00000000
#define CONFIG_SYS_GPCR1_VAL			0x00000000
#define CONFIG_SYS_GPCR2_VAL			0x00000000

/* GP direction register */
#define CONFIG_SYS_GPDR0_VAL			0x0000A000	/* CS1, PROG(FPGA) */
#define CONFIG_SYS_GPDR1_VAL			0x00022A80	/* nPWE, FFUART + BTUART pins */
#define CONFIG_SYS_GPDR2_VAL			0x0000C000	/* CS2, CS3 */

/* GP rising edge detect register */
#define CONFIG_SYS_GRER0_VAL			0x00000000
#define CONFIG_SYS_GRER1_VAL			0x00000000
#define CONFIG_SYS_GRER2_VAL			0x00000000

/* GP falling edge detect register */
#define CONFIG_SYS_GFER0_VAL			0x00000000
#define CONFIG_SYS_GFER1_VAL			0x00000000
#define CONFIG_SYS_GFER2_VAL			0x00000000

/* GP alternate function register */
#define CONFIG_SYS_GAFR0_L_VAL			0x80000000	/* CS1 */
#define CONFIG_SYS_GAFR0_U_VAL			0x00000010	/* RDY */
#define CONFIG_SYS_GAFR1_L_VAL			0x09988050	/* FFUART + BTUART pins */
#define CONFIG_SYS_GAFR1_U_VAL			0x00000008	/* nPWE */
#define CONFIG_SYS_GAFR2_L_VAL			0xA0000000	/* CS2, CS3 */
#define CONFIG_SYS_GAFR2_U_VAL			0x00000000

#define CONFIG_SYS_PSSR_VAL			0x00000020	/* Power manager sleep status */
#define CONFIG_SYS_CCCR_VAL			0x00000161	/* 100 MHz memory, 400 MHz CPU  */
#define CONFIG_SYS_CKEN_VAL			0x000000C0	/* BTUART and FFUART enabled    */
#define CONFIG_SYS_ICMR_VAL			0x00000000	/* No interrupts enabled        */

/* Memory settings */
#define CONFIG_SYS_MSC0_VAL			0x25F425F0

/* MDCNFG: SDRAM Configuration Register */
#define CONFIG_SYS_MDCNFG_VAL			0x000009C9

/* MDREFR: SDRAM Refresh Control Register */
#define CONFIG_SYS_MDREFR_VAL			0x00018018

/* MDMRS: Mode Register Set Configuration Register */
#define CONFIG_SYS_MDMRS_VAL			0x00220022

#endif	/* __CONFIG_H */
