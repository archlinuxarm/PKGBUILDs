/*
 * sh_eth.h - Driver for Renesas SH7763's gigabit ethernet controler.
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Copyright (c) 2008 Nobuhiro Iwamatsu
 * Copyright (c) 2007 Carlos Munoz <carlos@kenati.com>
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <netdev.h>
#include <asm/types.h>

#define SHETHER_NAME "sh_eth"

/* Malloc returns addresses in the P1 area (cacheable). However we need to
   use area P2 (non-cacheable) */
#define ADDR_TO_P2(addr)	((((int)(addr) & ~0xe0000000) | 0xa0000000))

/* The ethernet controller needs to use physical addresses */
#define ADDR_TO_PHY(addr)	((int)(addr) & ~0xe0000000)

/* Number of supported ports */
#define MAX_PORT_NUM	2

/* Buffers must be big enough to hold the largest ethernet frame. Also, rx
   buffers must be a multiple of 32 bytes */
#define MAX_BUF_SIZE	(48 * 32)

/* The number of tx descriptors must be large enough to point to 5 or more
   frames. If each frame uses 2 descriptors, at least 10 descriptors are needed.
   We use one descriptor per frame */
#define NUM_TX_DESC		8

/* The size of the tx descriptor is determined by how much padding is used.
   4, 20, or 52 bytes of padding can be used */
#define TX_DESC_PADDING		4
#define TX_DESC_SIZE		(12 + TX_DESC_PADDING)

/* Tx descriptor. We always use 3 bytes of padding */
struct tx_desc_s {
	volatile u32 td0;
	u32 td1;
	u32 td2;		/* Buffer start */
	u32 padding;
};

/* There is no limitation in the number of rx descriptors */
#define NUM_RX_DESC	8

/* The size of the rx descriptor is determined by how much padding is used.
   4, 20, or 52 bytes of padding can be used */
#define RX_DESC_PADDING		4
#define RX_DESC_SIZE		(12 + RX_DESC_PADDING)

/* Rx descriptor. We always use 4 bytes of padding */
struct rx_desc_s {
	volatile u32 rd0;
	volatile u32 rd1;
	u32 rd2;		/* Buffer start */
	u32 padding;
};

struct sh_eth_info {
	struct tx_desc_s *tx_desc_malloc;
	struct tx_desc_s *tx_desc_base;
	struct tx_desc_s *tx_desc_cur;
	struct rx_desc_s *rx_desc_malloc;
	struct rx_desc_s *rx_desc_base;
	struct rx_desc_s *rx_desc_cur;
	u8 *rx_buf_malloc;
	u8 *rx_buf_base;
	u8 mac_addr[6];
	u8 phy_addr;
	struct eth_device *dev;
};

struct sh_eth_dev {
	int port;
	struct sh_eth_info port_info[MAX_PORT_NUM];
};

/* Register Address */
#define BASE_IO_ADDR	0xfee00000

#define EDSR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0000)

#define TDLAR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0010)
#define TDFAR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0014)
#define TDFXR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0018)
#define TDFFR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x001c)

#define RDLAR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0030)
#define RDFAR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0034)
#define RDFXR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0038)
#define RDFFR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x003c)

#define EDMR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0400)
#define EDTRR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0408)
#define EDRRR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0410)
#define EESR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0428)
#define EESIPR(port)	(BASE_IO_ADDR + 0x800 * (port) + 0x0430)
#define TRSCER(port)	(BASE_IO_ADDR + 0x800 * (port) + 0x0438)
#define TFTR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0448)
#define FDR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0450)
#define RMCR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0458)
#define RPADIR(port)	(BASE_IO_ADDR + 0x800 * (port) + 0x0460)
#define FCFTR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0468)
#define ECMR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0500)
#define RFLR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0508)
#define ECSIPR(port)	(BASE_IO_ADDR + 0x800 * (port) + 0x0518)
#define PIR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0520)
#define PIPR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x052c)
#define APR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0554)
#define MPR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x0558)
#define TPAUSER(port)	(BASE_IO_ADDR + 0x800 * (port) + 0x0564)
#define GECMR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x05b0)
#define MALR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x05c8)
#define MAHR(port)		(BASE_IO_ADDR + 0x800 * (port) + 0x05c0)

/*
 * Register's bits
 * Copy from Linux driver source code
 */
#ifdef CONFIG_CPU_SH7763
/* EDSR */
enum EDSR_BIT {
	EDSR_ENT = 0x01, EDSR_ENR = 0x02,
};
#define EDSR_ENALL (EDSR_ENT|EDSR_ENR)
#endif

/* EDMR */
enum DMAC_M_BIT {
	EDMR_DL1 = 0x20, EDMR_DL0 = 0x10,
#ifdef CONFIG_CPU_SH7763
	EDMR_SRST	= 0x03,
	EMDR_DESC_R	= 0x30, /* Descriptor reserve size */
	EDMR_EL		= 0x40, /* Litte endian */
#else /* CONFIG_CPU_SH7763 */
	EDMR_SRST = 0x01,
#endif
};

/* RFLR */
#define RFLR_RFL_MIN	0x05EE	/* Recv Frame length 1518 byte */

/* EDTRR */
enum DMAC_T_BIT {
#ifdef CONFIG_CPU_SH7763
	EDTRR_TRNS = 0x03,
#else
	EDTRR_TRNS = 0x01,
#endif
};

/* GECMR */
enum GECMR_BIT {
	GECMR_1000B = 0x01, GECMR_100B = 0x04, GECMR_10B = 0x00,
};

/* EDRRR*/
enum EDRRR_R_BIT {
	EDRRR_R = 0x01,
};

/* TPAUSER */
enum TPAUSER_BIT {
	TPAUSER_TPAUSE = 0x0000ffff,
	TPAUSER_UNLIMITED = 0,
};

/* BCFR */
enum BCFR_BIT {
	BCFR_RPAUSE = 0x0000ffff,
	BCFR_UNLIMITED = 0,
};

/* PIR */
enum PIR_BIT {
	PIR_MDI = 0x08, PIR_MDO = 0x04, PIR_MMD = 0x02, PIR_MDC = 0x01,
};

/* PSR */
enum PHY_STATUS_BIT { PHY_ST_LINK = 0x01, };

/* EESR */
enum EESR_BIT {
#ifndef CONFIG_CPU_SH7763
	EESR_TWB  = 0x40000000,
#else
	EESR_TWB  = 0xC0000000,
	EESR_TC1  = 0x20000000,
	EESR_TUC  = 0x10000000,
	EESR_ROC  = 0x80000000,
#endif
	EESR_TABT = 0x04000000,
	EESR_RABT = 0x02000000, EESR_RFRMER = 0x01000000,
#ifndef CONFIG_CPU_SH7763
	EESR_ADE  = 0x00800000,
#endif
	EESR_ECI  = 0x00400000,
	EESR_FTC  = 0x00200000, EESR_TDE  = 0x00100000,
	EESR_TFE  = 0x00080000, EESR_FRC  = 0x00040000,
	EESR_RDE  = 0x00020000, EESR_RFE  = 0x00010000,
#ifndef CONFIG_CPU_SH7763
	EESR_CND  = 0x00000800,
#endif
	EESR_DLC  = 0x00000400,
	EESR_CD   = 0x00000200, EESR_RTO  = 0x00000100,
	EESR_RMAF = 0x00000080, EESR_CEEF = 0x00000040,
	EESR_CELF = 0x00000020, EESR_RRF  = 0x00000010,
	rESR_RTLF = 0x00000008, EESR_RTSF = 0x00000004,
	EESR_PRE  = 0x00000002, EESR_CERF = 0x00000001,
};


#ifdef CONFIG_CPU_SH7763
# define TX_CHECK (EESR_TC1 | EESR_FTC)
# define EESR_ERR_CHECK	(EESR_TWB | EESR_TABT | EESR_RABT | EESR_RDE \
		| EESR_RFRMER | EESR_TFE | EESR_TDE | EESR_ECI)
# define TX_ERROR_CEHCK (EESR_TWB | EESR_TABT | EESR_TDE | EESR_TFE)

#else
# define TX_CHECK (EESR_FTC | EESR_CND | EESR_DLC | EESR_CD | EESR_RTO)
# define EESR_ERR_CHECK	(EESR_TWB | EESR_TABT | EESR_RABT | EESR_RDE \
		| EESR_RFRMER | EESR_ADE | EESR_TFE | EESR_TDE | EESR_ECI)
# define TX_ERROR_CEHCK (EESR_TWB | EESR_TABT | EESR_ADE | EESR_TDE | EESR_TFE)
#endif

/* EESIPR */
enum DMAC_IM_BIT {
	DMAC_M_TWB = 0x40000000, DMAC_M_TABT = 0x04000000,
	DMAC_M_RABT = 0x02000000,
	DMAC_M_RFRMER = 0x01000000, DMAC_M_ADF = 0x00800000,
	DMAC_M_ECI = 0x00400000, DMAC_M_FTC = 0x00200000,
	DMAC_M_TDE = 0x00100000, DMAC_M_TFE = 0x00080000,
	DMAC_M_FRC = 0x00040000, DMAC_M_RDE = 0x00020000,
	DMAC_M_RFE = 0x00010000, DMAC_M_TINT4 = 0x00000800,
	DMAC_M_TINT3 = 0x00000400, DMAC_M_TINT2 = 0x00000200,
	DMAC_M_TINT1 = 0x00000100, DMAC_M_RINT8 = 0x00000080,
	DMAC_M_RINT5 = 0x00000010, DMAC_M_RINT4 = 0x00000008,
	DMAC_M_RINT3 = 0x00000004, DMAC_M_RINT2 = 0x00000002,
	DMAC_M_RINT1 = 0x00000001,
};

/* Receive descriptor bit */
enum RD_STS_BIT {
	RD_RACT = 0x80000000, RD_RDLE = 0x40000000,
	RD_RFP1 = 0x20000000, RD_RFP0 = 0x10000000,
	RD_RFE = 0x08000000, RD_RFS10 = 0x00000200,
	RD_RFS9 = 0x00000100, RD_RFS8 = 0x00000080,
	RD_RFS7 = 0x00000040, RD_RFS6 = 0x00000020,
	RD_RFS5 = 0x00000010, RD_RFS4 = 0x00000008,
	RD_RFS3 = 0x00000004, RD_RFS2 = 0x00000002,
	RD_RFS1 = 0x00000001,
};
#define RDF1ST	RD_RFP1
#define RDFEND	RD_RFP0
#define RD_RFP	(RD_RFP1|RD_RFP0)

/* RDFFR*/
enum RDFFR_BIT {
	RDFFR_RDLF = 0x01,
};

/* FCFTR */
enum FCFTR_BIT {
	FCFTR_RFF2 = 0x00040000, FCFTR_RFF1 = 0x00020000,
	FCFTR_RFF0 = 0x00010000, FCFTR_RFD2 = 0x00000004,
	FCFTR_RFD1 = 0x00000002, FCFTR_RFD0 = 0x00000001,
};
#define FIFO_F_D_RFF	(FCFTR_RFF2|FCFTR_RFF1|FCFTR_RFF0)
#define FIFO_F_D_RFD	(FCFTR_RFD2|FCFTR_RFD1|FCFTR_RFD0)

/* Transfer descriptor bit */
enum TD_STS_BIT {
#ifdef CONFIG_CPU_SH7763
	TD_TACT = 0x80000000,
#else
	TD_TACT = 0x7fffffff,
#endif
	TD_TDLE = 0x40000000, TD_TFP1 = 0x20000000,
	TD_TFP0 = 0x10000000,
};
#define TDF1ST	TD_TFP1
#define TDFEND	TD_TFP0
#define TD_TFP	(TD_TFP1|TD_TFP0)

/* RMCR */
enum RECV_RST_BIT { RMCR_RST = 0x01, };
/* ECMR */
enum FELIC_MODE_BIT {
#ifdef CONFIG_CPU_SH7763
	ECMR_TRCCM=0x04000000, ECMR_RCSC= 0x00800000, ECMR_DPAD= 0x00200000,
	ECMR_RZPF = 0x00100000,
#endif
	ECMR_ZPF = 0x00080000, ECMR_PFR = 0x00040000, ECMR_RXF = 0x00020000,
	ECMR_TXF = 0x00010000, ECMR_MCT = 0x00002000, ECMR_PRCEF = 0x00001000,
	ECMR_PMDE = 0x00000200, ECMR_RE = 0x00000040, ECMR_TE = 0x00000020,
	ECMR_ILB = 0x00000008, ECMR_ELB = 0x00000004, ECMR_DM = 0x00000002,
	ECMR_PRM = 0x00000001,
};

#ifdef CONFIG_CPU_SH7763
#define ECMR_CHG_DM	(ECMR_TRCCM | ECMR_RZPF | ECMR_ZPF | ECMR_PFR | ECMR_RXF | \
						ECMR_TXF | ECMR_MCT)
#else
#define ECMR_CHG_DM	(ECMR_ZPF | ECMR_PFR ECMR_RXF | ECMR_TXF | ECMR_MCT)
#endif

/* ECSR */
enum ECSR_STATUS_BIT {
#ifndef CONFIG_CPU_SH7763
	ECSR_BRCRX = 0x20, ECSR_PSRTO = 0x10,
#endif
	ECSR_LCHNG = 0x04,
	ECSR_MPD = 0x02, ECSR_ICD = 0x01,
};

#ifdef CONFIG_CPU_SH7763
# define ECSR_INIT (ECSR_ICD | ECSIPR_MPDIP)
#else
# define ECSR_INIT (ECSR_BRCRX | ECSR_PSRTO | \
			ECSR_LCHNG | ECSR_ICD | ECSIPR_MPDIP)
#endif

/* ECSIPR */
enum ECSIPR_STATUS_MASK_BIT {
#ifndef CONFIG_CPU_SH7763
	ECSIPR_BRCRXIP = 0x20, ECSIPR_PSRTOIP = 0x10,
#endif
	ECSIPR_LCHNGIP = 0x04,
	ECSIPR_MPDIP = 0x02, ECSIPR_ICDIP = 0x01,
};

#ifdef CONFIG_CPU_SH7763
# define ECSIPR_INIT (ECSIPR_LCHNGIP | ECSIPR_ICDIP | ECSIPR_MPDIP)
#else
# define ECSIPR_INIT (ECSIPR_BRCRXIP | ECSIPR_PSRTOIP | ECSIPR_LCHNGIP | \
				ECSIPR_ICDIP | ECSIPR_MPDIP)
#endif

/* APR */
enum APR_BIT {
	APR_AP = 0x00000004,
};

/* MPR */
enum MPR_BIT {
	MPR_MP = 0x00000006,
};

/* TRSCER */
enum DESC_I_BIT {
	DESC_I_TINT4 = 0x0800, DESC_I_TINT3 = 0x0400, DESC_I_TINT2 = 0x0200,
	DESC_I_TINT1 = 0x0100, DESC_I_RINT8 = 0x0080, DESC_I_RINT5 = 0x0010,
	DESC_I_RINT4 = 0x0008, DESC_I_RINT3 = 0x0004, DESC_I_RINT2 = 0x0002,
	DESC_I_RINT1 = 0x0001,
};

/* RPADIR */
enum RPADIR_BIT {
	RPADIR_PADS1 = 0x20000, RPADIR_PADS0 = 0x10000,
	RPADIR_PADR = 0x0003f,
};

#ifdef CONFIG_CPU_SH7763
# define RPADIR_INIT (0x00)
#else
# define RPADIR_INIT (RPADIR_PADS1)
#endif

/* FDR */
enum FIFO_SIZE_BIT {
	FIFO_SIZE_T = 0x00000700, FIFO_SIZE_R = 0x00000007,
};

enum PHY_OFFSETS {
	PHY_CTRL = 0, PHY_STAT = 1, PHY_IDT1 = 2, PHY_IDT2 = 3,
	PHY_ANA = 4, PHY_ANL = 5, PHY_ANE = 6,
	PHY_16 = 16,
};

/* PHY_CTRL */
enum PHY_CTRL_BIT {
	PHY_C_RESET = 0x8000, PHY_C_LOOPBK = 0x4000, PHY_C_SPEEDSL = 0x2000,
	PHY_C_ANEGEN = 0x1000, PHY_C_PWRDN = 0x0800, PHY_C_ISO = 0x0400,
	PHY_C_RANEG = 0x0200, PHY_C_DUPLEX = 0x0100, PHY_C_COLT = 0x0080,
};
#define DM9161_PHY_C_ANEGEN 0	/* auto nego special */

/* PHY_STAT */
enum PHY_STAT_BIT {
	PHY_S_100T4 = 0x8000, PHY_S_100X_F = 0x4000, PHY_S_100X_H = 0x2000,
	PHY_S_10T_F = 0x1000, PHY_S_10T_H = 0x0800, PHY_S_ANEGC = 0x0020,
	PHY_S_RFAULT = 0x0010, PHY_S_ANEGA = 0x0008, PHY_S_LINK = 0x0004,
	PHY_S_JAB = 0x0002, PHY_S_EXTD = 0x0001,
};

/* PHY_ANA */
enum PHY_ANA_BIT {
	PHY_A_NP = 0x8000, PHY_A_ACK = 0x4000, PHY_A_RF = 0x2000,
	PHY_A_FCS = 0x0400, PHY_A_T4 = 0x0200, PHY_A_FDX = 0x0100,
	PHY_A_HDX = 0x0080, PHY_A_10FDX = 0x0040, PHY_A_10HDX = 0x0020,
	PHY_A_SEL = 0x001e,
	PHY_A_EXT = 0x0001,
};

/* PHY_ANL */
enum PHY_ANL_BIT {
	PHY_L_NP = 0x8000, PHY_L_ACK = 0x4000, PHY_L_RF = 0x2000,
	PHY_L_FCS = 0x0400, PHY_L_T4 = 0x0200, PHY_L_FDX = 0x0100,
	PHY_L_HDX = 0x0080, PHY_L_10FDX = 0x0040, PHY_L_10HDX = 0x0020,
	PHY_L_SEL = 0x001f,
};

/* PHY_ANE */
enum PHY_ANE_BIT {
	PHY_E_PDF = 0x0010, PHY_E_LPNPA = 0x0008, PHY_E_NPA = 0x0004,
	PHY_E_PRX = 0x0002, PHY_E_LPANEGA = 0x0001,
};

/* DM9161 */
enum PHY_16_BIT {
	PHY_16_BP4B45 = 0x8000, PHY_16_BPSCR = 0x4000, PHY_16_BPALIGN = 0x2000,
	PHY_16_BP_ADPOK = 0x1000, PHY_16_Repeatmode = 0x0800,
	PHY_16_TXselect = 0x0400,
	PHY_16_Rsvd = 0x0200, PHY_16_RMIIEnable = 0x0100,
	PHY_16_Force100LNK = 0x0080,
	PHY_16_APDLED_CTL = 0x0040, PHY_16_COLLED_CTL = 0x0020,
	PHY_16_RPDCTR_EN = 0x0010,
	PHY_16_ResetStMch = 0x0008, PHY_16_PreamSupr = 0x0004,
	PHY_16_Sleepmode = 0x0002,
	PHY_16_RemoteLoopOut = 0x0001,
};
