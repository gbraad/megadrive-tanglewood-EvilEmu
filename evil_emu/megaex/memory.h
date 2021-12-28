/*
 *  memory.h

Copyright (c) 2011 Lee Hammerton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 */

#ifndef _MEGAEX_MEMORY_H
#define _MEGAEX_MEMORY_H

#include "mytypes.h"
#include "stdio.h"
#include "config.h"

U8 Z80_MEM_getByte(U16 address);
void Z80_MEM_setByte(U16 address, U8 data);

U8 Z80_IO_getByte(U16 address);
void Z80_IO_setByte(U16 address, U8 data);

void MEM_SaveState(FILE *outStream);
void MEM_LoadState(FILE *inStream);

void MEM_Initialise(unsigned char *_romPtr, U32 romSize, unsigned int num64Banks);

void IO_GetRegisterContents(U16 offset, char *buffer);
void IO_UpdateControllerLatch(U32 cycles);

typedef U8(*MEM_ReadByteMap)(U32 address, U32 upper24, U32 lower16);
typedef void(*MEM_WriteByteMap)(U32 address, U32 upper24, U32 lower16, U8 byte);

typedef U16(*MEM_ReadWordMap)(U32 address);
typedef void(*MEM_WriteWordMap)(U32 address, U16 word);

typedef unsigned char*(*MEM_DMAMap)(U32 address);

class Memory
{
public:
	static MEM_ReadByteMap mem_read[256];
	static MEM_WriteByteMap mem_write[256];

	static MEM_ReadWordMap mem_read_word[256];
	static MEM_WriteWordMap mem_write_word[256];

	static MEM_DMAMap mem_dma_addr[256];

	static U8 YM2612_Regs[2][256];
	static U8 YM2612_AddressLatch1;
	static U8 YM2612_AddressLatch2;

	static unsigned char* vRam;
	static unsigned char* cRam;
	static unsigned char* vsRam;
	static unsigned char* cartRom;
	static unsigned char* cartRam;
	static unsigned char* smsBios;
	static unsigned char* systemRam;
	static unsigned char* z80Ram;

	static U32 bankAddress;
};

#include "memory.inl"

#endif

