/*
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

#include <common.h>
#include <netdev.h>

DECLARE_GLOBAL_DATA_PTR;

/* ------------------------------------------------------------------------- */


/*
 * Miscelaneous platform dependent initialisations
 */

int board_init (void)
{
	/* memory and cpu-speed are setup before relocation */
	/* so we do _nothing_ here */

	/* arch number of LART-Board */
	gd->bd->bi_arch_number = MACH_TYPE_LART;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0xc0000100;

	return 0;
}

int dram_init (void)
{
	bd_t *bd = gd->bd;

	bd->bi_dram[0].start = PHYS_SDRAM_1;
	bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	bd->bi_dram[1].start = PHYS_SDRAM_2;
	bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
	bd->bi_dram[2].start = PHYS_SDRAM_3;
	bd->bi_dram[2].size = PHYS_SDRAM_3_SIZE;
	bd->bi_dram[3].start = PHYS_SDRAM_4;
	bd->bi_dram[3].size = PHYS_SDRAM_4_SIZE;

	return (0);
}

#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_CS8900
	rc = cs8900_initialize(0, CONFIG_CS8900_BASE);
#endif
	return rc;
}
#endif
