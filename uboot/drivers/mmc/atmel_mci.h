/*
 * Copyright (C) 2005-2006 Atmel Corporation
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
#ifndef __CPU_AT32AP_ATMEL_MCI_H__
#define __CPU_AT32AP_ATMEL_MCI_H__

/* Atmel MultiMedia Card Interface (MCI) registers */
#define MMCI_CR					0x0000
#define MMCI_MR					0x0004
#define MMCI_DTOR				0x0008
#define MMCI_SDCR				0x000c
#define MMCI_ARGR				0x0010
#define MMCI_CMDR				0x0014
#define MMCI_RSPR				0x0020
#define MMCI_RSPR1				0x0024
#define MMCI_RSPR2				0x0028
#define MMCI_RSPR3				0x002c
#define MMCI_RDR				0x0030
#define MMCI_TDR				0x0034
#define MMCI_SR					0x0040
#define MMCI_IER				0x0044
#define MMCI_IDR				0x0048
#define MMCI_IMR				0x004c

/* Bitfields in CR */
#define MMCI_MCIEN_OFFSET			0
#define MMCI_MCIEN_SIZE				1
#define MMCI_MCIDIS_OFFSET			1
#define MMCI_MCIDIS_SIZE			1
#define MMCI_PWSEN_OFFSET			2
#define MMCI_PWSEN_SIZE				1
#define MMCI_PWSDIS_OFFSET			3
#define MMCI_PWSDIS_SIZE			1
#define MMCI_SWRST_OFFSET			7
#define MMCI_SWRST_SIZE				1

/* Bitfields in MR */
#define MMCI_CLKDIV_OFFSET			0
#define MMCI_CLKDIV_SIZE			8
#define MMCI_PWSDIV_OFFSET			8
#define MMCI_PWSDIV_SIZE			3
#define MMCI_RDPROOF_OFFSET			11
#define MMCI_RDPROOF_SIZE			1
#define MMCI_WRPROOF_OFFSET			12
#define MMCI_WRPROOF_SIZE			1
#define MMCI_PDCPADV_OFFSET			14
#define MMCI_PDCPADV_SIZE			1
#define MMCI_PDCMODE_OFFSET			15
#define MMCI_PDCMODE_SIZE			1
#define MMCI_BLKLEN_OFFSET			16
#define MMCI_BLKLEN_SIZE			16

/* Bitfields in DTOR */
#define MMCI_DTOCYC_OFFSET			0
#define MMCI_DTOCYC_SIZE			4
#define MMCI_DTOMUL_OFFSET			4
#define MMCI_DTOMUL_SIZE			3

/* Bitfields in SDCR */
#define MMCI_SCDSEL_OFFSET			0
#define MMCI_SCDSEL_SIZE			4
#define MMCI_SCDBUS_OFFSET			7
#define MMCI_SCDBUS_SIZE			1

/* Bitfields in ARGR */
#define MMCI_ARG_OFFSET				0
#define MMCI_ARG_SIZE				32

/* Bitfields in CMDR */
#define MMCI_CMDNB_OFFSET			0
#define MMCI_CMDNB_SIZE				6
#define MMCI_RSPTYP_OFFSET			6
#define MMCI_RSPTYP_SIZE			2
#define MMCI_SPCMD_OFFSET			8
#define MMCI_SPCMD_SIZE				3
#define MMCI_OPDCMD_OFFSET			11
#define MMCI_OPDCMD_SIZE			1
#define MMCI_MAXLAT_OFFSET			12
#define MMCI_MAXLAT_SIZE			1
#define MMCI_TRCMD_OFFSET			16
#define MMCI_TRCMD_SIZE				2
#define MMCI_TRDIR_OFFSET			18
#define MMCI_TRDIR_SIZE				1
#define MMCI_TRTYP_OFFSET			19
#define MMCI_TRTYP_SIZE				2

/* Bitfields in RSPRx */
#define MMCI_RSP_OFFSET				0
#define MMCI_RSP_SIZE				32

/* Bitfields in SR/IER/IDR/IMR */
#define MMCI_CMDRDY_OFFSET			0
#define MMCI_CMDRDY_SIZE			1
#define MMCI_RXRDY_OFFSET			1
#define MMCI_RXRDY_SIZE				1
#define MMCI_TXRDY_OFFSET			2
#define MMCI_TXRDY_SIZE				1
#define MMCI_BLKE_OFFSET			3
#define MMCI_BLKE_SIZE				1
#define MMCI_DTIP_OFFSET			4
#define MMCI_DTIP_SIZE				1
#define MMCI_NOTBUSY_OFFSET			5
#define MMCI_NOTBUSY_SIZE			1
#define MMCI_ENDRX_OFFSET			6
#define MMCI_ENDRX_SIZE				1
#define MMCI_ENDTX_OFFSET			7
#define MMCI_ENDTX_SIZE				1
#define MMCI_RXBUFF_OFFSET			14
#define MMCI_RXBUFF_SIZE			1
#define MMCI_TXBUFE_OFFSET			15
#define MMCI_TXBUFE_SIZE			1
#define MMCI_RINDE_OFFSET			16
#define MMCI_RINDE_SIZE				1
#define MMCI_RDIRE_OFFSET			17
#define MMCI_RDIRE_SIZE				1
#define MMCI_RCRCE_OFFSET			18
#define MMCI_RCRCE_SIZE				1
#define MMCI_RENDE_OFFSET			19
#define MMCI_RENDE_SIZE				1
#define MMCI_RTOE_OFFSET			20
#define MMCI_RTOE_SIZE				1
#define MMCI_DCRCE_OFFSET			21
#define MMCI_DCRCE_SIZE				1
#define MMCI_DTOE_OFFSET			22
#define MMCI_DTOE_SIZE				1
#define MMCI_OVRE_OFFSET			30
#define MMCI_OVRE_SIZE				1
#define MMCI_UNRE_OFFSET			31
#define MMCI_UNRE_SIZE				1

/* Constants for DTOMUL */
#define MMCI_DTOMUL_1_CYCLE			0
#define MMCI_DTOMUL_16_CYCLES			1
#define MMCI_DTOMUL_128_CYCLES			2
#define MMCI_DTOMUL_256_CYCLES			3
#define MMCI_DTOMUL_1024_CYCLES			4
#define MMCI_DTOMUL_4096_CYCLES			5
#define MMCI_DTOMUL_65536_CYCLES		6
#define MMCI_DTOMUL_1048576_CYCLES		7

/* Constants for RSPTYP */
#define MMCI_RSPTYP_NO_RESP			0
#define MMCI_RSPTYP_48_BIT_RESP			1
#define MMCI_RSPTYP_136_BIT_RESP		2

/* Constants for SPCMD */
#define MMCI_SPCMD_NO_SPEC_CMD			0
#define MMCI_SPCMD_INIT_CMD			1
#define MMCI_SPCMD_SYNC_CMD			2
#define MMCI_SPCMD_INT_CMD			4
#define MMCI_SPCMD_INT_RESP			5

/* Constants for TRCMD */
#define MMCI_TRCMD_NO_TRANS			0
#define MMCI_TRCMD_START_TRANS			1
#define MMCI_TRCMD_STOP_TRANS			2

/* Constants for TRTYP */
#define MMCI_TRTYP_BLOCK			0
#define MMCI_TRTYP_MULTI_BLOCK			1
#define MMCI_TRTYP_STREAM			2

/* Bit manipulation macros */
#define MMCI_BIT(name)					\
	(1 << MMCI_##name##_OFFSET)
#define MMCI_BF(name,value)				\
	(((value) & ((1 << MMCI_##name##_SIZE) - 1))	\
	 << MMCI_##name##_OFFSET)
#define MMCI_BFEXT(name,value)				\
	(((value) >> MMCI_##name##_OFFSET)\
	 & ((1 << MMCI_##name##_SIZE) - 1))
#define MMCI_BFINS(name,value,old)			\
	(((old) & ~(((1 << MMCI_##name##_SIZE) - 1)	\
		    << MMCI_##name##_OFFSET))		\
	 | MMCI_BF(name,value))

/* Register access macros */
#define mmci_readl(reg)					\
	readl((void *)MMCI_BASE + MMCI_##reg)
#define mmci_writel(reg,value)				\
	writel((value), (void *)MMCI_BASE + MMCI_##reg)

#endif /* __CPU_AT32AP_ATMEL_MCI_H__ */
