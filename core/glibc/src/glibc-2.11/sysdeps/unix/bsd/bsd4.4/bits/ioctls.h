/*-
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ioctl.h	7.19 (Berkeley) 6/26/91
 */

#ifndef	_IOCTLS_H_
#define	_IOCTLS_H_

#define	TIOCGSIZE	TIOCGWINSZ
#define	TIOCSSIZE	TIOCSWINSZ

/*
 * Ioctl's have the command encoded in the lower word, and the size of
 * any in or out parameters in the upper word.  The high 3 bits of the
 * upper word are used to encode the in/out status of the parameter.
 */
#define	IOCPARM_MASK	0x1fff		/* parameter length, at most 13 bits */
#define	IOCPARM_LEN(x)	(((x) >> 16) & IOCPARM_MASK)
#define	IOCBASECMD(x)	((x) & ~IOCPARM_MASK)
#define	IOCGROUP(x)	(((x) >> 8) & 0xff)

#define	IOCPARM_MAX	NBPG		/* max size of ioctl, mult. of NBPG */
#define	IOC_VOID	0x20000000	/* no parameters */
#define	IOC_OUT		0x40000000	/* copy out parameters */
#define	IOC_IN		0x80000000	/* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)
#define	IOC_DIRMASK	0xe0000000	/* mask for IN/OUT/VOID */

#define _IOC(inout,group,num,len) \
	(inout | ((len & IOCPARM_MASK) << 16) | ((group) << 8) | (num))
#define	_IO(g,n)	_IOC(IOC_VOID,	(g), (n), 0)
#define	_IOR(g,n,t)	_IOC(IOC_OUT,	(g), (n), sizeof(t))
#define	_IOW(g,n,t)	_IOC(IOC_IN,	(g), (n), sizeof(t))
/* this should be _IORW, but stdio got there first */
#define	_IOWR(g,n,t)	_IOC(IOC_INOUT,	(g), (n), sizeof(t))

#define	TIOCMODG	_IOR('t', 3, int)	/* get modem control state */
#define	TIOCMODS	_IOW('t', 4, int)	/* set modem control state */
#define		TIOCM_LE	0001		/* line enable */
#define		TIOCM_DTR	0002		/* data terminal ready */
#define		TIOCM_RTS	0004		/* request to send */
#define		TIOCM_ST	0010		/* secondary transmit */
#define		TIOCM_SR	0020		/* secondary receive */
#define		TIOCM_CTS	0040		/* clear to send */
#define		TIOCM_CAR	0100		/* carrier detect */
#define		TIOCM_CD	TIOCM_CAR
#define		TIOCM_RNG	0200		/* ring */
#define		TIOCM_RI	TIOCM_RNG
#define		TIOCM_DSR	0400		/* data set ready */
						/* 8-10 compat */
#define	TIOCEXCL	_IO('t', 13)		/* set exclusive use of tty */
#define	TIOCNXCL	_IO('t', 14)		/* reset exclusive use of tty */
						/* 15 unused */
#define	TIOCFLUSH	_IOW('t', 16, int)	/* flush buffers */
						/* 17-18 compat */
#define	TIOCGETA	_IOR('t', 19, struct termios) /* get termios struct */
#define	TIOCSETA	_IOW('t', 20, struct termios) /* set termios struct */
#define	TIOCSETAW	_IOW('t', 21, struct termios) /* drain output, set */
#define	TIOCSETAF	_IOW('t', 22, struct termios) /* drn out, fls in, set */
#define	TIOCGETD	_IOR('t', 26, int)	/* get line discipline */
#define	TIOCSETD	_IOW('t', 27, int)	/* set line discipline */
						/* 127-124 compat */
#define	TIOCSBRK	_IO('t', 123)		/* set break bit */
#define	TIOCCBRK	_IO('t', 122)		/* clear break bit */
#define	TIOCSDTR	_IO('t', 121)		/* set data terminal ready */
#define	TIOCCDTR	_IO('t', 120)		/* clear data terminal ready */
#define	TIOCGPGRP	_IOR('t', 119, int)	/* get pgrp of tty */
#define	TIOCSPGRP	_IOW('t', 118, int)	/* set pgrp of tty */
						/* 117-116 compat */
#define	TIOCOUTQ	_IOR('t', 115, int)	/* output queue size */
#define	TIOCSTI		_IOW('t', 114, char)	/* simulate terminal input */
#define	TIOCNOTTY	_IO('t', 113)		/* void tty association */
#define	TIOCPKT		_IOW('t', 112, int)	/* pty: set/clear packet mode */
#define		TIOCPKT_DATA		0x00	/* data packet */
#define		TIOCPKT_FLUSHREAD	0x01	/* flush packet */
#define		TIOCPKT_FLUSHWRITE	0x02	/* flush packet */
#define		TIOCPKT_STOP		0x04	/* stop output */
#define		TIOCPKT_START		0x08	/* start output */
#define		TIOCPKT_NOSTOP		0x10	/* no more ^S, ^Q */
#define		TIOCPKT_DOSTOP		0x20	/* now do ^S ^Q */
#define		TIOCPKT_IOCTL		0x40	/* state change of pty driver */
#define	TIOCSTOP	_IO('t', 111)		/* stop output, like ^S */
#define	TIOCSTART	_IO('t', 110)		/* start output, like ^Q */
#define	TIOCMSET	_IOW('t', 109, int)	/* set all modem bits */
#define	TIOCMBIS	_IOW('t', 108, int)	/* bis modem bits */
#define	TIOCMBIC	_IOW('t', 107, int)	/* bic modem bits */
#define	TIOCMGET	_IOR('t', 106, int)	/* get all modem bits */
#define	TIOCREMOTE	_IOW('t', 105, int)	/* remote input editing */
#define	TIOCGWINSZ	_IOR('t', 104, struct winsize)	/* get window size */
#define	TIOCSWINSZ	_IOW('t', 103, struct winsize)	/* set window size */
#define	TIOCUCNTL	_IOW('t', 102, int)	/* pty: set/clr usr cntl mode */
#define		UIOCCMD(n)	_IO('u', n)		/* usr cntl op "n" */
#define	TIOCCONS	_IOW('t', 98, int)		/* become virtual console */
#define	TIOCSCTTY	_IO('t', 97)		/* become controlling tty */
#define	TIOCEXT		_IOW('t', 96, int)	/* pty: external processing */
#define	TIOCSIG		_IO('t', 95)		/* pty: generate signal */
#define TIOCDRAIN	_IO('t', 94)		/* wait till output drained */

#define TTYDISC		0		/* termios tty line discipline */
#define	TABLDISC	3		/* tablet discipline */
#define	SLIPDISC	4		/* serial IP discipline */


#define	FIOCLEX		_IO('f', 1)		/* set close on exec on fd */
#define	FIONCLEX	_IO('f', 2)		/* remove close on exec */
#define	FIONREAD	_IOR('f', 127, int)	/* get # bytes to read */
#define	FIONBIO		_IOW('f', 126, int)	/* set/clear non-blocking i/o */
#define	FIOASYNC	_IOW('f', 125, int)	/* set/clear async i/o */
#define	FIOSETOWN	_IOW('f', 124, int)	/* set owner */
#define	FIOGETOWN	_IOR('f', 123, int)	/* get owner */

/* socket i/o controls */
#define	SIOCSHIWAT	_IOW('s',  0, int)		/* set high watermark */
#define	SIOCGHIWAT	_IOR('s',  1, int)		/* get high watermark */
#define	SIOCSLOWAT	_IOW('s',  2, int)		/* set low watermark */
#define	SIOCGLOWAT	_IOR('s',  3, int)		/* get low watermark */
#define	SIOCATMARK	_IOR('s',  7, int)		/* at oob mark? */
#define	SIOCSPGRP	_IOW('s',  8, int)		/* set process group */
#define	SIOCGPGRP	_IOR('s',  9, int)		/* get process group */

#define	SIOCADDRT	_IOW('r', 10, struct ortentry)	/* add route */
#define	SIOCDELRT	_IOW('r', 11, struct ortentry)	/* delete route */

#define	SIOCSIFADDR	_IOW('i', 12, struct ifreq)	/* set ifnet address */
#define	OSIOCGIFADDR	_IOWR('i',13, struct ifreq)	/* get ifnet address */
#define	SIOCGIFADDR	_IOWR('i',33, struct ifreq)	/* get ifnet address */
#define	SIOCSIFDSTADDR	_IOW('i', 14, struct ifreq)	/* set p-p address */
#define	OSIOCGIFDSTADDR	_IOWR('i',15, struct ifreq)	/* get p-p address */
#define	SIOCGIFDSTADDR	_IOWR('i',34, struct ifreq)	/* get p-p address */
#define	SIOCSIFFLAGS	_IOW('i', 16, struct ifreq)	/* set ifnet flags */
#define	SIOCGIFFLAGS	_IOWR('i',17, struct ifreq)	/* get ifnet flags */
#define	OSIOCGIFBRDADDR	_IOWR('i',18, struct ifreq)	/* get broadcast addr */
#define	SIOCGIFBRDADDR	_IOWR('i',35, struct ifreq)	/* get broadcast addr */
#define	SIOCSIFBRDADDR	_IOW('i',19, struct ifreq)	/* set broadcast addr */
#define	OSIOCGIFCONF	_IOWR('i',20, struct ifconf)	/* get ifnet list */
#define	SIOCGIFCONF	_IOWR('i',36, struct ifconf)	/* get ifnet list */
#define	OSIOCGIFNETMASK	_IOWR('i',21, struct ifreq)	/* get net addr mask */
#define	SIOCGIFNETMASK	_IOWR('i',37, struct ifreq)	/* get net addr mask */
#define	SIOCSIFNETMASK	_IOW('i',22, struct ifreq)	/* set net addr mask */
#define	SIOCGIFMETRIC	_IOWR('i',23, struct ifreq)	/* get IF metric */
#define	SIOCSIFMETRIC	_IOW('i',24, struct ifreq)	/* set IF metric */
#define	SIOCDIFADDR	_IOW('i',25, struct ifreq)	/* delete IF addr */
#define	SIOCAIFADDR	_IOW('i',26, struct ifaliasreq)	/* add/chg IF alias */

#define	SIOCSARP	_IOW('i', 30, struct arpreq)	/* set arp entry */
#define	OSIOCGARP	_IOWR('i',31, struct arpreq)	/* get arp entry */
#define	SIOCGARP	_IOWR('i',38, struct arpreq)	/* get arp entry */
#define	SIOCDARP	_IOW('i', 32, struct arpreq)	/* delete arp entry */


/* Compatibility with 4.3 BSD terminal driver.
   From 4.4 <sys/ioctl_compat.h>.  */

#ifdef USE_OLD_TTY
# undef  TIOCGETD
# define TIOCGETD	_IOR('t', 0, int)	/* get line discipline */
# undef  TIOCSETD
# define TIOCSETD	_IOW('t', 1, int)	/* set line discipline */
#else
# define OTIOCGETD	_IOR('t', 0, int)	/* get line discipline */
# define OTIOCSETD	_IOW('t', 1, int)	/* set line discipline */
#endif
#define	TIOCHPCL	_IO('t', 2)		/* hang up on last close */
#define	TIOCGETP	_IOR('t', 8,struct sgttyb)/* get parameters -- gtty */
#define	TIOCSETP	_IOW('t', 9,struct sgttyb)/* set parameters -- stty */
#define	TIOCSETN	_IOW('t',10,struct sgttyb)/* as above, but no flushtty*/
#define	TIOCSETC	_IOW('t',17,struct tchars)/* set special characters */
#define	TIOCGETC	_IOR('t',18,struct tchars)/* get special characters */
#define		TANDEM		0x00000001	/* send stopc on out q full */
#define		CBREAK		0x00000002	/* half-cooked mode */
#define		LCASE		0x00000004	/* simulate lower case */
#define		ECHO		0x00000008	/* echo input */
#define		CRMOD		0x00000010	/* map \r to \r\n on output */
#define		RAW		0x00000020	/* no i/o processing */
#define		ODDP		0x00000040	/* get/send odd parity */
#define		EVENP		0x00000080	/* get/send even parity */
#define		ANYP		0x000000c0	/* get any parity/send none */
#define		NLDELAY		0x00000300	/* \n delay */
#define			NL0	0x00000000
#define			NL1	0x00000100	/* tty 37 */
#define			NL2	0x00000200	/* vt05 */
#define			NL3	0x00000300
#define		TBDELAY		0x00000c00	/* horizontal tab delay */
#define			TAB0	0x00000000
#define			TAB1	0x00000400	/* tty 37 */
#define			TAB2	0x00000800
#define		XTABS		0x00000c00	/* expand tabs on output */
#define		CRDELAY		0x00003000	/* \r delay */
#define			CR0	0x00000000
#define			CR1	0x00001000	/* tn 300 */
#define			CR2	0x00002000	/* tty 37 */
#define			CR3	0x00003000	/* concept 100 */
#define		VTDELAY		0x00004000	/* vertical tab delay */
#define			FF0	0x00000000
#define			FF1	0x00004000	/* tty 37 */
#define		BSDELAY		0x00008000	/* \b delay */
#define			BS0	0x00000000
#define			BS1	0x00008000
#define		ALLDELAY	(NLDELAY|TBDELAY|CRDELAY|VTDELAY|BSDELAY)
#define		CRTBS		0x00010000	/* do backspacing for crt */
#define		PRTERA		0x00020000	/* \ ... / erase */
#define		CRTERA		0x00040000	/* " \b " to wipe out char */
#define		TILDE		0x00080000	/* hazeltine tilde kludge */
#define		MDMBUF		0x00100000	/*start/stop output on carrier*/
#define		LITOUT		0x00200000	/* literal output */
#define		TOSTOP		0x00400000	/*SIGSTOP on background output*/
#define		FLUSHO		0x00800000	/* flush output to terminal */
#define		NOHANG		0x01000000	/* (no-op) was no SIGHUP on carrier drop */
#define		L001000		0x02000000
#define		CRTKIL		0x04000000	/* kill line with " \b " */
#define		PASS8		0x08000000
#define		CTLECH		0x10000000	/* echo control chars as ^X */
#define		PENDIN		0x20000000	/* tp->t_rawq needs reread */
#define		DECCTQ		0x40000000	/* only ^Q starts after ^S */
#define		NOFLSH		0x80000000	/* no output flush on signal */
#define	TIOCLBIS	_IOW('t', 127, int)	/* bis local mode bits */
#define	TIOCLBIC	_IOW('t', 126, int)	/* bic local mode bits */
#define	TIOCLSET	_IOW('t', 125, int)	/* set entire local mode word */
#define	TIOCLGET	_IOR('t', 124, int)	/* get local modes */
#define		LCRTBS		(CRTBS>>16)
#define		LPRTERA		(PRTERA>>16)
#define		LCRTERA		(CRTERA>>16)
#define		LTILDE		(TILDE>>16)
#define		LMDMBUF		(MDMBUF>>16)
#define		LLITOUT		(LITOUT>>16)
#define		LTOSTOP		(TOSTOP>>16)
#define		LFLUSHO		(FLUSHO>>16)
#define		LNOHANG		(NOHANG>>16)
#define		LCRTKIL		(CRTKIL>>16)
#define		LPASS8		(PASS8>>16)
#define		LCTLECH		(CTLECH>>16)
#define		LPENDIN		(PENDIN>>16)
#define		LDECCTQ		(DECCTQ>>16)
#define		LNOFLSH		(NOFLSH>>16)
#define	TIOCSLTC	_IOW('t',117,struct ltchars)/* set local special chars*/
#define	TIOCGLTC	_IOR('t',116,struct ltchars)/* get local special chars*/
#define OTIOCCONS	_IO('t', 98)	/* for hp300 -- sans int arg */
#define	OTTYDISC	0
#define	NETLDISC	1
#define	NTTYDISC	2

/* From 4.4 <sys/ttydev.h>.   */
#ifdef USE_OLD_TTY
#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define	B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define EXTA	14
#define EXTB	15
#endif /* USE_OLD_TTY */

#endif /* !_IOCTLS_H_ */
