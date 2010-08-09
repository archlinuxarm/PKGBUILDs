/*
 * (C) Copyright 2001-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
#include <command.h>
#include <net.h>
#include <miiphy.h>

void eth_parse_enetaddr(const char *addr, uchar *enetaddr)
{
	char *end;
	int i;

	for (i = 0; i < 6; ++i) {
		enetaddr[i] = addr ? simple_strtoul(addr, &end, 16) : 0;
		if (addr)
			addr = (*end) ? end + 1 : end;
	}
}

int eth_getenv_enetaddr(char *name, uchar *enetaddr)
{
	eth_parse_enetaddr(getenv(name), enetaddr);
	return is_valid_ether_addr(enetaddr);
}

int eth_setenv_enetaddr(char *name, const uchar *enetaddr)
{
	char buf[20];

	sprintf(buf, "%pM", enetaddr);

	return setenv(name, buf);
}

int eth_getenv_enetaddr_by_index(int index, uchar *enetaddr)
{
	char enetvar[32];
	sprintf(enetvar, index ? "eth%daddr" : "ethaddr", index);
	return eth_getenv_enetaddr(enetvar, enetaddr);
}

#ifdef CONFIG_NET_MULTI

static int eth_mac_skip(int index)
{
	char enetvar[15];
	char *skip_state;
	sprintf(enetvar, index ? "eth%dmacskip" : "ethmacskip", index);
	return ((skip_state = getenv(enetvar)) != NULL);
}

/*
 * CPU and board-specific Ethernet initializations.  Aliased function
 * signals caller to move on
 */
static int __def_eth_init(bd_t *bis)
{
	return -1;
}
int cpu_eth_init(bd_t *bis) __attribute__((weak, alias("__def_eth_init")));
int board_eth_init(bd_t *bis) __attribute__((weak, alias("__def_eth_init")));

extern int mv6436x_eth_initialize(bd_t *);
extern int mv6446x_eth_initialize(bd_t *);

#ifdef CONFIG_API
extern void (*push_packet)(volatile void *, int);

static struct {
	uchar data[PKTSIZE];
	int length;
} eth_rcv_bufs[PKTBUFSRX];

static unsigned int eth_rcv_current = 0, eth_rcv_last = 0;
#endif

static struct eth_device *eth_devices, *eth_current;

struct eth_device *eth_get_dev(void)
{
	return eth_current;
}

struct eth_device *eth_get_dev_by_name(char *devname)
{
	struct eth_device *dev, *target_dev;

	if (!eth_devices)
		return NULL;

	dev = eth_devices;
	target_dev = NULL;
	do {
		if (strcmp(devname, dev->name) == 0) {
			target_dev = dev;
			break;
		}
		dev = dev->next;
	} while (dev != eth_devices);

	return target_dev;
}

struct eth_device *eth_get_dev_by_index(int index)
{
	struct eth_device *dev, *target_dev;
	int idx = 0;

	if (!eth_devices)
		return NULL;

	dev = eth_devices;
	target_dev = NULL;
	do {
		if (idx == index) {
			target_dev = dev;
			break;
		}
		dev = dev->next;
		idx++;
	} while (dev != eth_devices);

	return target_dev;
}

int eth_get_dev_index (void)
{
	struct eth_device *dev;
	int num = 0;

	if (!eth_devices) {
		return (-1);
	}

	for (dev = eth_devices; dev; dev = dev->next) {
		if (dev == eth_current)
			break;
		++num;
	}

	if (dev) {
		return (num);
	}

	return (0);
}

int eth_register(struct eth_device* dev)
{
	struct eth_device *d;

	if (!eth_devices) {
		eth_current = eth_devices = dev;
#ifdef CONFIG_NET_MULTI
		/* update current ethernet name */
		{
			char *act = getenv("ethact");
			if (act == NULL || strcmp(act, eth_current->name) != 0)
				setenv("ethact", eth_current->name);
		}
#endif
	} else {
		for (d=eth_devices; d->next!=eth_devices; d=d->next)
			;
		d->next = dev;
	}

	dev->state = ETH_STATE_INIT;
	dev->next  = eth_devices;

	return 0;
}

int eth_initialize(bd_t *bis)
{
	unsigned char env_enetaddr[6];
	int eth_number = 0;

	eth_devices = NULL;
	eth_current = NULL;

	show_boot_progress (64);
#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
	miiphy_init();
#endif
	/* Try board-specific initialization first.  If it fails or isn't
	 * present, try the cpu-specific initialization */
	if (board_eth_init(bis) < 0)
		cpu_eth_init(bis);

#if defined(CONFIG_DB64360) || defined(CONFIG_CPCI750)
	mv6436x_eth_initialize(bis);
#endif
#if defined(CONFIG_DB64460) || defined(CONFIG_P3Mx)
	mv6446x_eth_initialize(bis);
#endif
	if (!eth_devices) {
		puts ("No ethernet found.\n");
		show_boot_progress (-64);
	} else {
		struct eth_device *dev = eth_devices;
		char *ethprime = getenv ("ethprime");

		show_boot_progress (65);
		do {
			if (eth_number)
				puts (", ");

			printf("%s", dev->name);

			if (ethprime && strcmp (dev->name, ethprime) == 0) {
				eth_current = dev;
				puts (" [PRIME]");
			}

			if (strchr(dev->name, ' '))
				puts("\nWarning: eth device name has a space!\n");

			eth_getenv_enetaddr_by_index(eth_number, env_enetaddr);

			if (memcmp(env_enetaddr, "\0\0\0\0\0\0", 6)) {
				if (memcmp(dev->enetaddr, "\0\0\0\0\0\0", 6) &&
				    memcmp(dev->enetaddr, env_enetaddr, 6))
				{
					printf ("\nWarning: %s MAC addresses don't match:\n",
						dev->name);
					printf ("Address in SROM is         %pM\n",
						dev->enetaddr);
					printf ("Address in environment is  %pM\n",
						env_enetaddr);
				}

				memcpy(dev->enetaddr, env_enetaddr, 6);
			}
			if (dev->write_hwaddr &&
				!eth_mac_skip(eth_number) &&
				is_valid_ether_addr(dev->enetaddr)) {
				dev->write_hwaddr(dev);
			}

			eth_number++;
			dev = dev->next;
		} while(dev != eth_devices);

#ifdef CONFIG_NET_MULTI
		/* update current ethernet name */
		if (eth_current) {
			char *act = getenv("ethact");
			if (act == NULL || strcmp(act, eth_current->name) != 0)
				setenv("ethact", eth_current->name);
		} else
			setenv("ethact", NULL);
#endif

		putc ('\n');
	}

	return eth_number;
}

#ifdef CONFIG_MCAST_TFTP
/* Multicast.
 * mcast_addr: multicast ipaddr from which multicast Mac is made
 * join: 1=join, 0=leave.
 */
int eth_mcast_join( IPaddr_t mcast_ip, u8 join)
{
 u8 mcast_mac[6];
	if (!eth_current || !eth_current->mcast)
		return -1;
	mcast_mac[5] = htonl(mcast_ip) & 0xff;
	mcast_mac[4] = (htonl(mcast_ip)>>8) & 0xff;
	mcast_mac[3] = (htonl(mcast_ip)>>16) & 0x7f;
	mcast_mac[2] = 0x5e;
	mcast_mac[1] = 0x0;
	mcast_mac[0] = 0x1;
	return eth_current->mcast(eth_current, mcast_mac, join);
}

/* the 'way' for ethernet-CRC-32. Spliced in from Linux lib/crc32.c
 * and this is the ethernet-crc method needed for TSEC -- and perhaps
 * some other adapter -- hash tables
 */
#define CRCPOLY_LE 0xedb88320
u32 ether_crc (size_t len, unsigned char const *p)
{
	int i;
	u32 crc;
	crc = ~0;
	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}
	/* an reverse the bits, cuz of way they arrive -- last-first */
	crc = (crc >> 16) | (crc << 16);
	crc = (crc >> 8 & 0x00ff00ff) | (crc << 8 & 0xff00ff00);
	crc = (crc >> 4 & 0x0f0f0f0f) | (crc << 4 & 0xf0f0f0f0);
	crc = (crc >> 2 & 0x33333333) | (crc << 2 & 0xcccccccc);
	crc = (crc >> 1 & 0x55555555) | (crc << 1 & 0xaaaaaaaa);
	return crc;
}

#endif


int eth_init(bd_t *bis)
{
	int eth_number;
	struct eth_device *old_current, *dev;

	if (!eth_current) {
		puts ("No ethernet found.\n");
		return -1;
	}

	/* Sync environment with network devices */
	eth_number = 0;
	dev = eth_devices;
	do {
		uchar env_enetaddr[6];

		if (eth_getenv_enetaddr_by_index(eth_number, env_enetaddr))
			memcpy(dev->enetaddr, env_enetaddr, 6);

		++eth_number;
		dev = dev->next;
	} while (dev != eth_devices);

	old_current = eth_current;
	do {
		debug("Trying %s\n", eth_current->name);

		if (eth_current->init(eth_current,bis) >= 0) {
			eth_current->state = ETH_STATE_ACTIVE;

			return 0;
		}
		debug("FAIL\n");

		eth_try_another(0);
	} while (old_current != eth_current);

	return -1;
}

void eth_halt(void)
{
	if (!eth_current)
		return;

	eth_current->halt(eth_current);

	eth_current->state = ETH_STATE_PASSIVE;
}

int eth_send(volatile void *packet, int length)
{
	if (!eth_current)
		return -1;

	return eth_current->send(eth_current, packet, length);
}

int eth_rx(void)
{
	if (!eth_current)
		return -1;

	return eth_current->recv(eth_current);
}

#ifdef CONFIG_API
static void eth_save_packet(volatile void *packet, int length)
{
	volatile char *p = packet;
	int i;

	if ((eth_rcv_last+1) % PKTBUFSRX == eth_rcv_current)
		return;

	if (PKTSIZE < length)
		return;

	for (i = 0; i < length; i++)
		eth_rcv_bufs[eth_rcv_last].data[i] = p[i];

	eth_rcv_bufs[eth_rcv_last].length = length;
	eth_rcv_last = (eth_rcv_last + 1) % PKTBUFSRX;
}

int eth_receive(volatile void *packet, int length)
{
	volatile char *p = packet;
	void *pp = push_packet;
	int i;

	if (eth_rcv_current == eth_rcv_last) {
		push_packet = eth_save_packet;
		eth_rx();
		push_packet = pp;

		if (eth_rcv_current == eth_rcv_last)
			return -1;
	}

	if (length < eth_rcv_bufs[eth_rcv_current].length)
		return -1;

	length = eth_rcv_bufs[eth_rcv_current].length;

	for (i = 0; i < length; i++)
		p[i] = eth_rcv_bufs[eth_rcv_current].data[i];

	eth_rcv_current = (eth_rcv_current + 1) % PKTBUFSRX;
	return length;
}
#endif /* CONFIG_API */

void eth_try_another(int first_restart)
{
	static struct eth_device *first_failed = NULL;
	char *ethrotate;

	/*
	 * Do not rotate between network interfaces when
	 * 'ethrotate' variable is set to 'no'.
	 */
	if (((ethrotate = getenv ("ethrotate")) != NULL) &&
	    (strcmp(ethrotate, "no") == 0))
		return;

	if (!eth_current)
		return;

	if (first_restart) {
		first_failed = eth_current;
	}

	eth_current = eth_current->next;

#ifdef CONFIG_NET_MULTI
	/* update current ethernet name */
	{
		char *act = getenv("ethact");
		if (act == NULL || strcmp(act, eth_current->name) != 0)
			setenv("ethact", eth_current->name);
	}
#endif

	if (first_failed == eth_current) {
		NetRestartWrap = 1;
	}
}

#ifdef CONFIG_NET_MULTI
void eth_set_current(void)
{
	static char *act = NULL;
	static int  env_changed_id = 0;
	struct eth_device* old_current;
	int	env_id;

	if (!eth_current)	/* XXX no current */
		return;

	env_id = get_env_id();
	if ((act == NULL) || (env_changed_id != env_id)) {
		act = getenv("ethact");
		env_changed_id = env_id;
	}
	if (act != NULL) {
		old_current = eth_current;
		do {
			if (strcmp(eth_current->name, act) == 0)
				return;
			eth_current = eth_current->next;
		} while (old_current != eth_current);
	}

	setenv("ethact", eth_current->name);
}
#endif

char *eth_get_name (void)
{
	return (eth_current ? eth_current->name : "unknown");
}

#else /* !CONFIG_NET_MULTI */

#warning Ethernet driver is deprecated.  Please update to use CONFIG_NET_MULTI

extern int at91rm9200_miiphy_initialize(bd_t *bis);
extern int mcf52x2_miiphy_initialize(bd_t *bis);
extern int ns7520_miiphy_initialize(bd_t *bis);


int eth_initialize(bd_t *bis)
{
#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII)
	miiphy_init();
#endif

#if defined(CONFIG_AT91RM9200)
	at91rm9200_miiphy_initialize(bis);
#endif
#if defined(CONFIG_MCF52x2)
	mcf52x2_miiphy_initialize(bis);
#endif
#if defined(CONFIG_DRIVER_NS7520_ETHERNET)
	ns7520_miiphy_initialize(bis);
#endif
	return 0;
}
#endif
