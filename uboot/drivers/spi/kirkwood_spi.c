/*
 * (C) Copyright 2009
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Prafulla Wadaskar <prafulla@marvell.com>
 *
 * Derived from drivers/spi/mpc8xxx_spi.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/arch/kirkwood.h>
#include <asm/arch/spi.h>
#include <asm/arch/mpp.h>

static struct kwspi_registers *spireg = (struct kwspi_registers *)KW_SPI_BASE;

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
				unsigned int max_hz, unsigned int mode)
{
	struct spi_slave *slave;
	u32 data;
	u32 kwspi_mpp_config[] = {
		MPP0_GPIO,
		MPP7_SPI_SCn,
		0
	};

	if (!spi_cs_is_valid(bus, cs))
		return NULL;

	slave = malloc(sizeof(struct spi_slave));
	if (!slave)
		return NULL;

	slave->bus = bus;
	slave->cs = cs;

	writel(~KWSPI_CSN_ACT | KWSPI_SMEMRDY, &spireg->ctrl);

	/* calculate spi clock prescaller using max_hz */
	data = ((CONFIG_SYS_TCLK / 2) / max_hz) & KWSPI_CLKPRESCL_MASK;
	data |= 0x10;

	/* program spi clock prescaller using max_hz */
	writel(KWSPI_ADRLEN_3BYTE | data, &spireg->cfg);
	debug("data = 0x%08x \n", data);

	writel(KWSPI_SMEMRDIRQ, &spireg->irq_cause);
	writel(KWSPI_IRQMASK, spireg->irq_mask);

	/* program mpp registers to select  SPI_CSn */
	if (cs) {
		kwspi_mpp_config[0] = MPP0_GPIO;
		kwspi_mpp_config[1] = MPP7_SPI_SCn;
	} else {
		kwspi_mpp_config[0] = MPP0_SPI_SCn;
		kwspi_mpp_config[1] = MPP7_GPO;
	}
	kirkwood_mpp_conf(kwspi_mpp_config);

	return slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	free(slave);
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
}

#ifndef CONFIG_SPI_CS_IS_VALID
/*
 * you can define this function board specific
 * define above CONFIG in board specific config file and
 * provide the function in board specific src file
 */
int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return (bus == 0 && (cs == 0 || cs == 1));
}
#endif

void spi_cs_activate(struct spi_slave *slave)
{
	writel(readl(&spireg->ctrl) | KWSPI_IRQUNMASK, &spireg->ctrl);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	writel(readl(&spireg->ctrl) & KWSPI_IRQMASK, &spireg->ctrl);
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags)
{
	unsigned int tmpdout, tmpdin;
	int tm, isread = 0;

	debug("spi_xfer: slave %u:%u dout %08X din %08X bitlen %u\n",
	      slave->bus, slave->cs, dout, din, bitlen);

	if (flags & SPI_XFER_BEGIN)
		spi_cs_activate(slave);

	/*
	 * handle data in 8-bit chunks
	 * TBD: 2byte xfer mode to be enabled
	 */
	writel(((readl(&spireg->cfg) & ~KWSPI_XFERLEN_MASK) |
		KWSPI_XFERLEN_1BYTE), &spireg->cfg);

	while (bitlen > 4) {
		debug("loopstart bitlen %d\n", bitlen);
		tmpdout = 0;

		/* Shift data so it's msb-justified */
		if (dout)
			tmpdout = *(u32 *) dout & 0x0ff;

		writel(~KWSPI_SMEMRDIRQ, &spireg->irq_cause);
		writel(tmpdout, &spireg->dout);	/* Write the data out */
		debug("*** spi_xfer: ... %08x written, bitlen %d\n",
		      tmpdout, bitlen);

		/*
		 * Wait for SPI transmit to get out
		 * or time out (1 second = 1000 ms)
		 * The NE event must be read and cleared first
		 */
		for (tm = 0, isread = 0; tm < KWSPI_TIMEOUT; ++tm) {
			if (readl(&spireg->irq_cause) & KWSPI_SMEMRDIRQ) {
				isread = 1;
				tmpdin = readl(&spireg->din);
				debug
					("spi_xfer: din %08x..%08x read\n",
					din, tmpdin);

				if (din) {
					*((u8 *) din) = (u8) tmpdin;
					din += 1;
				}
				if (dout)
					dout += 1;
				bitlen -= 8;
			}
			if (isread)
				break;
		}
		if (tm >= KWSPI_TIMEOUT)
			printf("*** spi_xfer: Time out during SPI transfer\n");

		debug("loopend bitlen %d\n", bitlen);
	}

	if (flags & SPI_XFER_END)
		spi_cs_deactivate(slave);

	return 0;
}
