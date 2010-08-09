/*
 * 2004-2005 Gary Jennejohn <garyj@denx.de>
 *
 * Modified for the MP2USB by (C) Copyright 2005 Eric Benard
 * ebenard@eukrea.com
 *
 * Configuration settings for the MP2USB board.
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

#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_AT91_LEGACY

/* ARM asynchronous clock */
#define AT91C_MAIN_CLOCK	179712000	/* from 18.432 MHz crystal (18432000 / 4 * 45) */
#define AT91C_MASTER_CLOCK	(AT91C_MAIN_CLOCK/3)	/* peripheral clock */

#define AT91_SLOW_CLOCK		32768	/* slow clock */

#define CONFIG_ARM920T		1	/* This is an ARM920T Core	*/
#define CONFIG_AT91RM9200	1	/* It's an Atmel AT91RM9200 SoC	*/
#define CONFIG_AT91RM9200DK	1	/* on an AT91RM9200DK Board	*/
#define CONFIG_MP2USB		1	/* on an MP2USB Board		*/
#undef  CONFIG_USE_IRQ			/* we don't need IRQ/FIQ stuff	*/
#define USE_920T_MMU		1

#define CONFIG_CMDLINE_TAG	1	/* enable passing of ATAGs	*/
#define CONFIG_SETUP_MEMORY_TAGS 1
#define CONFIG_INITRD_TAG	1

#define CONFIG_SYS_ATMEL_PLL_INIT_BUG	1
#ifndef CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_SYS_USE_MAIN_OSCILLATOR	1
/* flash */
#define CONFIG_SYS_EBI_CFGR_VAL	0x00000000
#define CONFIG_SYS_SMC_CSR0_VAL	0x00003084 /* 16bit, 2 TDF, 4 WS */

/* clocks */
#define CONFIG_SYS_PLLAR_VAL	0x20263E04 /* 180 MHz for PCK */
#define CONFIG_SYS_PLLBR_VAL	0x1048bE0E /* 48 MHz (divider by 2 for USB) */
#define CONFIG_SYS_MCKR_VAL	0x00000202 /* PCK/3 = MCK Master Clock = 60MHz from PLLA */

/* sdram */
#define CONFIG_SYS_PIOC_ASR_VAL	0xFFFF0000 /* Configure PIOC as peripheral (D16/D31) */
#define CONFIG_SYS_PIOC_BSR_VAL	0x00000000
#define CONFIG_SYS_PIOC_PDR_VAL	0xFFFF0000
#define CONFIG_SYS_EBI_CSA_VAL	0x00000002 /* CS1=CONFIG_SYS_SDRAM */
#define CONFIG_SYS_SDRC_CR_VAL	0x3211295A /* set up the CONFIG_SYS_SDRAM */
#define CONFIG_SYS_SDRAM	0x20000000 /* address of the CONFIG_SYS_SDRAM */
#define CONFIG_SYS_SDRAM1	0x20000020 /* address of the CONFIG_SYS_SDRAM */
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

#define CONFIG_BAUDRATE		115200

#define CONFIG_SYS_AT91C_BRGR_DIVISOR	33	/* hardcode so no __divsi3 : AT91C_MASTER_CLOCK /(baudrate * 16) */

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

#define CONFIG_USB_OHCI_NEW	1
#define CONFIG_USB_KEYBOARD	1
#define CONFIG_USB_STORAGE	1
#define CONFIG_DOS_PARTITION	1
#define CONFIG_AT91C_PQFP_UHPBUG 1

#undef CONFIG_SYS_USB_OHCI_BOARD_INIT
#define CONFIG_SYS_USB_OHCI_CPU_INIT		1
#define CONFIG_SYS_USB_OHCI_REGS_BASE		AT91_USB_HOST_BASE
#define CONFIG_SYS_USB_OHCI_SLOT_NAME		"at91rm9200"
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	15

#undef CONFIG_HARD_I2C

#ifdef CONFIG_HARD_I2C
#define CONFIG_SYS_I2C_SPEED		0	/* not used */
#define CONFIG_SYS_I2C_SLAVE		0	/* not used */
#define CONFIG_RTC_RS5C372A		/* RICOH I2C RTC */
#define CONFIG_SYS_I2C_RTC_ADDR	0x32
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x50
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN 1
#define CONFIG_SYS_I2C_EEPROM_ADDR_OVERFLOW
#endif
/* still about 20 kB free with this defined */
#define CONFIG_SYS_LONGHELP

#define CONFIG_BOOTDELAY      3

#if !defined(CONFIG_HARD_I2C)
#define CONFIG_TIMESTAMP
#endif


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
#define CONFIG_CMD_NFS
#define CONFIG_CMD_SNTP

#if defined(CONFIG_HARD_I2C)

    #define CONFIG_CMD_DATE
    #define CONFIG_CMD_EEPROM
    #define CONFIG_CMD_I2C
    #define CONFIG_CMD_MISC

#else

    #define CONFIG_CMD_CACHE
    #define CONFIG_CMD_USB

    #undef CONFIG_CMD_BDI
    #undef CONFIG_CMD_FPGA
    #undef CONFIG_CMD_IMI
    #undef CONFIG_CMD_LOADS
    #undef CONFIG_CMD_MISC
    #undef CONFIG_CMD_SOURCE

#endif


#define CONFIG_SYS_LONGHELP

#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM		0x20000000
#define PHYS_SDRAM_SIZE		0x08000000	/* 128 megs */

#define CONFIG_SYS_MEMTEST_START	PHYS_SDRAM
#define CONFIG_SYS_MEMTEST_END		CONFIG_SYS_MEMTEST_START + PHYS_SDRAM_SIZE - 262144

#define CONFIG_NET_MULTI		1
#ifdef CONFIG_NET_MULTI
#define CONFIG_DRIVER_AT91EMAC		1
#define CONFIG_SYS_RX_ETH_BUFFER	8
#else
#define CONFIG_DRIVER_ETHER		1
#endif
#define CONFIG_NET_RETRY_COUNT		20
#undef CONFIG_AT91C_USE_RMII

#define PHYS_FLASH_1			0x10000000
#define PHYS_FLASH_SIZE			0x1000000  /* 16 megs main flash */
#define CONFIG_SYS_FLASH_BASE			PHYS_FLASH_1
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_FLASH_BASE
#define CONFIG_SYS_MAX_FLASH_BANKS		1
#define CONFIG_SYS_MAX_FLASH_SECT		256
#define CONFIG_SYS_FLASH_ERASE_TOUT		(2 * CONFIG_SYS_HZ)	/* Timeout for Flash Erase */
#define CONFIG_SYS_FLASH_WRITE_TOUT		(2 * CONFIG_SYS_HZ)	/* Timeout for Flash Write */
#define CONFIG_SYS_FLASH_LOCK_TOUT		(10*CONFIG_SYS_HZ)	/* Timeout for Flash Set Lock Bit */
#define CONFIG_SYS_FLASH_UNLOCK_TOUT		(10*CONFIG_SYS_HZ)	/* Timeout for Flash Clear Lock Bits */
#define CONFIG_SYS_FLASH_PROTECTION				/* "Real" (hardware) sectors protection */

#define CONFIG_ENV_IS_IN_FLASH		1
#define CONFIG_ENV_OFFSET			0x20000		/* after u-boot.bin */
#define CONFIG_ENV_ADDR			(CONFIG_SYS_FLASH_BASE+CONFIG_ENV_OFFSET)
#define CONFIG_ENV_SIZE			0x20000

#define CONFIG_SYS_LOAD_ADDR		0x21000000  /* default load address */

#define CONFIG_SYS_BAUDRATE_TABLE	{ 115200, 57600, 38400, 19200, 9600 }

#define CONFIG_SYS_PROMPT		"=> "		/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		256		/* Console I/O Buffer Size */
#define CONFIG_SYS_MAXARGS		32		/* max number of command args */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16) /* Print Buffer Size */

#define CONFIG_SYS_STDIO_DEREGISTER           /* needs stdio_deregister */

#define CONFIG_SYS_HZ 1000
#define CONFIG_SYS_HZ_CLOCK (AT91C_MASTER_CLOCK/2)	/* AT91C_TC0_CMR is implicitly set to */
						/* AT91C_TC_TIMER_DIV1_CLOCK */

#define CONFIG_STACKSIZE	(32*1024)	/* regular stack */

#ifdef CONFIG_USE_IRQ
#error CONFIG_USE_IRQ not supported
#endif

#define CONFIG_SYS_DEVICE_NULLDEV	 1	/* enble null device		*/
#undef CONFIG_SILENT_CONSOLE		/* enable silent startup	*/

#define CONFIG_AUTOBOOT_KEYED
#define CONFIG_AUTOBOOT_PROMPT	\
	"Press SPACE to abort autoboot in %d seconds\n", bootdelay
#define CONFIG_AUTOBOOT_STOP_STR " "
#define CONFIG_AUTOBOOT_DELAY_STR "d"

#define CONFIG_VERSION_VARIABLE	1       /* include version env variable */

#endif	/* __CONFIG_H */
