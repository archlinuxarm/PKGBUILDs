/*
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/


#ifndef _SC_SynthImpl_
#define _SC_SynthImpl_

#include "SC_Synth.h"
#include "SC_SynthDef.h"
#include "HashTable.h"
#include "SC_AllocPool.h"
#include "SC_UnorderedList.h"

extern SynthInterfaceTable gSynthInterfaceTable;
void InitSynthInterfaceTable();

typedef void (*SetupInterfaceFunc)(SynthInterfaceTable*);

const int kMaxSynths = 1024;
extern StaticHashTable<Synth, kMaxSynths, const char*> gSynthTable;
extern AllocPool *gSynthAllocPool;

extern float32 gSine[8193];

#endif