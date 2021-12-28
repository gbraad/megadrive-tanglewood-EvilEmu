/*
 *  debugger.c

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "megaex/config.h"
#include "megaex/platform.h"
#include "megaex/callbacks.h"
#include "megaex/globals.h"
#include "megaex/emulator.h"
#include "megaex/cpu/m68k/cpu.h"
#include "megaex/cpu/m68k/cpu_dis.h"
#include "megaex/cpu/z80/z80.h"
#include "megaex/cpu/z80/z80_dis.h"
#include "megaex/vdp/vdp.h"
#include "megaex/mytypes.h"
#include "megaex/memory.h"
#include "megaex/gui/debugger.h"
#include "megaex/gui/font.h"

DebugState stageCheck = DebugState::Idle;
DebugMode dbMode = DebugMode::Off;

DebugMode DEB_GetDebugMode()
{
	return dbMode;
}

DebugState DEB_GetDebugState()
{
	return stageCheck;
}

#if EMU_ENABLE_68K_DEBUGGER

#define MAX_BPS	20
U32 bpAddresses[MAX_BPS]={0x30c};
U32 Z80_bpAddresses[MAX_BPS]={0x66};
int numBps=0;
int Z80_numBps=0;

int copNum=0;

int debugDrawX = 128;
int debugDrawY = 128;

extern U8 CRAM[0x200];
extern CPU_Ins* CPU_Information[65536];
extern CPU_Decode CPU_DisTable[65536];
extern Z80_Decode Z80_DisTable[256];
extern Z80_Ins* Z80_Information[256];

static ion::input::Keycode debuggerKeyBreak = ion::input::Keycode::TAB;
static ion::input::Keycode debuggerKeyContinue = ion::input::Keycode::F5;
static ion::input::Keycode debuggerKeyStepInstruction = ion::input::Keycode::F10;
static ion::input::Keycode debuggerKeyStepFrame = ion::input::Keycode::F8;
static ion::input::Keycode debuggerKeyToggleBreakpoint = ion::input::Keycode::F9;
static ion::input::Keycode debuggerKeySaveState = ion::input::Keycode::F11;
static ion::input::Keycode debuggerKeyLoadState = ion::input::Keycode::F12;
static ion::input::Keycode debuggerKeyScrollUp = ion::input::Keycode::UP;
static ion::input::Keycode debuggerKeyScrollDown = ion::input::Keycode::DOWN;
static ion::input::Keycode debuggerKey68KMode = ion::input::Keycode::F1;
static ion::input::Keycode debuggerKeyZ80Mode = ion::input::Keycode::F2;
static ion::input::Keycode debuggerKeySpriteMode = ion::input::Keycode::F3;
static ion::input::Keycode debuggerKey68KHistoryMode = ion::input::Keycode::F4;

void DEB_PauseEmulation(DebugMode pauseMode, const char *reason)
{
	dbMode=pauseMode;
	Globals::g_pause=1;
	EMU_PRINTF("Invoking debugger due to %s\n", reason);
}

void DrawChar(U32 x,U32 y, char c,int cMask1,int cMask2)
{
	int a,b;
	unsigned char *fontChar=&FontData[c*6*8];
	
	x*=8;
	y*=8;
	
	for (a=0;a<8;a++)
	{
		for (b=0;b<6;b++)
		{
			doPixelNoScale(x+b+1,y+a,(*fontChar) * cMask1,(*fontChar) * cMask2);
			fontChar++;
		}
	}
}

void PrintAt(int cMask1,int cMask2,U32 x,U32 y,const char *msg,...)
{
	static char tStringBuffer[32768];
	char *pMsg=tStringBuffer;
	va_list args;

	va_start (args, msg);
	vsprintf (tStringBuffer,msg, args);
	va_end (args);

	while (*pMsg)
	{
		DrawChar(x,y,*pMsg,cMask1,cMask2);
		x++;
		pMsg++;
	}
}

void DisplayWindow32(U32 x,U32 y, U32 w, U32 h, U32 colour)
{
	U32 xx,yy;
	x*=8;
	y*=8;
	w*=8;
	h*=8;
	w+=x;
	h+=y;
	for (yy=y;yy<h;yy++)
	{
		for (xx=x;xx<w;xx++)
		{
			doPixel32NoScale(xx,yy,colour);
		}
	}
}

void DisplayWindow(U32 x,U32 y, U32 w, U32 h, U8 AR, U8 GB)
{
	U32 xx,yy;
	x*=8;
	y*=8;
	w*=8;
	h*=8;
	w+=x;
	h+=y;
	for (yy=y;yy<h;yy++)
	{
		for (xx=x;xx<w;xx++)
		{
			doPixelNoScale(xx,yy,AR,GB);
		}
	}
}

void ShowZ80State(int offs)
{
	UNUSED_ARGUMENT(offs);
	DisplayWindow(0,0,66,11,0,0);
	PrintAt(debugDrawX,debugDrawY,1,1," A=%02X  B=%02X  C=%02x  D=%02x",Z80::Z80_regs.R[Z80_REG_A],Z80::Z80_regs.R[Z80_REG_B],Z80::Z80_regs.R[Z80_REG_C],Z80::Z80_regs.R[Z80_REG_D]);
	PrintAt(debugDrawX,debugDrawY,1,2," E=%02X  F=%02X  H=%02x  L=%02x",Z80::Z80_regs.R[Z80_REG_E],Z80::Z80_regs.R[Z80_REG_F],Z80::Z80_regs.R[Z80_REG_H],Z80::Z80_regs.R[Z80_REG_L]);
	PrintAt(debugDrawX,debugDrawY,1,3,"A'=%02X B'=%02X C'=%02x D'=%02x",Z80::Z80_regs.R_[Z80_REG_A],Z80::Z80_regs.R_[Z80_REG_B],Z80::Z80_regs.R_[Z80_REG_C],Z80::Z80_regs.R_[Z80_REG_D]);
	PrintAt(debugDrawX,debugDrawY,1,4,"E'=%02X F'=%02X H'=%02x L'=%02x",Z80::Z80_regs.R_[Z80_REG_E],Z80::Z80_regs.R_[Z80_REG_F],Z80::Z80_regs.R_[Z80_REG_H],Z80::Z80_regs.R_[Z80_REG_L]);
  PrintAt(debugDrawX,debugDrawY,1,5,"AF=%02X%02X BC=%02X%02X DE=%02X%02X HL=%02X%02X IR=%04X",Z80::Z80_regs.R[Z80_REG_A],Z80::Z80_regs.R[Z80_REG_F],
		Z80::Z80_regs.R[Z80_REG_B],Z80::Z80_regs.R[Z80_REG_C],Z80::Z80_regs.R[Z80_REG_D],Z80::Z80_regs.R[Z80_REG_E],Z80::Z80_regs.R[Z80_REG_H],Z80::Z80_regs.R[Z80_REG_L],Z80::Z80_regs.IR);
	PrintAt(debugDrawX,debugDrawY,1,6,"IX=%04X IY=%04X SP=%04X IFF1=%01X IFF2=%01X\n",Z80::Z80_regs.IX,Z80::Z80_regs.IY,Z80::Z80_regs.SP,Z80::Z80_regs.IFF1,Z80::Z80_regs.IFF2);
	PrintAt(debugDrawX,debugDrawY,1,8,"[  S: Z:B5: H:B3:PV: N: C ]  Interrupt Mode = %d",Z80::Z80_regs.InterruptMode);
	PrintAt(debugDrawX,debugDrawY,1,9,"[ %s:%s:%s:%s:%s:%s:%s:%s ]  Bank Address = %08X",				/* Bank address is not part of z80, but convenient to show here for now */
		   Z80::Z80_regs.R[Z80_REG_F] & 0x80 ? " 1" : " 0",
		   Z80::Z80_regs.R[Z80_REG_F] & 0x40 ? " 1" : " 0",
		   Z80::Z80_regs.R[Z80_REG_F] & 0x20 ? " 1" : " 0",
		   Z80::Z80_regs.R[Z80_REG_F] & 0x10 ? " 1" : " 0",
		   Z80::Z80_regs.R[Z80_REG_F] & 0x08 ? " 1" : " 0",
		   Z80::Z80_regs.R[Z80_REG_F] & 0x04 ? " 1" : " 0",
		   Z80::Z80_regs.R[Z80_REG_F] & 0x02 ? " 1" : " 0",
		   Z80::Z80_regs.R[Z80_REG_F] & 0x01 ? " 1" : " 0", Memory::bankAddress);
}


void ShowCPUState(int offs)
{
	DisplayWindow(0,0+offs,66,10,0,0);
    PrintAt(debugDrawX,debugDrawY,1,1+offs," D0=%08X  D1=%08X  D2=%08x  D3=%08x",M68K::cpu_regs.D[0],M68K::cpu_regs.D[1],M68K::cpu_regs.D[2],M68K::cpu_regs.D[3]);
    PrintAt(debugDrawX,debugDrawY,1,2+offs," D4=%08X  D5=%08X  D6=%08x  D7=%08x",M68K::cpu_regs.D[4],M68K::cpu_regs.D[5],M68K::cpu_regs.D[6],M68K::cpu_regs.D[7]);
    PrintAt(debugDrawX,debugDrawY,1,3+offs," A0=%08X  A1=%08X  A2=%08x  A3=%08x",M68K::cpu_regs.A[0],M68K::cpu_regs.A[1],M68K::cpu_regs.A[2],M68K::cpu_regs.A[3]);
    PrintAt(debugDrawX,debugDrawY,1,4+offs," A4=%08X  A5=%08X  A6=%08x  A7=%08x",M68K::cpu_regs.A[4],M68K::cpu_regs.A[5],M68K::cpu_regs.A[6],M68K::cpu_regs.A[7]);
    PrintAt(debugDrawX,debugDrawY,1,5+offs,"USP=%08X ISP=%08x\n",M68K::cpu_regs.USP,M68K::cpu_regs.ISP);
    PrintAt(debugDrawX,debugDrawY,1,7+offs,"          [ T1:T0: S: M:  :I2:I1:I0:  :  :  : X: N: Z: V: C ]");
    PrintAt(debugDrawX,debugDrawY,1,8+offs," SR=%04X  [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]", M68K::cpu_regs.SR, 
		   M68K::cpu_regs.SR & 0x8000 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x4000 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x2000 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x1000 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0800 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0400 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0200 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0100 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0080 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0040 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0020 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0010 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0008 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0004 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0002 ? " 1" : " 0",
		   M68K::cpu_regs.SR & 0x0001 ? " 1" : " 0");
}

#if ENABLE_32X_MODE

#include "sh2.h"
#include "sh2_memory.h"

extern SH2_State* master;
extern SH2_State* slave;

void ShowSH2State(SH2_State* cpu)
{
	DisplayWindow(0,1+0,66,10,0,0);
	PrintAt(debugDrawX,debugDrawY,1,1+1,"R00 =%08X\tR01 =%08X\tR02 =%08X\tR03 =%08X\n",cpu->R[0],cpu->R[1],cpu->R[2],cpu->R[3]);
	PrintAt(debugDrawX,debugDrawY,1,1+2,"R04 =%08X\tR05 =%08X\tR06 =%08X\tR07 =%08X\n",cpu->R[4],cpu->R[5],cpu->R[6],cpu->R[7]);
	PrintAt(debugDrawX,debugDrawY,1,1+3,"R08 =%08X\tR09 =%08X\tR10 =%08X\tR11 =%08X\n",cpu->R[8],cpu->R[9],cpu->R[10],cpu->R[11]);
	PrintAt(debugDrawX,debugDrawY,1,1+4,"R12 =%08X\tR13 =%08X\tR14 =%08X\tR15 =%08X\n",cpu->R[12],cpu->R[13],cpu->R[14],cpu->R[15]);
	PrintAt(debugDrawX,debugDrawY,1,1+5,"\n");
	PrintAt(debugDrawX,debugDrawY,1,1+6,"MACH=%08X\tMACL=%08X\tPR  =%08X\tPC  =%08X\n",cpu->MACH,cpu->MACL,cpu->PR,cpu->PC);
	PrintAt(debugDrawX,debugDrawY,1,1+7,"GBR =%08X\tVBR =%08X\n",cpu->GBR,cpu->VBR);
	PrintAt(debugDrawX,debugDrawY,1,1+9,"          [   :  :  :  :  :  : M: Q:I3:I2:I1:I0:  :  : S: T ]\n");
	PrintAt(debugDrawX,debugDrawY,1,1+10,"SR = %04X [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]\n", cpu->SR&0xFFFF, 
		cpu->SR & 0x8000 ? " 1" : " 0",
		cpu->SR & 0x4000 ? " 1" : " 0",
		cpu->SR & 0x2000 ? " 1" : " 0",
		cpu->SR & 0x1000 ? " 1" : " 0",
		cpu->SR & 0x0800 ? " 1" : " 0",
		cpu->SR & 0x0400 ? " 1" : " 0",
		cpu->SR & 0x0200 ? " 1" : " 0",
		cpu->SR & 0x0100 ? " 1" : " 0",
		cpu->SR & 0x0080 ? " 1" : " 0",
		cpu->SR & 0x0040 ? " 1" : " 0",
		cpu->SR & 0x0020 ? " 1" : " 0",
		cpu->SR & 0x0010 ? " 1" : " 0",
		cpu->SR & 0x0008 ? " 1" : " 0",
		cpu->SR & 0x0004 ? " 1" : " 0",
		cpu->SR & 0x0002 ? " 1" : " 0",
		cpu->SR & 0x0001 ? " 1" : " 0");
	PrintAt(debugDrawX,debugDrawY,1,1+12,"PipeLine : %d\t%d\t%d\t%d\t%d\n",cpu->pipeLine[0].stage,cpu->pipeLine[1].stage,cpu->pipeLine[2].stage,cpu->pipeLine[3].stage,cpu->pipeLine[4].stage);
	PrintAt(debugDrawX,debugDrawY,1,1+13,"PipeLine : %04X\t%04X\t%04X\t%04X\t%04X\n",cpu->pipeLine[0].opcode,cpu->pipeLine[1].opcode,cpu->pipeLine[2].opcode,cpu->pipeLine[3].opcode,cpu->pipeLine[4].opcode);
}

extern SH2_Ins		*SH2_Information[65536];
extern SH2_Decode	SH2_DisTable[65536];

U32 SH2_GetOpcodeLength(SH2_State* cpu,U32 address)
{
	U16	opcode;
	U16	operands[8];
	U32	insCount;
	S32	a;

	opcode = SH2_Debug_Read_Word(cpu,address);

	if (SH2_Information[opcode])
	{
		for (a=0;a<SH2_Information[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & SH2_Information[opcode]->operandMask[a]) >> 
									SH2_Information[opcode]->operandShift[a];
		}
	}
			
	insCount=SH2_DisTable[opcode](cpu,address+1,operands[0],operands[1],operands[2],
						operands[3],operands[4],operands[5],operands[6],operands[7]);

	return insCount;
}

extern char SH2_mnemonicData[256];

U32 SH2_DissasembleAddress(U32 x,U32 y,U32 address,SH2_State* cpu,int cursor)
{
	U32	insCount;
/*	S32	a;*/
	U32 b;
	int cMask1=0x0F,cMask2=0xFF;
	
	insCount=SH2_GetOpcodeLength(cpu,address);		/* note this also does the disassemble */
/*
	for (a=0;a<Z80_numBps;a++)
	{
		if (address == Z80_bpAddresses[a])
		{
			cMask1=0x0F;
			cMask2=0x00;
			break;
		}
	}
	*/
	if (cursor)
	{
		PrintAt(cMask1,cMask2,x,y,"%08X >",address);
	}
	else
	{
		PrintAt(cMask1,cMask2,x,y,"%08X ",address);
	}
	
	for (b=0;b<insCount+1;b++)
	{
		PrintAt(cMask1,cMask2,x+10+b*5,y,"%04X ",SH2_Debug_Read_Word(cpu,address+b*2));
	}
			
	PrintAt(cMask1,cMask2,x+30,y,"%s",SH2_mnemonicData);
	
	return insCount+1;
}

void DisplaySH2Dis(SH2_State* cpu)
{
	int a;
	U32 address = cpu->pipeLine[0].PC;

	if (cpu->pipeLine[0].stage==0)
	{
		address=cpu->PC;
	}

	DisplayWindow(0,19+0,70,12,0,0);
	for (a=0;a<10;a++)
	{
		address+=2*SH2_DissasembleAddress(1,20+a+0,address,cpu,0);
	}
}


#endif

U32 GetOpcodeLength(U32 address)
{
	U16	opcode;
	U16	operands[8];
	U32	insCount;
	S32	a;

	opcode = MEM_getWord(address);

	if (CPU_Information[opcode])
	{
		for (a=0;a<CPU_Information[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & CPU_Information[opcode]->operandMask[a]) >> 
									CPU_Information[opcode]->operandShift[a];
		}
	}
			
	insCount=CPU_DisTable[opcode](address+2,operands);

	return insCount;
}

U8 Z80_MEM_getByte(U16 address);

U32 Z80_GetOpcodeLength(U32 address)
{
	U8	opcode;
	U16	operands[8];
	U32	insCount;
	S32	a;

	opcode = Z80_MEM_getByte(address);

	if (Z80_Information[opcode])
	{
		for (a=0;a<Z80_Information[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & Z80_Information[opcode]->operandMask[a]) >> 
									Z80_Information[opcode]->operandShift[a];
		}
	}
			
	insCount=Z80_DisTable[opcode](address+1,operands);

	return insCount;
}


U32 Z80_DissasembleAddress(U32 x,U32 y,U32 address,int cursor)
{
	U32	insCount;
	S32	a;
	U32 b;
	int cMask1=0x0F,cMask2=0xFF;
	
	insCount=Z80_GetOpcodeLength(address);		/* note this also does the disassemble */

	for (a=0;a<Z80_numBps;a++)
	{
		if (address == Z80_bpAddresses[a])
		{
			cMask1=0x0F;
			cMask2=0x00;
			break;
		}
	}

	if (cursor)
	{
		PrintAt(cMask1,cMask2,x,y,"%08X >",address);
	}
	else
	{
		PrintAt(cMask1,cMask2,x,y,"%08X ",address);
	}
	
	for (b=0;b<insCount+1;b++)
	{
		PrintAt(cMask1,cMask2,x+10+b*5,y,"%02X ",Z80_MEM_getByte(address+b+0));
	}
			
	PrintAt(cMask1,cMask2,x+30,y,"%s",M68K::mnemonicData);
	
	return insCount+1;
}


U32 DissasembleAddress(U32 x,U32 y,U32 address,int cursor)
{
	U32	insCount;
	S32	a;
	U32 b;
	int cMask1=0x0F,cMask2=0xFF;
	
	insCount=GetOpcodeLength(address);		/* note this also does the dissasemble */

	for (a=0;a<numBps;a++)
	{
		if (address == bpAddresses[a])
		{
			cMask1=0x0F;
			cMask2=0x00;
			break;
		}
	}

	if (cursor)
	{
	PrintAt(cMask1,cMask2,x,y,"%08X >",address);
	}
	else
	{
	PrintAt(cMask1,cMask2,x,y,"%08X ",address);
	}
	
	for (b=0;b<(insCount+2)/2;b++)
	{
		PrintAt(cMask1,cMask2,x+10+b*5,y,"%02X%02X ",MEM_getByte(address+b*2+0),MEM_getByte(address+b*2+1));
	}
			
	PrintAt(cMask1,cMask2,x+30,y,"%s",M68K::mnemonicData);
	
	return insCount+2;
}

void DisplayHelp()
{
	DisplayWindow(84,0,30,20,0,0);

	PrintAt(debugDrawX,debugDrawY,85,1,"%s - Step Instruction", ion::input::KeycodeNames[(int)debuggerKeyStepInstruction]);
	PrintAt(debugDrawX,debugDrawY,85,2, "%s - Step Frame", ion::input::KeycodeNames[(int)debuggerKeyStepFrame]);
	PrintAt(debugDrawX,debugDrawY,85,3,"%s - Continue", ion::input::KeycodeNames[(int)debuggerKeyContinue]);
	PrintAt(debugDrawX,debugDrawY,85,4,"%s - Toggle Breakpoint", ion::input::KeycodeNames[(int)debuggerKeyToggleBreakpoint]);
	PrintAt(debugDrawX,debugDrawY,85,5,"%s/%s - Move cursor", ion::input::KeycodeNames[(int)debuggerKeyScrollUp], ion::input::KeycodeNames[(int)debuggerKeyScrollDown]);
	PrintAt(debugDrawX,debugDrawY,85,6,"%s - 68K debugger", ion::input::KeycodeNames[(int)debuggerKey68KMode]);
	PrintAt(debugDrawX,debugDrawY,85,7,"%s - Z80 debugger", ion::input::KeycodeNames[(int)debuggerKeyZ80Mode]);
	PrintAt(debugDrawX,debugDrawY,85,8,"%s - Sprite debugger", ion::input::KeycodeNames[(int)debuggerKeySpriteMode]);
}

/* Debug Function */
void DrawTile(int xx,int yy,U32 address,int pal,U32 flipH,U32 flipV)
{
	int x,y;
	int colour;

	address&=0xFFFF;

	for (y=0;y<8;y++)
	{
		for (x=0;x<8;x++)
		{
			if ((x&1)==0)
			{
				colour=(Memory::vRam[address+(y*4) + (x/2)]&0xF0)>>4;
			}
			else
			{
				colour=Memory::vRam[address+(y*4) + (x/2)]&0x0F;
			}
			
			if (colour!=0)
			{
				U8 r=colour;
				U8 gb=colour|(colour<<4);

				if (pal!=-1)
				{
					gb=Memory::cRam[pal*2*16+colour*2+0]&0x0E;
					r=Memory::cRam[pal*2*16+colour*2+1]&0x0E;
					gb|=Memory::cRam[pal*2*16+colour*2+1]&0xE0;
				}
				if (flipH && flipV)
				{
					doPixelNoScale((7-x)+xx,(7-y)+yy,r,gb);
				}
				if (!flipH && flipV)
				{
					doPixelNoScale(x+xx,(7-y)+yy,r,gb);
				}
				if (flipH && !flipV)
				{
					doPixelNoScale((7-x)+xx,y+yy,r,gb);
				}
				if (!flipH && !flipV)
				{
					doPixelNoScale(x+xx,y+yy,r,gb);
				}
			}
		}
	}
}

void SMS_DrawTile(int xx,int yy,U32 address,int pal,U32 flipH,U32 flipV)
{
	int x,y,p;
	int colour;
	U8 plane[4];
	U8 planeMask;
	U8 planeShift;

	address&=0x3FFF;

	for (y=0;y<8;y++)
	{
		for (p=0;p<4;p++)
		{
			plane[p]=Memory::vRam[address+y*4+p];
		}

		planeMask=0x80;
		planeShift=7;
		for (x=0;x<8;x++)
		{
			colour=((plane[3]&planeMask)>>planeShift)<<3;
			colour|=((plane[2]&planeMask)>>planeShift)<<2;
			colour|=((plane[1]&planeMask)>>planeShift)<<1;
			colour|=((plane[0]&planeMask)>>planeShift)<<0;

			planeMask>>=1;
			planeShift--;

			if (colour!=0)		/* not sure about transparency yet on sms */
			{
				U8 r=colour;
				U8 gb=colour|(colour<<4);

				if (pal!=-1)
				{
					r=(Memory::cRam[pal*16+colour]&0x03)<<2;
					gb=(Memory::cRam[pal*16+colour]&0x0C)<<4;
					gb|=(Memory::cRam[pal*16+colour]&0x30)>>2;
				}
				if (flipH && flipV)
				{
					doPixelNoScale((7-x)+xx,(7-y)+yy,r,gb);
				}
				if (!flipH && flipV)
				{
					doPixelNoScale(x+xx,(7-y)+yy,r,gb);
				}
				if (flipH && !flipV)
				{
					doPixelNoScale((7-x)+xx,y+yy,r,gb);
				}
				if (!flipH && !flipV)
				{
					doPixelNoScale(x+xx,y+yy,r,gb);
				}
			}
		}
	}
}

/* Slow clipping tile draw for sprites (until i do it properly!) */
void DrawTileClipped(int xx,int yy,U32 address,int pal,U32 flipH,U32 flipV)
{
	int x,y;
	int colour;

	address&=0xFFFF;

	for (y=0;y<8;y++)
	{
		for (x=0;x<8;x++)
		{
			if ((x&1)==0)
			{
				colour=(Memory::vRam[address+(y*4) + (x/2)]&0xF0)>>4;
			}
			else
			{
				colour=Memory::vRam[address+(y*4) + (x/2)]&0x0F;
			}
			
			if (colour!=0)
			{
				U8 r=colour;
				U8 gb=colour|(colour<<4);

				if (pal!=-1)
				{
					gb=Memory::cRam[pal*2*16+colour*2+0]&0x0E;
					r=Memory::cRam[pal*2*16+colour*2+1]&0x0E;
					gb|=Memory::cRam[pal*2*16+colour*2+1]&0xE0;

				}
				if (flipH && flipV)
				{
					doPixelClippedNoScale((7-x)+xx,(7-y)+yy,r,gb);
				}
				if (!flipH && flipV)
				{
					doPixelClippedNoScale(x+xx,(7-y)+yy,r,gb);
				}
				if (flipH && !flipV)
				{
					doPixelClippedNoScale((7-x)+xx,y+yy,r,gb);
				}
				if (!flipH && !flipV)
				{
					doPixelClippedNoScale(x+xx,y+yy,r,gb);
				}
			}
		}
	}
}

void DisplayApproximationOfScreen(int xx,int yy,int winNumber)
{
	int x,y;
	int displaySizeX = (VDP::VDP_Registers[12]&0x01) ? 40 : 32;
	int displaySizeY = (VDP::VDP_Registers[1]&0x08) ? 30 : 28;			/* not quite true.. ntsc can never be 30! */
	int windowSizeX,windowSizeY;
	S32 scrollAmount=((Memory::vsRam[2]&0x0F)<<8)|(Memory::vsRam[3]);
	U32 address=(VDP::VDP_Registers[0x04])&0x07;
	U32 baseAddress;
	address<<=13;

	if (winNumber==0)
	{
		scrollAmount=((Memory::vsRam[0]&0x0F)<<8)|(Memory::vsRam[1]);
		address=(VDP::VDP_Registers[0x02])&0x38;
		address<<=10;
	}
	baseAddress=address;

	switch (VDP::VDP_Registers[16] & 0x30)
	{
	default:
	case 0x20:			/* Marked Prohibited */
	case 0x00:
		windowSizeY=32;
		break;
	case 0x10:
		windowSizeY=64;
		break;
	case 0x30:
		windowSizeY=128;
		break;
	}
	switch (VDP::VDP_Registers[16] & 0x03)
	{
	default:
	case 2:			/* Marked Prohibited */
	case 0:
		windowSizeX=32;
		break;
	case 1:
		windowSizeX=64;
		break;
	case 3:
		windowSizeX=128;
		break;
	}

	if (scrollAmount&0x0800)
		scrollAmount|=0xFFFFF000;

	address-=baseAddress;
	address+=(windowSizeX*2)*(scrollAmount/8);
	address&=(windowSizeY*windowSizeX*2-1);
	address+=baseAddress;
	/* Dump screen representation? */



	for (y=0;y<displaySizeY;y++)
	{
		for (x=0;x<displaySizeX;x++)
		{
			U16 tile = (Memory::vRam[address+0]<<8)|Memory::vRam[address+1];

			U16 tileAddress = tile & 0x07FF;			/* pccv hnnn nnnn nnnn */

			int pal = (tile & 0x6000)>>13;

			tileAddress <<= 5;

			DrawTile(xx+x*8,y*8+yy,tileAddress,pal,tile&0x0800,tile&0x1000);

			address+=2;
		}

		address+=(windowSizeX*2)-(displaySizeX*2);
	}
}

void SMS_DisplayApproximationOfScreen(int xx,int yy)
{
	int x,y;
	int displaySizeX = 32;
	int displaySizeY = 24;
	U32 address=(VDP::VDP_Registers[0x02])&0x0E;

	address<<=10;
	address&=0x3FFF;

	for (y=0;y<displaySizeY;y++)
	{
		for (x=0;x<displaySizeX;x++)
		{
			U16 tile = (Memory::vRam[address+1]<<8)|Memory::vRam[address+0];

			U16 tileAddress = tile & 0x01FF;			/* ---p cvhn nnnn nnnn */

			int pal = (tile & 0x0800)>>11;

			tileAddress <<= 5;

			SMS_DrawTile(xx+x*8,y*8+yy,tileAddress,pal,tile&0x0200,tile&0x0400);

			address+=2;
		}
	}
}

int tileOffset=0;
int palIndex=-1;

void DisplaySomeTiles()
{
	int x,y;
	int address=0+32*tileOffset;
	/* Dump a few tiles */

	for (y=0;y<20;y++)
	{
		for (x=0;x<40;x++)
		{
			DrawTile(70*8+x*8,y*8+40*8,address,palIndex,0,0);
			address+=32;
		}
	}
}

void SMS_DisplaySomeTiles()
{
	int x,y;
	int address=0+32*tileOffset;
	/* Dump a few tiles */

	for (y=0;y<20;y++)
	{
		for (x=0;x<40;x++)
		{
			SMS_DrawTile(70*8+x*8,y*8+40*8,address,palIndex,0,0);
			address+=32;
		}
	}
}

void DisplayPalette()
{
	int x,y;
	for (y=0;y<4;y++)
	{
		for (x=0;x<16;x++)
		{
			U8 r=0;
			U8 gb=0;

			gb|=Memory::cRam[((y*16+x)*2)+0]&0x0E;
			r=Memory::cRam[((y*16+x)*2)+1]&0x0E;
			gb|=Memory::cRam[((y*16+x)*2)+1]&0xE0;
			DisplayWindow(30+x,0x14+32+(y*2),1,1,r,gb);
		}
	}
#if ENABLE_32X_MODE
	for (y=0;y<16;y++)
	{
		for (x=0;x<16;x++)
		{
			U32 colour;
			U8 bg=0;
			U8 gr=0;
			bg=CRAM[(x+y*16)*2+0];
			gr=CRAM[(x+y*16)*2+1];

			colour=0;
			colour|=(gr&0x1F)<<(3+8+8);
			colour|=(bg&0x7C)<<(1);
			colour|=(bg&0x03)<<(6+8);
			colour|=(gr&0xE0)<<(8-2);

			DisplayWindow32(40+x,0x14+32+(y*2),1,1,colour);
		}
	}
#endif
}

void SMS_DisplayPalette()
{
	int x,y;
	for (y=0;y<2;y++)
	{
		for (x=0;x<16;x++)
		{
			U8 r=0;
			U8 gb=0;

			r=(Memory::cRam[y*16+x]&0x03)<<2;
			gb|=(Memory::cRam[y*16+x]&0x0C)<<4;
			gb|=(Memory::cRam[y*16+x]&0x30)>>2;
			DisplayWindow(30+x,0x14+32+(y*2),1,1,r,gb);
		}
	}
}

void DisplaySprites()
{
	int x,y;
	U16 baseSpriteAddress = (VDP::VDP_Registers[5]&0x7F)<<9;
	U16 totSprites = 80; /*HACK*/
	U16 spriteAddress=baseSpriteAddress;
/* Sprite Data 


    Index + 0  :   ------yy yyyyyyyy
 Index + 2  :   ----hhvv
 Index + 3  :   -lllllll
 Index + 4  :   pccvhnnn nnnnnnnn
 Index + 6  :   ------xx xxxxxxxx

 y = Vertical coordinate of sprite
 h = Horizontal size in cells (00b=1 cell, 11b=4 cells)
 v = Vertical size in cells (00b=1 cell, 11b=4 cells)
 l = Link field
 p = Priority
 c = Color palette
 v = Vertical flip
 h = Horizontal flip
 n = Sprite pattern start index
 x = Horizontal coordinate of sprite

*/

	while (--totSprites)
	{
		U16 yPos = ( (Memory::vRam[spriteAddress+0]<<8) | (Memory::vRam[spriteAddress+1]) ) & 0x03FF;
		U16 size = Memory::vRam[spriteAddress+2] & 0x0F;
		U16 link = Memory::vRam[spriteAddress+3] & 0x7F;
		U16 ctrl = ( (Memory::vRam[spriteAddress+4]<<8) | (Memory::vRam[spriteAddress+5]) ) & 0xFFFF;
		U16 xPos = ( (Memory::vRam[spriteAddress+6]<<8) | (Memory::vRam[spriteAddress+7]) ) & 0x03FF;
		int vSize = size&0x03;
		int hSize = (size&0x0C)>>2;

		/* Need to draw correct number of sprites for one line etc.*/

		for (x=0;x<=hSize;x++)
		{
			for (y=0;y<=vSize;y++)
			{
				if ((ctrl&0x0800) && (ctrl&0x1000))		/* H & V flip */
				{
					DrawTileClipped(xPos+((hSize)-x)*8,yPos+((vSize)-y)*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000);
				}
				if (((ctrl&0x0800)==0) && (ctrl&0x1000))/* V flip */
				{
					DrawTileClipped(xPos+x*8,yPos+((vSize)-y)*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000);
				}
				if ((ctrl&0x0800) && ((ctrl&0x1000)==0))/* H flip */
				{
					DrawTileClipped(xPos+((hSize)-x)*8,yPos+y*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000);
				}
				if (((ctrl&0x0800)==0) && ((ctrl&0x1000)==0))		/* no flip */
				{
					DrawTileClipped(xPos+x*8,yPos+y*8,((ctrl&0x07FF)<<5)+y*32 +x*(vSize+1)*32,(ctrl&0x6000)>>13,ctrl&0x0800,ctrl&0x1000);
				}

			}
		}

		if (link == 0)
			break;

		spriteAddress=baseSpriteAddress+link*8;
		spriteAddress&=0xFFFF;
	}
}

#if ENABLE_32X_MODE
extern U8 SH2_68K_COMM_REGISTER[16];
extern U16 FIFO_BUFFER[8];

void DisplayComm()
{
	int y;

	for (y=0;y<8;y++)
	{
		PrintAt(debugDrawX,debugDrawY,60+y*4,0x12+39,"%04X",FIFO_BUFFER[y]);
	}
	for (y=0;y<16;y++)
	{
		PrintAt(debugDrawX,debugDrawY,60+y*2,0x12+40,"%02X",SH2_68K_COMM_REGISTER[y]);
	}
}
#endif
void DisplayCustomRegs()
{
	int y;
	static char buffer[256]={0};

	DisplayWindow(0,31,14*8+1,512/16+2,0,0);
	
	for (y=0;y<0x20;y++)
	{
		VDP_GetRegisterContents(y,buffer);
		PrintAt(debugDrawX,debugDrawY,1, y+32, buffer);
	}

	for (y=0;y<0x20;y+=2)
	{
		IO_GetRegisterContents(y,buffer);
		PrintAt(debugDrawX,debugDrawY,30,y/2+32,buffer);
	}

	sprintf(buffer,"Line %03d, Col %03d", Globals::lineNo, Globals::colNo);
	PrintAt(debugDrawX,debugDrawY,30,0x12+32,buffer);

	DisplayPalette();
	DisplaySomeTiles();
	DisplayApproximationOfScreen(70*8,16*8,0);
}

extern U16	PSG_RegsCounters[4];
extern U16	PSG_ToneCounter[4];
extern U16	PSG_ToneOut[4];
extern U8	PSG_RegsAttenuation[4];
extern U16	PSG_NoiseShiftRegister;

#if SMS_MODE
extern U8 SMS_MemPageRegister;
extern U32 SMS_Slot0_Bank;
extern U32 SMS_Slot1_Bank;
extern U32 SMS_Slot2_Bank;
extern U8 SMS_Ram_Control;

void SMS_DisplayCustomRegs()
{
	int y;
	static char buffer[256]={0};

	DisplayWindow(0,31,14*8+1,512/16+2,0,0);
	
	for (y=0;y<0x10;y++)
	{
		VDP_GetRegisterContents(y,buffer);
		PrintAt(debugDrawX,debugDrawY,1, y+32, buffer);
	}

	SMS_DisplayPalette();
	
	SMS_DisplaySomeTiles();
	
	SMS_DisplayApproximationOfScreen(70*8,16*8);

	for (y=0;y<4;y++)
	{
		PrintAt(debugDrawX,debugDrawY,30,y+32,"TN %04X", PSG_RegsCounters[y]);
		PrintAt(debugDrawX,debugDrawY,40,y+32,"TC %04X",PSG_ToneCounter[y]);
		PrintAt(debugDrawX,debugDrawY,50,y+32,"TO %04X",PSG_ToneOut[y]);
		PrintAt(debugDrawX,debugDrawY,60,y+32,"V  %02X", PSG_RegsAttenuation[y]);
	}
	PrintAt(debugDrawX,debugDrawY,30,4+32,"Noise %04X",PSG_NoiseShiftRegister);

	PrintAt(debugDrawX,debugDrawY,30,6+32,"MemSelect %02X",SMS_MemPageRegister);
	PrintAt(debugDrawX,debugDrawY,30,7+32,"Page 0 %08X",SMS_Slot0_Bank);
	PrintAt(debugDrawX,debugDrawY,30,8+32,"Page 1 %08X",SMS_Slot1_Bank);
	PrintAt(debugDrawX,debugDrawY,30,9+32,"Page 2 %08X",SMS_Slot2_Bank);
	PrintAt(debugDrawX,debugDrawY,30,10+32,"Ram Control %02X",SMS_Ram_Control);
	
/*
	for (y=0;y<0x20;y+=2)
	{
		IO_GetRegisterContents(y,buffer);
		PrintAt(debugDrawX,debugDrawY,30,y/2+32,buffer);
	}

	sprintf(buffer,"Line %03d, Col %03d",lineNo,colNo);
	PrintAt(debugDrawX,debugDrawY,30,0x12+32,buffer);

	DisplayPalette();
*/
}
#endif

U32 lastPC;
#define PCCACHESIZE	1000

U32	pcCache[PCCACHESIZE];
U32	cachePos=0;

void DecodePCHistory(int offs)
{
	int a;
	DisplayWindow(0,0,14*8+1,31+34,0,0);

	for (a=0;a<32+31;a++)
	{
		if (a+offs < PCCACHESIZE)
		{
			PrintAt(debugDrawX,debugDrawY,1,1+a,"%d : PC History : %08X\n",a+offs,pcCache[a+offs]);
		}
	}
}

int cpOffs = 0;
int cpuOffs = 0;
int Z80Offs = 0;
int hisOffs = 0;
			
int bpAt=0xFFFFFFFF;

void Z80_BreakpointModify(U32 address)
{
	int a;
	int b;

	for (a=0;a<Z80_numBps;a++)
	{
		if (address==Z80_bpAddresses[a])
		{
			/* Remove breakpoint */
			Z80_numBps--;

			for (b=a;b<Z80_numBps;b++)
			{
				Z80_bpAddresses[b]=Z80_bpAddresses[b+1];
			}
			return;
		}
	}

	Z80_bpAddresses[Z80_numBps]=address;
	Z80_numBps++;
}

void BreakpointModify(U32 address)
{
	int a;
	int b;

	for (a=0;a<numBps;a++)
	{
		if (address==bpAddresses[a])
		{
			/* Remove breakpoint */
			numBps--;

			for (b=a;b<numBps;b++)
			{
				bpAddresses[b]=bpAddresses[b+1];
			}
			return;
		}
	}

	bpAddresses[numBps]=address;
	numBps++;
}

int spriteOffset=0;

void DecodeSpriteTable()
{
	U16 baseSpriteAddress = (VDP::VDP_Registers[5]&0x7F)<<9;
	U16 spriteAddress;
	U16 totSpritesScreen = (VDP::VDP_Registers[12]&0x01) ? 80 : 64;
	U16 curLink =0;
	int line=0;
	int actualOffset=spriteOffset;

	DisplayWindow(0,0,14*8+1,31+34,0,0);

	spriteAddress=baseSpriteAddress;
	while (1)	
	{
		U16 yPos = ( (Memory::vRam[spriteAddress+0]<<8) | (Memory::vRam[spriteAddress+1]) ) & 0x03FF;
		U16 size = Memory::vRam[spriteAddress+2] & 0x0F;
		U16 link = Memory::vRam[spriteAddress+3] & 0x7F;
		U16 xPos = ( (Memory::vRam[spriteAddress+6]<<8) | (Memory::vRam[spriteAddress+7]) ) & 0x03FF;
		int vSize = size&0x03;
		int hSize = (size&0x0C)>>2;

		if (actualOffset==0)
		{
			PrintAt(debugDrawX,debugDrawY,1,1+line,"%d->%d : %d,%d [%d,%d]",curLink,link,xPos,yPos,(hSize+1)*8,(vSize+1)*8);

			line++;
			if (line>=64)
				break;
		}
		else
		{
			actualOffset--;
		}

		if (link == 0)
			break;

		curLink=link;
		if (curLink>=totSpritesScreen)
			break;

		spriteAddress=baseSpriteAddress+link*8;
		spriteAddress&=0xFFFF;
	}
}

void DisplayZ80Dis(int offs)
{
	int a;
	U16 address = Z80::Z80_regs.lastInstruction;

	UNUSED_ARGUMENT(offs);

		
	if (Z80::Z80_regs.stage==0)
	{
		address=Z80::Z80_regs.PC;
	}

	DisplayWindow(0,19,70,12,0,0);
	for (a=0;a<10;a++)
	{
		if (bpAt==a)
		{
			Z80_BreakpointModify(address);
			bpAt=0xFFFFFFFF;
		}

		address+=Z80_DissasembleAddress(1,20+a,address,Z80Offs==a);
	}
}

void ShowYM2612()
{
	int a,b;

	DisplayWindow(60,0,60,70,0,0);

	for (b=0;b<2;b++)
	{
		for (a=0;a<32;a++)
		{
			PrintAt(debugDrawX,debugDrawY,60+b*26+ 0,a,"%02X",Memory::YM2612_Regs[b][a*8+0]);
			PrintAt(debugDrawX,debugDrawY,60+b*26+ 3,a,"%02X",Memory::YM2612_Regs[b][a*8+1]);
			PrintAt(debugDrawX,debugDrawY,60+b*26+ 6,a,"%02X",Memory::YM2612_Regs[b][a*8+2]);
			PrintAt(debugDrawX,debugDrawY,60+b*26+ 9,a,"%02X",Memory::YM2612_Regs[b][a*8+3]);
			PrintAt(debugDrawX,debugDrawY,60+b*26+12,a,"%02X",Memory::YM2612_Regs[b][a*8+4]);
			PrintAt(debugDrawX,debugDrawY,60+b*26+15,a,"%02X",Memory::YM2612_Regs[b][a*8+5]);
			PrintAt(debugDrawX,debugDrawY,60+b*26+18,a,"%02X",Memory::YM2612_Regs[b][a*8+6]);
			PrintAt(debugDrawX,debugDrawY,60+b*26+21,a,"%02X",Memory::YM2612_Regs[b][a*8+7]);
		}
	}
}

void Display68000Dis(int offs)
{
	int a;
#if CPU_DEBUG_CALLTRACE
	U32 address = M68K::cpu_regs.lastInstruction;
#else
	U32 address = M68K::cpu_regs.PC;
#endif

	if (M68K::cpu_regs.stage==0)
	{
		address=M68K::cpu_regs.PC;
	}

	DisplayWindow(0,19+offs,70,12,0,0);
	for (a=0;a<10;a++)
	{
		if (bpAt==a)
		{
			BreakpointModify(address);
			bpAt=0xFFFFFFFF;
		}

		address+=DissasembleAddress(1,20+a+offs,address,cpuOffs==a);
	}
}

void DisplayDebugger()
{
	int y;

	VDP_WriteLock();

	if (Globals::g_pause || (stageCheck == DebugState::Continue68K) || (stageCheck == DebugState::ContinueZ80))
	{
		if (dbMode == DebugMode::M68K)
		{
			ShowCPUState(0);
		
			DisplayHelp();
		
			DisplayCustomRegs();
#if ENABLE_32X_MODE
			DisplayComm();
#endif	
			Display68000Dis(0);
		}
		if (dbMode== DebugMode::M68K_History)
		{
			DecodePCHistory(hisOffs);
		}
		if (dbMode== DebugMode::Sprites)
		{
			DecodeSpriteTable();
		}
		if (dbMode== DebugMode::Z80)
		{
			ShowZ80State(0);
		
			DisplayZ80Dis(0);

#if !SMS_MODE
			ShowCPUState(32);
			
			Display68000Dis(32);

			ShowYM2612();

			for (y=0;y<4;y++)
			{
				PrintAt(debugDrawX,debugDrawY,60,y+32,"TN %04X", PSG_RegsCounters[y]);
				PrintAt(debugDrawX,debugDrawY,70,y+32,"TC %04X",PSG_ToneCounter[y]);
				PrintAt(debugDrawX,debugDrawY,80,y+32,"TO %04X",PSG_ToneOut[y]);
				PrintAt(debugDrawX,debugDrawY,90,y+32,"V  %02X", PSG_RegsAttenuation[y]);
			}
			PrintAt(debugDrawX,debugDrawY,60,4+32,"Noise %04X",PSG_NoiseShiftRegister);

#else
			SMS_DisplayCustomRegs();
#endif
#if ENABLE_32X_MODE
			DisplayComm();
#endif
		}
#if ENABLE_32X_MODE
		if (dbMode==DEB_Mode_SH2_Master)
		{
			ShowSH2State(master);

			DisplaySH2Dis(master);
			
			DisplayComm();
		}
		if (dbMode==DEB_Mode_SH2_Slave)
		{
			ShowSH2State(slave);

			DisplaySH2Dis(slave);
			
			DisplayComm();
		}
#endif
		
		Globals::g_newScreenNotify=1;
	}

	VDP_WriteUnlock();
}

extern U32 Z80Cycles;

void DebuggerStep()
{
	if (stageCheck != DebugState::Idle)
	{
		if (stageCheck == DebugState::Step68K)
		{
			if (M68K::cpu_regs.stage == 0)
			{
				Globals::g_pause = 1;		/* single step cpu */
				stageCheck = DebugState::Idle;
			}
		}
		if (stageCheck == DebugState::StepHardware)
		{
			Globals::g_pause = 1;			/* single step hardware */
			stageCheck = DebugState::Idle;
		}
		if (stageCheck == DebugState::StepZ80)
		{
			if ((Z80::Z80_regs.stage == 0) && (Z80Cycles == 0))
			{
				Globals::g_pause = 1;
				stageCheck = DebugState::Idle;
			}
		}
		if (stageCheck == DebugState::StepFrame68K)
		{
			Globals::g_pause = 1;
		}
		if (stageCheck == DebugState::StepFrameZ80)
		{
			Globals::g_pause = 1;
		}

#if ENABLE_32X_MODE
		if (stageCheck == (DEB_Mode_SH2_Master + 1))
		{
			Globals::g_pause = 1;
			stageCheck = 0;
		}
#endif
	}

	if (M68K::cpu_regs.stage == 0)
	{
		for (int a = 0; a < numBps; a++)
		{
			if (bpAddresses[a] == M68K::cpu_regs.PC)
			{
				if (!Globals::g_pause)
				{
					dbMode = DebugMode::M68K;
				}
				Globals::g_pause = 1;
			}
		}
	}
	if (Z80::Z80_regs.stage == 0)
	{
		for (int a = 0; a < Z80_numBps; a++)
		{
			if (Z80_bpAddresses[a] == Z80::Z80_regs.PC)
			{
				if (!Globals::g_pause)
				{
					dbMode = DebugMode::Z80;
				}
				Globals::g_pause = 1;
			}
		}
	}
}

#define ENABLE_PC_HISTORY		0
int UpdateDebugger(const ion::input::Keyboard& keyboard)
{
#if 1
	if (M68K::cpu_regs.stage == 0)
	{
#if ENABLE_PC_HISTORY
		lastPC = M68K::cpu_regs.PC;

		if (cachePos < PCCACHESIZE)
		{
			if (pcCache[cachePos] != lastPC)
			{
				pcCache[cachePos++] = lastPC;
			}
		}
		else
		{
			if (pcCache[PCCACHESIZE - 1] != lastPC)
			{
				memmove(pcCache, pcCache + 1, (PCCACHESIZE - 1) * sizeof(U32));

				pcCache[PCCACHESIZE - 1] = lastPC;
			}
		}
#endif
	}

	//Break to debugger
	if (keyboard.KeyPressedThisFrame(debuggerKeyBreak))
	{
		Globals::g_pause = !Globals::g_pause;
		stageCheck = DebugState::Idle;

		if(Globals::g_pause)
			dbMode = DebugMode::M68K;
		else
			dbMode = DebugMode::Off;
	}

	//Mode select
	if (keyboard.KeyPressedThisFrame(debuggerKeyZ80Mode))
		dbMode = DebugMode::Z80;

	if (keyboard.KeyPressedThisFrame(debuggerKey68KMode))
		dbMode = DebugMode::M68K;

	if (keyboard.KeyPressedThisFrame(debuggerKey68KHistoryMode))
		dbMode = DebugMode::M68K_History;

	if (keyboard.KeyPressedThisFrame(debuggerKeySpriteMode))
		dbMode = DebugMode::Sprites;

	//Save/load state
	if (keyboard.KeyPressedThisFrame(debuggerKeySaveState))
	{
		/* Save state */
		FILE *save=fopen("mega_save.dmp","wb");

		MEM_SaveState(save);
		CPU_SaveState(save);

		fclose(save);
	}
	if (keyboard.KeyPressedThisFrame(debuggerKeyLoadState))
	{
		/* Load state */

		FILE *load=fopen("mega_save.dmp","rb");

		if (load)
		{
			MEM_LoadState(load);
			CPU_LoadState(load);
			fclose(load);
		}
	}

	if (Globals::g_pause || (stageCheck == DebugState::Continue68K) || (stageCheck == DebugState::ContinueZ80)) // || (stageCheck==6) || (stageCheck==9))
	{
		/////////////////////////////////////////////////////////////////////////////////////////////////
		// Z80 MODE
		/////////////////////////////////////////////////////////////////////////////////////////////////
		if (dbMode== DebugMode::Z80)
		{
			/* While paused - enable debugger keys */
			if (keyboard.KeyPressedThisFrame(debuggerKeyStepInstruction))
			{
				/* cpu step into instruction */
				stageCheck = DebugState::StepZ80;

				Globals::g_pause=0;
			}
			if (keyboard.KeyPressedThisFrame(debuggerKeyStepFrame))
			{
				//Step one render frame
				stageCheck= DebugState::StepFrameZ80;
				Globals::g_pause=0;
			}
			if (keyboard.KeyPressedThisFrame(debuggerKeyContinue))
			{
				if (stageCheck==DebugState::ContinueZ80)
				{
					stageCheck= DebugState::Idle;
					dbMode = DebugMode::Off;
					Globals::g_pause=1;
				}
				else
				{
					stageCheck= DebugState::ContinueZ80;
					Globals::g_pause=0;
				}
			}
			if (keyboard.KeyPressedThisFrame(debuggerKeyToggleBreakpoint))
				bpAt=Z80Offs;
			if (keyboard.KeyPressedThisFrame(debuggerKeyScrollUp) && Z80Offs>0)
				Z80Offs--;
			if (keyboard.KeyPressedThisFrame(debuggerKeyScrollDown) && Z80Offs<9)
				Z80Offs++;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// 68K HISTORY MODE
		/////////////////////////////////////////////////////////////////////////////////////////////////
		if (dbMode== DebugMode::M68K_History)
		{
			if (keyboard.KeyPressedThisFrame(debuggerKeyScrollUp))
			{
				hisOffs-=32;
				if (hisOffs<0)
					hisOffs=0;
			}
			if (keyboard.KeyPressedThisFrame(debuggerKeyScrollDown))
			{
				hisOffs+=32;
				if (hisOffs>=PCCACHESIZE)
					hisOffs=PCCACHESIZE-1;
			}
		}
		if (CheckKey('C'))
			Z80_Reset();

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// SPRITES MODE
		/////////////////////////////////////////////////////////////////////////////////////////////////
		if (dbMode== DebugMode::Sprites)
		{
			if (CheckKey('W'))
			{
				spriteOffset--;
				if (spriteOffset<=0)
					spriteOffset=0;
			}
			if (CheckKey('S'))
			{
				spriteOffset++;
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		// 68K MODE
		/////////////////////////////////////////////////////////////////////////////////////////////////
		if (dbMode== DebugMode::M68K)
		{
			/* While paused - enable debugger keys */
			if (keyboard.KeyPressedThisFrame(debuggerKeyStepInstruction))
			{
				/* cpu step into instruction */
				stageCheck= DebugState::Step68K;

				Globals::g_pause=0;
			}
			if (keyboard.KeyPressedThisFrame(debuggerKeyStepFrame))
			{
				//Step one render frame
				stageCheck= DebugState::StepFrame68K;
				Globals::g_pause=0;
			}
			if (keyboard.KeyPressedThisFrame(debuggerKeyContinue))
			{
				if (stageCheck== DebugState::Continue68K)
				{
					stageCheck= DebugState::Idle;
					dbMode = DebugMode::Off;
					Globals::g_pause=1;
				}
				else
				{
					stageCheck= DebugState::Continue68K;
					Globals::g_pause=0;
				}
			}
			if (CheckKey('Q'))
			{
				palIndex++;
				if (palIndex>3)
					palIndex=-1;
			}
			if (CheckKey('A'))
			{
				tileOffset-=20*32;
			}
			if (CheckKey('Z'))
			{
				tileOffset+=20*32;
			}
			if (keyboard.KeyPressedThisFrame(debuggerKeyScrollUp) && cpuOffs>0)
				cpuOffs--;
			if (keyboard.KeyPressedThisFrame(debuggerKeyScrollDown) && cpuOffs<9)
				cpuOffs++;
			if (keyboard.KeyPressedThisFrame(debuggerKeyToggleBreakpoint))
				bpAt=cpuOffs;
		}
	}
#endif
	return Globals::g_pause;
}
#endif
