/*
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Based on:
 *
 * -------------------------------------------------------------------------
 *
 *  linux/include/asm-arm/arch-davinci/hardware.h
 *
 *  Copyright (C) 2006 Texas Instruments.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <config.h>
#include <asm/sizes.h>

#define	REG(addr)	(*(volatile unsigned int *)(addr))
#define REG_P(addr)	((volatile unsigned int *)(addr))

typedef volatile unsigned int	dv_reg;
typedef volatile unsigned int *	dv_reg_p;

/*
 * Base register addresses
 *
 * NOTE:  some of these DM6446-specific addresses DO NOT WORK
 * on other DaVinci chips.  Double check them before you try
 * using the addresses ... or PSC module identifiers, etc.
 */
#ifndef CONFIG_SOC_DA8XX

#define DAVINCI_DMA_3PCC_BASE			(0x01c00000)
#define DAVINCI_DMA_3PTC0_BASE			(0x01c10000)
#define DAVINCI_DMA_3PTC1_BASE			(0x01c10400)
#define DAVINCI_UART0_BASE			(0x01c20000)
#define DAVINCI_UART1_BASE			(0x01c20400)
#define DAVINCI_I2C_BASE			(0x01c21000)
#define DAVINCI_TIMER0_BASE			(0x01c21400)
#define DAVINCI_TIMER1_BASE			(0x01c21800)
#define DAVINCI_WDOG_BASE			(0x01c21c00)
#define DAVINCI_PWM0_BASE			(0x01c22000)
#define DAVINCI_PWM1_BASE			(0x01c22400)
#define DAVINCI_PWM2_BASE			(0x01c22800)
#define DAVINCI_SYSTEM_MODULE_BASE		(0x01c40000)
#define DAVINCI_PLL_CNTRL0_BASE			(0x01c40800)
#define DAVINCI_PLL_CNTRL1_BASE			(0x01c40c00)
#define DAVINCI_PWR_SLEEP_CNTRL_BASE		(0x01c41000)
#define DAVINCI_ARM_INTC_BASE			(0x01c48000)
#define DAVINCI_USB_OTG_BASE			(0x01c64000)
#define DAVINCI_CFC_ATA_BASE			(0x01c66000)
#define DAVINCI_SPI_BASE			(0x01c66800)
#define DAVINCI_GPIO_BASE			(0x01c67000)
#define DAVINCI_VPSS_REGS_BASE			(0x01c70000)
#if !defined(CONFIG_SOC_DM646X)
#define DAVINCI_ASYNC_EMIF_DATA_CE0_BASE	(0x02000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE1_BASE	(0x04000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE2_BASE	(0x06000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE3_BASE	(0x08000000)
#endif
#define DAVINCI_DDR_BASE			(0x80000000)

#ifdef CONFIG_SOC_DM644X
#define DAVINCI_UART2_BASE			0x01c20800
#define DAVINCI_UHPI_BASE			0x01c67800
#define DAVINCI_EMAC_CNTRL_REGS_BASE		0x01c80000
#define DAVINCI_EMAC_WRAPPER_CNTRL_REGS_BASE	0x01c81000
#define DAVINCI_EMAC_WRAPPER_RAM_BASE		0x01c82000
#define DAVINCI_MDIO_CNTRL_REGS_BASE		0x01c84000
#define DAVINCI_IMCOP_BASE			0x01cc0000
#define DAVINCI_ASYNC_EMIF_CNTRL_BASE		0x01e00000
#define DAVINCI_VLYNQ_BASE			0x01e01000
#define DAVINCI_ASP_BASE			0x01e02000
#define DAVINCI_MMC_SD_BASE			0x01e10000
#define DAVINCI_MS_BASE				0x01e20000
#define DAVINCI_VLYNQ_REMOTE_BASE		0x0c000000

#elif defined(CONFIG_SOC_DM355)
#define DAVINCI_MMC_SD1_BASE			0x01e00000
#define DAVINCI_ASP0_BASE			0x01e02000
#define DAVINCI_ASP1_BASE			0x01e04000
#define DAVINCI_UART2_BASE			0x01e06000
#define DAVINCI_ASYNC_EMIF_CNTRL_BASE		0x01e10000
#define DAVINCI_MMC_SD0_BASE			0x01e11000

#elif defined(CONFIG_SOC_DM365)
#define DAVINCI_MMC_SD1_BASE			0x01d00000
#define DAVINCI_ASYNC_EMIF_CNTRL_BASE		0x01d10000
#define DAVINCI_MMC_SD0_BASE			0x01d11000

#elif defined(CONFIG_SOC_DM646X)
#define DAVINCI_ASYNC_EMIF_CNTRL_BASE		0x20008000
#define DAVINCI_ASYNC_EMIF_DATA_CE0_BASE	0x42000000
#define DAVINCI_ASYNC_EMIF_DATA_CE1_BASE	0x44000000
#define DAVINCI_ASYNC_EMIF_DATA_CE2_BASE	0x46000000
#define DAVINCI_ASYNC_EMIF_DATA_CE3_BASE	0x48000000

#endif

#else /* CONFIG_SOC_DA8XX */

#define DAVINCI_UART0_BASE			0x01c42000
#define DAVINCI_UART1_BASE			0x01d0c000
#define DAVINCI_UART2_BASE			0x01d0d000
#define DAVINCI_I2C0_BASE			0x01c22000
#define DAVINCI_I2C1_BASE			0x01e28000
#define DAVINCI_TIMER0_BASE			0x01c20000
#define DAVINCI_TIMER1_BASE			0x01c21000
#define DAVINCI_WDOG_BASE			0x01c21000
#define DAVINCI_PLL_CNTRL0_BASE			0x01c11000
#define DAVINCI_PSC0_BASE			0x01c10000
#define DAVINCI_PSC1_BASE			0x01e27000
#define DAVINCI_SPI0_BASE			0x01c41000
#define DAVINCI_USB_OTG_BASE			0x01e00000
#define DAVINCI_SPI1_BASE			0x01e12000
#define DAVINCI_GPIO_BASE			0x01e26000
#define DAVINCI_EMAC_CNTRL_REGS_BASE		0x01e23000
#define DAVINCI_EMAC_WRAPPER_CNTRL_REGS_BASE	0x01e22000
#define DAVINCI_EMAC_WRAPPER_RAM_BASE		0x01e20000
#define DAVINCI_MDIO_CNTRL_REGS_BASE		0x01e24000
#define DAVINCI_ASYNC_EMIF_CNTRL_BASE		0x68000000
#define DAVINCI_ASYNC_EMIF_DATA_CE0_BASE	0x40000000
#define DAVINCI_ASYNC_EMIF_DATA_CE2_BASE	0x60000000
#define DAVINCI_ASYNC_EMIF_DATA_CE3_BASE	0x62000000
#define DAVINCI_ASYNC_EMIF_DATA_CE4_BASE	0x64000000
#define DAVINCI_ASYNC_EMIF_DATA_CE5_BASE	0x66000000
#define DAVINCI_DDR_EMIF_CTRL_BASE		0xb0000000
#define DAVINCI_DDR_EMIF_DATA_BASE		0xc0000000
#define DAVINCI_INTC_BASE			0xfffee000
#define DAVINCI_BOOTCFG_BASE			0x01c14000

#endif /* CONFIG_SOC_DA8XX */

/* Power and Sleep Controller (PSC) Domains */
#define DAVINCI_GPSC_ARMDOMAIN		0
#define DAVINCI_GPSC_DSPDOMAIN		1

#ifndef CONFIG_SOC_DA8XX

#define DAVINCI_LPSC_VPSSMSTR		0
#define DAVINCI_LPSC_VPSSSLV		1
#define DAVINCI_LPSC_TPCC		2
#define DAVINCI_LPSC_TPTC0		3
#define DAVINCI_LPSC_TPTC1		4
#define DAVINCI_LPSC_EMAC		5
#define DAVINCI_LPSC_EMAC_WRAPPER	6
#define DAVINCI_LPSC_MDIO		7
#define DAVINCI_LPSC_IEEE1394		8
#define DAVINCI_LPSC_USB		9
#define DAVINCI_LPSC_ATA		10
#define DAVINCI_LPSC_VLYNQ		11
#define DAVINCI_LPSC_UHPI		12
#define DAVINCI_LPSC_DDR_EMIF		13
#define DAVINCI_LPSC_AEMIF		14
#define DAVINCI_LPSC_MMC_SD		15
#define DAVINCI_LPSC_MEMSTICK		16
#define DAVINCI_LPSC_McBSP		17
#define DAVINCI_LPSC_I2C		18
#define DAVINCI_LPSC_UART0		19
#define DAVINCI_LPSC_UART1		20
#define DAVINCI_LPSC_UART2		21
#define DAVINCI_LPSC_SPI		22
#define DAVINCI_LPSC_PWM0		23
#define DAVINCI_LPSC_PWM1		24
#define DAVINCI_LPSC_PWM2		25
#define DAVINCI_LPSC_GPIO		26
#define DAVINCI_LPSC_TIMER0		27
#define DAVINCI_LPSC_TIMER1		28
#define DAVINCI_LPSC_TIMER2		29
#define DAVINCI_LPSC_SYSTEM_SUBSYS	30
#define DAVINCI_LPSC_ARM		31
#define DAVINCI_LPSC_SCR2		32
#define DAVINCI_LPSC_SCR3		33
#define DAVINCI_LPSC_SCR4		34
#define DAVINCI_LPSC_CROSSBAR		35
#define DAVINCI_LPSC_CFG27		36
#define DAVINCI_LPSC_CFG3		37
#define DAVINCI_LPSC_CFG5		38
#define DAVINCI_LPSC_GEM		39
#define DAVINCI_LPSC_IMCOP		40

#define DAVINCI_DM646X_LPSC_EMAC	14
#define DAVINCI_DM646X_LPSC_UART0	26
#define DAVINCI_DM646X_LPSC_I2C		31

#else /* CONFIG_SOC_DA8XX */

enum davinci_lpsc_ids {
	DAVINCI_LPSC_TPCC = 0,
	DAVINCI_LPSC_TPTC0,
	DAVINCI_LPSC_TPTC1,
	DAVINCI_LPSC_AEMIF,
	DAVINCI_LPSC_SPI0,
	DAVINCI_LPSC_MMC_SD,
	DAVINCI_LPSC_AINTC,
	DAVINCI_LPSC_ARM_RAM_ROM,
	DAVINCI_LPSC_SECCTL_KEYMGR,
	DAVINCI_LPSC_UART0,
	DAVINCI_LPSC_SCR0,
	DAVINCI_LPSC_SCR1,
	DAVINCI_LPSC_SCR2,
	DAVINCI_LPSC_DMAX,
	DAVINCI_LPSC_ARM,
	DAVINCI_LPSC_GEM,
	/* for LPSCs in PSC1, offset from 32 for differentiation */
	DAVINCI_LPSC_PSC1_BASE = 32,
	DAVINCI_LPSC_USB11,
	DAVINCI_LPSC_USB20,
	DAVINCI_LPSC_GPIO,
	DAVINCI_LPSC_UHPI,
	DAVINCI_LPSC_EMAC,
	DAVINCI_LPSC_DDR_EMIF,
	DAVINCI_LPSC_McASP0,
	DAVINCI_LPSC_McASP1,
	DAVINCI_LPSC_McASP2,
	DAVINCI_LPSC_SPI1,
	DAVINCI_LPSC_I2C1,
	DAVINCI_LPSC_UART1,
	DAVINCI_LPSC_UART2,
	DAVINCI_LPSC_LCDC,
	DAVINCI_LPSC_ePWM,
	DAVINCI_LPSC_eCAP,
	DAVINCI_LPSC_eQEP,
	DAVINCI_LPSC_SCR_P0,
	DAVINCI_LPSC_SCR_P1,
	DAVINCI_LPSC_CR_P3,
	DAVINCI_LPSC_L3_CBA_RAM
};

#endif /* CONFIG_SOC_DA8XX */

void lpsc_on(unsigned int id);
void dsp_on(void);

void davinci_enable_uart0(void);
void davinci_enable_emac(void);
void davinci_enable_i2c(void);
void davinci_errata_workarounds(void);

#ifndef CONFIG_SOC_DA8XX

/* Some PSC defines */
#define PSC_CHP_SHRTSW			(0x01c40038)
#define PSC_GBLCTL			(0x01c41010)
#define PSC_EPCPR			(0x01c41070)
#define PSC_EPCCR			(0x01c41078)
#define PSC_PTCMD			(0x01c41120)
#define PSC_PTSTAT			(0x01c41128)
#define PSC_PDSTAT			(0x01c41200)
#define PSC_PDSTAT1			(0x01c41204)
#define PSC_PDCTL			(0x01c41300)
#define PSC_PDCTL1			(0x01c41304)

#define PSC_MDCTL_BASE			(0x01c41a00)
#define PSC_MDSTAT_BASE			(0x01c41800)

#define VDD3P3V_PWDN			(0x01c40048)
#define UART0_PWREMU_MGMT		(0x01c20030)

#define PSC_SILVER_BULLET		(0x01c41a20)

#else /* CONFIG_SOC_DA8XX */

#define PSC_PSC0_MODULE_ID_CNT		16
#define PSC_PSC1_MODULE_ID_CNT		32

struct davinci_psc_regs {
	dv_reg	revid;
	dv_reg	rsvd0[71];
	dv_reg	ptcmd;
	dv_reg	rsvd1;
	dv_reg	ptstat;
	dv_reg	rsvd2[437];
	union {
		struct {
			dv_reg	mdstat[PSC_PSC0_MODULE_ID_CNT];
			dv_reg	rsvd3[112];
			dv_reg	mdctl[PSC_PSC0_MODULE_ID_CNT];
		} psc0;
		struct {
			dv_reg	mdstat[PSC_PSC1_MODULE_ID_CNT];
			dv_reg	rsvd3[96];
			dv_reg	mdctl[PSC_PSC1_MODULE_ID_CNT];
		} psc1;
	};
};

#define davinci_psc0_regs ((struct davinci_psc_regs *)DAVINCI_PSC0_BASE)
#define davinci_psc1_regs ((struct davinci_psc_regs *)DAVINCI_PSC1_BASE)

#endif /* CONFIG_SOC_DA8XX */

#ifndef CONFIG_SOC_DA8XX

/* Miscellania... */
#define VBPR				(0x20000020)

/* NOTE:  system control modules are *highly* chip-specific, both
 * as to register content (e.g. for muxing) and which registers exist.
 */
#define PINMUX0				0x01c40000
#define PINMUX1				0x01c40004
#define PINMUX2				0x01c40008
#define PINMUX3				0x01c4000c
#define PINMUX4				0x01c40010

#else /* CONFIG_SOC_DA8XX */

struct davinci_pllc_regs {
	dv_reg	revid;
	dv_reg	rsvd1[56];
	dv_reg	rstype;
	dv_reg	rsvd2[6];
	dv_reg	pllctl;
	dv_reg	ocsel;
	dv_reg	rsvd3[2];
	dv_reg	pllm;
	dv_reg	prediv;
	dv_reg	plldiv1;
	dv_reg	plldiv2;
	dv_reg	plldiv3;
	dv_reg	oscdiv;
	dv_reg	postdiv;
	dv_reg	rsvd4[3];
	dv_reg	pllcmd;
	dv_reg	pllstat;
	dv_reg	alnctl;
	dv_reg	dchange;
	dv_reg	cken;
	dv_reg	ckstat;
	dv_reg	systat;
	dv_reg	rsvd5[3];
	dv_reg	plldiv4;
	dv_reg	plldiv5;
	dv_reg	plldiv6;
	dv_reg	plldiv7;
	dv_reg	rsvd6[32];
	dv_reg	emucnt0;
	dv_reg	emucnt1;
};

#define davinci_pllc_regs ((struct davinci_pllc_regs *)DAVINCI_PLL_CNTRL0_BASE)
#define DAVINCI_PLLC_DIV_MASK	0x1f

/* Clock IDs */
enum davinci_clk_ids {
	DAVINCI_SPI0_CLKID = 2,
	DAVINCI_UART2_CLKID = 2,
	DAVINCI_MDIO_CLKID = 4,
	DAVINCI_ARM_CLKID = 6,
	DAVINCI_PLLM_CLKID = 0xff,
	DAVINCI_PLLC_CLKID = 0x100,
	DAVINCI_AUXCLK_CLKID = 0x101
};

int clk_get(enum davinci_clk_ids id);

/* Boot config */
struct davinci_syscfg_regs {
	dv_reg	revid;
	dv_reg	rsvd[71];
	dv_reg	pinmux[20];
	dv_reg	suspsrc;
	dv_reg	chipsig;
	dv_reg	chipsig_clr;
	dv_reg	cfgchip0;
	dv_reg	cfgchip1;
	dv_reg	cfgchip2;
	dv_reg	cfgchip3;
	dv_reg	cfgchip4;
};

#define davinci_syscfg_regs \
	((struct davinci_syscfg_regs *)DAVINCI_BOOTCFG_BASE)

/* Emulation suspend bits */
#define DAVINCI_SYSCFG_SUSPSRC_EMAC		(1 << 5)
#define DAVINCI_SYSCFG_SUSPSRC_I2C		(1 << 16)
#define DAVINCI_SYSCFG_SUSPSRC_SPI0		(1 << 21)
#define DAVINCI_SYSCFG_SUSPSRC_SPI1		(1 << 22)
#define DAVINCI_SYSCFG_SUSPSRC_UART2		(1 << 20)
#define DAVINCI_SYSCFG_SUSPSRC_TIMER0		(1 << 27)

/* Interrupt controller */
struct davinci_aintc_regs {
	dv_reg	revid;
	dv_reg	cr;
	dv_reg	dummy0[2];
	dv_reg	ger;
	dv_reg	dummy1[219];
	dv_reg	ecr1;
	dv_reg	ecr2;
	dv_reg	ecr3;
	dv_reg	dummy2[1117];
	dv_reg	hier;
};

#define davinci_aintc_regs ((struct davinci_aintc_regs *)DAVINCI_INTC_BASE)

struct davinci_uart_ctrl_regs {
	dv_reg	revid1;
	dv_reg	revid2;
	dv_reg	pwremu_mgmt;
	dv_reg	mdr;
};

#define DAVINCI_UART_CTRL_BASE 0x28
#define DAVINCI_UART0_CTRL_ADDR (DAVINCI_UART0_BASE + DAVINCI_UART_CTRL_BASE)
#define DAVINCI_UART1_CTRL_ADDR (DAVINCI_UART1_BASE + DAVINCI_UART_CTRL_BASE)
#define DAVINCI_UART2_CTRL_ADDR (DAVINCI_UART2_BASE + DAVINCI_UART_CTRL_BASE)

#define davinci_uart0_ctrl_regs \
	((struct davinci_uart_ctrl_regs *)DAVINCI_UART0_CTRL_ADDR)
#define davinci_uart1_ctrl_regs \
	((struct davinci_uart_ctrl_regs *)DAVINCI_UART1_CTRL_ADDR)
#define davinci_uart2_ctrl_regs \
	((struct davinci_uart_ctrl_regs *)DAVINCI_UART2_CTRL_ADDR)

/* UART PWREMU_MGMT definitions */
#define DAVINCI_UART_PWREMU_MGMT_FREE	(1 << 0)
#define DAVINCI_UART_PWREMU_MGMT_URRST	(1 << 13)
#define DAVINCI_UART_PWREMU_MGMT_UTRST	(1 << 14)

#endif /* CONFIG_SOC_DA8XX */

#endif /* __ASM_ARCH_HARDWARE_H */
