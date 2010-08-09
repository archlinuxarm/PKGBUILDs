/*
 * Rick Bronson <rick@efn.org>
 *
 * Configuration settings for the AT91RM9200DK board.
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

#define CONFIG_AT91_LEGACY

/* ARM asynchronous clock */
#define AT91C_MAIN_CLOCK	179712000	/* from 18.432 MHz crystal (18432000 / 4 * 39) */
#define AT91C_MASTER_CLOCK	59904000	/* peripheral clock (AT91C_MASTER_CLOCK / 3) */
/* #define AT91C_MASTER_CLOCK	44928000 */	/* peripheral clock (AT91C_MASTER_CLOCK / 4) */

#define AT91_SLOW_CLOCK		32768	/* slow clock */

#define CONFIG_ARM920T		1	/* This is an ARM920T Core	*/
#define CONFIG_AT91RM9200	1	/* It's an Atmel AT91RM9200 SoC	*/
#define CONFIG_AT91RM9200DK	1	/* on an AT91RM9200DK Board	*/
#undef  CONFIG_USE_IRQ			/* we don't need IRQ/FIQ stuff	*/
#define USE_920T_MMU		1

#define CONFIG_CMDLINE_TAG	1	/* enable passing of ATAGs	*/
#define CONFIG_SETUP_MEMORY_TAGS 1
#define CONFIG_INITRD_TAG	1

#ifndef CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_SYS_USE_MAIN_OSCILLATOR		1
/* flash */
#define CONFIG_SYS_EBI_CFGR_VAL	0x00000000
#define CONFIG_SYS_SMC_CSR0_VAL	0x00003284 /* 16bit, 2 TDF, 4 WS */

/* clocks */
#define CONFIG_SYS_PLLAR_VAL	0x20263E04 /* 179.712000 MHz for PCK */
#define CONFIG_SYS_PLLBR_VAL	0x10483E0E /* 48.054857 MHz (divider by 2 for USB) */
#define CONFIG_SYS_MCKR_VAL	0x00000202 /* PCK/3 = MCK Master Clock = 59.904000MHz from PLLA */

/* sdram */
#define CONFIG_SYS_PIOC_ASR_VAL	0xFFFF0000 /* Configure PIOC as peripheral (D16/D31) */
#define CONFIG_SYS_PIOC_BSR_VAL	0x00000000
#define CONFIG_SYS_PIOC_PDR_VAL	0xFFFF0000
#define CONFIG_SYS_EBI_CSA_VAL	0x00000002 /* CS1=CONFIG_SYS_SDRAM */
#define CONFIG_SYS_SDRC_CR_VAL	0x2188c155 /* set up the CONFIG_SYS_SDRAM */
#define CONFIG_SYS_SDRAM	0x20000000 /* address of the CONFIG_SYS_SDRAM */
#define CONFIG_SYS_SDRAM1	0x20000080 /* address of the CONFIG_SYS_SDRAM */
#define CONFIG_SYS_SDRAM_VAL	0x00000000 /* value written to CONFIG_SYS_SDRAM */
#define CONFIG_SYS_SDRC_MR_VAL	0x00000002 /* Precharge All */
#define CONFIG_SYS_SDRC_MR_VAL1	0x00000004 /* refresh */
#define CONFIG_SYS_SDRC_MR_VAL2	0x00000003 /* Load Mode Register */
#define CONFIG_SYS_SDRC_MR_VAL3	0x00000000 /* Normal Mode */
#define CONFIG_SYS_SDRC_TR_VAL	0x000002E0 /* Write refresh rate */
#else
#define CONFIG_SKIP_RELOCATE_UBOOT
#endif	/* CONFIG_SKIP_LOWLEVEL_INIT */
/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN	(CONFIG_ENV_SIZE + 128*1024)
#define CONFIG_SYS_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */

#define CONFIG_BAUDRATE 115200

/*
 * Hardware drivers
 */

/* define one of these to choose the DBGU, USART0  or USART1 as console */
#define CONFIG_AT91RM9200_USART
#define CONFIG_DBGU
#undef CONFIG_USART0
#undef CONFIG_USART1

#undef	CONFIG_HWFLOW			/* don't include RTS/CTS flow control support	*/

#undef	CONFIG_MODEM_SUPPORT		/* disable modem initialization stuff */

#define CONFIG_BOOTDELAY      3
/* #define CONFIG_ENV_OVERWRITE	1 */


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

#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII

#include <asm/arch/AT91RM9200.h>	/* needed for port definitions */

#define CONFIG_NR_DRAM_BANKS 1
#define PHYS_SDRAM 0x20000000
#define PHYS_SDRAM_SIZE 0x2000000  /* 32 megs */

#define CONFIG_SYS_MEMTEST_START		PHYS_SDRAM
#define CONFIG_SYS_MEMTEST_END			CONFIG_SYS_MEMTEST_START + PHYS_SDRAM_SIZE - 262144

#define CONFIG_NET_MULTI		1
#ifdef CONFIG_NET_MULTI
#define CONFIG_DRIVER_AT91EMAC		1
#define CONFIG_SYS_RX_ETH_BUFFER	8
#else
#define CONFIG_DRIVER_ETHER		1
#endif

#define CONFIG_NET_RETRY_COUNT		20
#define CONFIG_AT91C_USE_RMII

/* AC Characteristics */
/* DLYBS = tCSS = 250ns min and DLYBCT = tCSH = 250ns */
#define DATAFLASH_TCSS	(0xC << 16)
#define DATAFLASH_TCHS	(0x1 << 24)

#define CONFIG_HAS_DATAFLASH		1
#define CONFIG_SYS_SPI_WRITE_TOUT		(5*CONFIG_SYS_HZ)
#define CONFIG_SYS_MAX_DATAFLASH_BANKS		2
#define CONFIG_SYS_MAX_DATAFLASH_PAGES		16384
#define CONFIG_SYS_DATAFLASH_LOGIC_ADDR_CS0	0xC0000000	/* Logical adress for CS0 */
#define CONFIG_SYS_DATAFLASH_LOGIC_ADDR_CS3	0xD0000000	/* Logical adress for CS3 */

#define PHYS_FLASH_1			0x10000000
#define PHYS_FLASH_SIZE			0x200000  /* 2 megs main flash */
#define CONFIG_SYS_FLASH_BASE			PHYS_FLASH_1
#define CONFIG_SYS_MAX_FLASH_BANKS		1
#define CONFIG_SYS_MAX_FLASH_SECT		256
#define CONFIG_SYS_FLASH_ERASE_TOUT		(2*CONFIG_SYS_HZ) /* Timeout for Flash Erase */
#define CONFIG_SYS_FLASH_WRITE_TOUT		(2*CONFIG_SYS_HZ) /* Timeout for Flash Write */

#undef	CONFIG_ENV_IS_IN_DATAFLASH

#ifdef CONFIG_ENV_IS_IN_DATAFLASH
#define CONFIG_ENV_OFFSET			0x20000
#define CONFIG_ENV_ADDR			(CONFIG_SYS_DATAFLASH_LOGIC_ADDR_CS0 + CONFIG_ENV_OFFSET)
#define CONFIG_ENV_SIZE			0x2000  /* 0x8000 */
#else
#define CONFIG_ENV_IS_IN_FLASH		1
#ifdef CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_ENV_ADDR			(PHYS_FLASH_1 + 0xe000)  /* between boot.bin and u-boot.bin.gz */
#define CONFIG_ENV_SIZE			0x2000  /* 0x8000 */
#else
#define CONFIG_ENV_ADDR			(PHYS_FLASH_1 + 0x60000)  /* after u-boot.bin */
#define CONFIG_ENV_SIZE			0x10000 /* sectors are 64K here */
#endif	/* CONFIG_SKIP_LOWLEVEL_INIT */
#endif	/* CONFIG_ENV_IS_IN_DATAFLASH */


#define CONFIG_SYS_LOAD_ADDR		0x21000000  /* default load address */

#ifdef CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_SYS_BOOT_SIZE		0x6000 /* 24 KBytes */
#define CONFIG_SYS_U_BOOT_BASE		(PHYS_FLASH_1 + 0x10000)
#define CONFIG_SYS_U_BOOT_SIZE		0x10000 /* 64 KBytes */
#else
#define CONFIG_SYS_BOOT_SIZE		0x00 /* 0 KBytes */
#define CONFIG_SYS_U_BOOT_BASE		PHYS_FLASH_1
#define CONFIG_SYS_U_BOOT_SIZE		0x60000 /* 384 KBytes */
#endif	/* CONFIG_SKIP_LOWLEVEL_INIT */

#define CONFIG_SYS_BAUDRATE_TABLE	{ 115200, 19200, 38400, 57600, 9600 }

#define CONFIG_SYS_PROMPT		"U-Boot> "	/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		256		/* Console I/O Buffer Size */
#define CONFIG_SYS_MAXARGS		16		/* max number of command args */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16) /* Print Buffer Size */

#define CONFIG_SYS_HZ 1000
#define CONFIG_SYS_HZ_CLOCK AT91C_MASTER_CLOCK/2	/* AT91C_TC0_CMR is implicitly set to */
						/* AT91C_TC_TIMER_DIV1_CLOCK */

#define CONFIG_STACKSIZE	(32*1024)	/* regular stack */

#ifdef CONFIG_USE_IRQ
#error CONFIG_USE_IRQ not supported
#endif

#endif
