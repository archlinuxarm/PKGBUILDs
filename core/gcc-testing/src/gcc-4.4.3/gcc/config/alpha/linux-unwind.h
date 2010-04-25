/* DWARF2 EH unwinding support for Alpha Linux.
   Copyright (C) 2004, 2005, 2009 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

/* Do code reading to identify a signal frame, and set the frame
   state data appropriately.  See unwind-dw2.c for the structs.  */

#include <signal.h>
#include <sys/ucontext.h>

#define MD_FALLBACK_FRAME_STATE_FOR alpha_fallback_frame_state

static _Unwind_Reason_Code
alpha_fallback_frame_state (struct _Unwind_Context *context,
			    _Unwind_FrameState *fs)
{
  unsigned int *pc = context->ra;
  struct sigcontext *sc;
  long new_cfa, i;

  if (pc[0] != 0x47fe0410		/* mov $30,$16 */
      || pc[2] != 0x00000083		/* callsys */)
    return _URC_END_OF_STACK;
  if (context->cfa == 0)
    return _URC_END_OF_STACK;
  if (pc[1] == 0x201f0067)		/* lda $0,NR_sigreturn */
    sc = context->cfa;
  else if (pc[1] == 0x201f015f)	/* lda $0,NR_rt_sigreturn */
    {
      struct rt_sigframe {
	struct siginfo info;
	struct ucontext uc;
      } *rt_ = context->cfa;
      sc = &rt_->uc.uc_mcontext;
    }
  else
    return _URC_END_OF_STACK;
  new_cfa = sc->sc_regs[30];
  fs->regs.cfa_how = CFA_REG_OFFSET;
  fs->regs.cfa_reg = 30;
  fs->regs.cfa_offset = new_cfa - (long) context->cfa;
  for (i = 0; i < 30; ++i)
    {
      fs->regs.reg[i].how = REG_SAVED_OFFSET;
      fs->regs.reg[i].loc.offset
	= (long)&sc->sc_regs[i] - new_cfa;
    }
  for (i = 0; i < 31; ++i)
    {
      fs->regs.reg[i+32].how = REG_SAVED_OFFSET;
      fs->regs.reg[i+32].loc.offset
	= (long)&sc->sc_fpregs[i] - new_cfa;
    }
  fs->regs.reg[64].how = REG_SAVED_OFFSET;
  fs->regs.reg[64].loc.offset = (long)&sc->sc_pc - new_cfa;
  fs->retaddr_column = 64;
  return _URC_NO_REASON;
}
