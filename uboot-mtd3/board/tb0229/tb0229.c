/*
 * Board initialize code for TANBAC Evaluation board TB0229.
 *
 * (C) Masami Komiya <mkomiya@sonare.it> 2004
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/reboot.h>
#include <pci.h>

void _machine_restart(void)
{
	void (*f)(void) = (void *) 0xbfc00000;

	f();
}

#if defined(CONFIG_PCI)
static struct pci_controller hose;

void pci_init_board (void)
{
	init_vr4131_pci(&hose);
}
#endif

phys_size_t initdram(int board_type)
{
	return get_ram_size (CONFIG_SYS_SDRAM_BASE, 0x8000000);
}

int checkboard (void)
{
	printf("Board: TANBAC TB0229 ");
	printf("(CPU Speed %d MHz)\n", (int)CPU_CLOCK_RATE/1000000);

	set_io_port_base(0);

	return 0;
}

int board_eth_init(bd_t *bis)
{
	return pci_eth_init(bis);
}
