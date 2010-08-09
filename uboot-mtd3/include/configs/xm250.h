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

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_PXA250	       1	/* This is an PXA250 CPU	*/
#define CONFIG_XM250	       1	/* on a MicroSys XM250 Board	*/
#undef CONFIG_USE_IRQ			/* we don't need IRQ/FIQ stuff	*/

/* we will never enable dcache, because we have to setup MMU first */
#define CONFIG_SYS_NO_DCACHE

/*
 * Size of malloc() pool; this lives below the uppermost 128 KiB which are
 * used for the RAM copy of the uboot code
 *
 */
#define CONFIG_SYS_MALLOC_LEN		(256*1024)
#define CONFIG_SYS_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */

/*
 * Hardware drivers
 */
#define CONFIG_NET_MULTI
#define CONFIG_SMC91111
#define CONFIG_SMC91111_BASE		0x04000300
#undef	CONFIG_SMC91111_EXT_PHY
#define CONFIG_SMC_USE_32_BIT
#undef	CONFIG_SHOW_ACTIVITY
#define CONFIG_NET_RETRY_COUNT		10	   /* # of retries		*/

/*
 * I2C bus
 */
#define CONFIG_HARD_I2C			1
#define CONFIG_SYS_I2C_SPEED			50000
#define CONFIG_SYS_I2C_SLAVE			0xfe

#define CONFIG_RTC_PCF8563		1
#define CONFIG_SYS_I2C_RTC_ADDR		0x51

#define CONFIG_SYS_I2C_EEPROM_ADDR		0x58	/* A0 = 0 (hardwired)		*/
#define CONFIG_SYS_EEPROM_PAGE_WRITE_BITS	4	/* 4 bits = 16 octets		*/
#define CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS	10	/* between stop and start	*/
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN		1	/* length of address		*/
#define CONFIG_SYS_EEPROM_SIZE			2048	/* size in bytes		*/
#undef	CONFIG_SYS_I2C_INIT_BOARD			/* board has no own init	*/

/*
 * select serial console configuration
 */
#define CONFIG_PXA_SERIAL
#define CONFIG_FFUART	       1       /* we use FFUART */

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

#define CONFIG_BAUDRATE		115200


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

#define CONFIG_CMD_ELF
#define CONFIG_CMD_EEPROM
#define CONFIG_CMD_DATE
#define CONFIG_CMD_I2C


#define CONFIG_BOOTDELAY	3

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP				/* undef to save memory		*/
#define CONFIG_SYS_PROMPT		"=> "		/* Monitor Command Prompt	*/
#define CONFIG_SYS_CBSIZE		256		/* Console I/O Buffer Size	*/
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16) /* Print Buffer Size */
#define CONFIG_SYS_MAXARGS		16		/* max number of command args	*/
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size	*/

#define CONFIG_SYS_MEMTEST_START	0xa0400000	/* memtest works on		*/
#define CONFIG_SYS_MEMTEST_END		0xa0800000	/* 4 ... 8 MB in DRAM		*/

#define CONFIG_SYS_LOAD_ADDR		0xa3000000	/* default load address */

#define CONFIG_SYS_HZ			1000
#define CONFIG_SYS_CPUSPEED		0x161		/* set core clock to 400/400/100 MHz */

						/* valid baudrates */

#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*
 * Definitions related to passing arguments to kernel.
 */
#define CONFIG_CMDLINE_TAG	 1	 /* send commandline to Kernel		*/
#define CONFIG_SETUP_MEMORY_TAGS 1	 /* send memory definition to kernel	*/
#define	CONFIG_INITRD_TAG	 1	 /* do not send initrd params		*/
#undef	CONFIG_VFD			 /* do not send framebuffer setup	*/

/*
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128*1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */
#endif

/*
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	4
#define PHYS_SDRAM_1		0xa0000000 /* SDRAM Bank #1	*/
#define PHYS_SDRAM_1_SIZE	0x04000000 /* 64 MB		*/
#define PHYS_SDRAM_2		0xa4000000 /* SDRAM Bank #2	*/
#define PHYS_SDRAM_2_SIZE	0x00000000 /* 0 MB		*/
#define PHYS_SDRAM_3		0xa8000000 /* SDRAM Bank #3	*/
#define PHYS_SDRAM_3_SIZE	0x00000000 /* 0 MB		*/
#define PHYS_SDRAM_4		0xac000000 /* SDRAM Bank #4	*/
#define PHYS_SDRAM_4_SIZE	0x00000000 /* 0 MB		*/

#define PHYS_FLASH_1		0x00000000 /* Flash Bank #1	*/
#define PHYS_FLASH_2		0x04000000 /* Flash Bank #1	*/
#define PHYS_FLASH_SIZE		0x01000000 /* 16 MB		*/
#define PHYS_FLASH_BANK_SIZE	0x01000000 /* 16 MB Banks	*/
#define PHYS_FLASH_SECT_SIZE	0x00040000 /* 256 KB sectors (x2) */

#define CONFIG_SYS_DRAM_BASE		0xa0000000
#define CONFIG_SYS_DRAM_SIZE		0x04000000

#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH_1

/*
 * FLASH and environment organization
 */
#define CONFIG_SYS_MAX_FLASH_BANKS	1     /* max number of memory banks		*/
#define CONFIG_SYS_MAX_FLASH_SECT	128   /* max number of sectors on one chip	*/

/* timeout values are in ticks */
#define CONFIG_SYS_FLASH_ERASE_TOUT	(2*CONFIG_SYS_HZ)	/* Timeout for Flash Erase	*/
#define CONFIG_SYS_FLASH_WRITE_TOUT	(2*CONFIG_SYS_HZ)	/* Timeout for Flash Write	*/
#define CONFIG_SYS_FLASH_LOCK_TOUT	(2*CONFIG_SYS_HZ)	/* Timeout for Flash Set Lock Bit */
#define CONFIG_SYS_FLASH_UNLOCK_TOUT	(2*CONFIG_SYS_HZ)	/* Timeout for Flash Clear Lock Bits */
#define CONFIG_SYS_FLASH_PROTECTION			/* "Real" (hardware) sectors protection */

#define CONFIG_ENV_IS_IN_FLASH	1
#define CONFIG_ENV_ADDR		(PHYS_FLASH_1 + 0x40000)	/* Addr of Environment Sector	*/
#define CONFIG_ENV_SIZE		0x4000
#define CONFIG_ENV_SECT_SIZE	0x40000				/* Size of the Environment Sector	*/
#define CONFIG_SYS_MONITOR_LEN		0x20000				/* 128 KiB */

/******************************************************************************
 *
 * CPU specific defines
 *
 ******************************************************************************/

/*
 * GPIO settings
 *
 * GPIO pin assignments
 * GPIO	    Name	Dir Out AF
 * 0	    NC
 * 1	    NC
 * 2	    SIRQ1	I
 * 3	    SIRQ2	I
 * 4	    SIRQ3	I
 * 5	    DMAACK1	O   0
 * 6	    DMAACK2	O   0
 * 7	    DMAACK3	O   0
 * 8	    TC1		O   0
 * 9	    TC2		O   0
 * 10	    TC3		O   0
 * 11	    nDMAEN	O   1
 * 12	    AENCTRL	O   0
 * 13	    PLDTC	O   0
 * 14	    ETHIRQ	I
 * 15	    NC
 * 16	    NC
 * 17	    NC
 * 18	    RDY		I
 * 19	    DMASIO	I
 * 20	    ETHIRQ	NC
 * 21	    NC
 * 22	    PGMEN	O   1	 FIXME for debug only enable flash
 * 23	    NC
 * 24	    NC
 * 25	    NC
 * 26	    NC
 * 27	    NC
 * 28	    NC
 * 29	    NC
 * 30	    NC
 * 31	    NC
 * 32	    NC
 * 33	    NC
 * 34	    FFRXD	I	01
 * 35	    FFCTS	I	01
 * 36	    FFDCD	I	01
 * 37	    FFDSR	I	01
 * 38	    FFRI	I	01
 * 39	    FFTXD	O   1	10
 * 40	    FFDTR	O   0	10
 * 41	    FFRTS	O   0	10
 * 42	    RS232FOFF	O   0	00
 * 43	    NC
 * 44	    NC
 * 45	    IRSL0	O   0
 * 46	    IRRX0	I	01
 * 47	    IRTX0	O   0	10
 * 48	    NC
 * 49	    nIOWE	O   0
 * 50	    NC
 * 51	    NC
 * 52	    NC
 * 53	    NC
 * 54	    NC
 * 55	    NC
 * 56	    NC
 * 57	    NC
 * 58	    DKDIRQ	I
 * 59	    NC
 * 60	    NC
 * 61	    NC
 * 62	    NC
 * 63	    NC
 * 64	    COMLED	O   0
 * 65	    COMLED	O   0
 * 66	    COMLED	O   0
 * 67	    COMLED	O   0
 * 68	    COMLED	O   0
 * 69	    COMLED	O   0
 * 70	    COMLED	O   0
 * 71	    COMLED	O   0
 * 72	    NC
 * 73	    NC
 * 74	    NC
 * 75	    NC
 * 76	    NC
 * 77	    NC
 * 78	    CSIO	O   1
 * 79	    NC
 * 80	    CSETH	O   1
 *
 * NOTE: All NC's are defined to be outputs
 *
 */
/* Pin direction control */
#define CONFIG_SYS_GPDR0_VAL	    0xd3808000
#define CONFIG_SYS_GPDR1_VAL	    0xfcffab83
#define CONFIG_SYS_GPDR2_VAL	    0x0001ffff
/* Set and Clear registers */
#define CONFIG_SYS_GPSR0_VAL	    0x00008000
#define CONFIG_SYS_GPSR1_VAL	    0x00ff0002
#define CONFIG_SYS_GPSR2_VAL	    0x0001c000
#define CONFIG_SYS_GPCR0_VAL	    0x00000000
#define CONFIG_SYS_GPCR1_VAL	    0x00000000
#define CONFIG_SYS_GPCR2_VAL	    0x00000000
/* Edge detect registers (these are set by the kernel) */
#define CONFIG_SYS_GRER0_VAL	    0x00002180
#define CONFIG_SYS_GRER1_VAL	    0x00000000
#define CONFIG_SYS_GRER2_VAL	    0x00000000
#define CONFIG_SYS_GFER0_VAL	    0x000043e0
#define CONFIG_SYS_GFER1_VAL	    0x00000000
#define CONFIG_SYS_GFER2_VAL	    0x00000000
/* Alternate function registers */
#define CONFIG_SYS_GAFR0_L_VAL	    0x80000004
#define CONFIG_SYS_GAFR0_U_VAL	    0x595a8010
#define CONFIG_SYS_GAFR1_L_VAL	    0x699a9559
#define CONFIG_SYS_GAFR1_U_VAL	    0xaaa5aaaa
#define CONFIG_SYS_GAFR2_L_VAL	    0xaaaaaaaa
#define CONFIG_SYS_GAFR2_U_VAL	    0x00000002

/*
 * Clocks, power control and interrupts
 */
#define CONFIG_SYS_PSSR_VAL	    0x00000030
#define CONFIG_SYS_CCCR_VAL	    0x00000161	/* 100 MHz memory, 400 MHz CPU, 400 Turbo  */
#define CONFIG_SYS_CKEN_VAL	    0x000141ec	/* FFUART and STUART enabled	*/
#define CONFIG_SYS_ICMR_VAL	    0x00000000	/* No interrupts enabled	*/

/* FIXME
 *
 * RTC settings
 * Watchdog
 *
 */

/*
 * Memory settings
 *
 */
#define CONFIG_SYS_MSC0_VAL	    0x122423f0	/* FLASH   / LAN	    (cs0)/(cS1)	  */
#define CONFIG_SYS_MSC1_VAL	    0x35f4aa4c	/* USB	   / ST3+ST5	    (cs2)/(cS3)	  */
#define CONFIG_SYS_MSC2_VAL	    0x35f435fc	/* IDE	   / BCR + WatchDog (cs4)/(cS5)	  */
#define CONFIG_SYS_MDCNFG_VAL	    0x000009c9
#define CONFIG_SYS_MDMRS_VAL	    0x00220022
#define CONFIG_SYS_MDREFR_VAL	    0x000da018	/* Initial setting, individual bits set in lowlevel_init.S */

/*
 * PCMCIA and CF Interfaces (NOT USED, these values from lubbock init)
 */
#define CONFIG_SYS_MECR_VAL	      0x00000000
#define CONFIG_SYS_MCMEM0_VAL	      0x00010504
#define CONFIG_SYS_MCMEM1_VAL	      0x00010504
#define CONFIG_SYS_MCATT0_VAL	      0x00010504
#define CONFIG_SYS_MCATT1_VAL	      0x00010504
#define CONFIG_SYS_MCIO0_VAL	      0x00004715
#define CONFIG_SYS_MCIO1_VAL	      0x00004715

/* Board specific defines */

#ifndef __ASSEMBLY__

/* global prototypes */
void led_code(int code, int color);

#endif

#endif	/* __CONFIG_H */
