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

#pragma once

#include "mytypes.h"

ION_C_API U8 Z80_MEM_getByte(U16 address);
ION_C_API void Z80_MEM_setByte(U16 address, U8 data);

ION_C_API U8 Z80_IO_getByte(U16 address);
ION_C_API void Z80_IO_setByte(U16 address, U8 data);

ION_C_API void MEM_SaveState(FILE *outStream);
ION_C_API void MEM_LoadState(FILE *inStream);

ION_C_API void MEM_Initialise(unsigned char *_romPtr, U32 romSize, unsigned int num64Banks);

ION_C_API void MEM_MapKickstartLow(int yes);

ION_C_API inline U8		MEM_getByte(U32 address);
ION_C_API inline void	MEM_setByte(U32 address,U8 byte);
ION_C_API inline U16	MEM_getWord(U32 address);
ION_C_API inline void	MEM_setWord(U32 address,U16 word);
ION_C_API inline U32	MEM_getLong(U32 address);
ION_C_API inline void	MEM_setLong(U32 address,U32 dword);

typedef U8(*MEM_ReadByteMap)(U32 address, U32 upper24, U32 lower16);
typedef void(*MEM_WriteByteMap)(U32 address, U32 upper24, U32 lower16, U8 byte);

typedef U16(*MEM_ReadWordMap)(U32 address);
typedef void(*MEM_WriteWordMap)(U32 address, U16 word);

ION_C_API MEM_ReadByteMap	mem_read[256];
ION_C_API MEM_WriteByteMap	mem_write[256];

ION_C_API MEM_ReadWordMap	mem_read_word[256];
ION_C_API MEM_WriteWordMap	mem_write_word[256];

#include "memory.inl"
