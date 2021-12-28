/*
 *  memory.c

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "globals.h"
#include "cpu/z80/z80.h"
#include "vdp/vdp.h"
#include "memory.h"
#include "audio/psg.h"
#include "audio/lj_ym2612.h"
#include "mgaudio.h"
#include "gui/debugger.h"
#include "emulator.h"

//#include "memory.inl"

#define SYS_RAM_SIZE		(64*1024)
#define Z80_RAM_SIZE		(8*1024)
#if VRAM_128KB_MODE
#define VRAM_SIZE			(128*1024)
#else
#define VRAM_SIZE			(64*1024)
#endif
#define CRAM_SIZE			(128)
#define VSRAM_SIZE			(80)
#define CARTRAM_SIZE		(32*1024)		/* this is probably too big - 8k on most carts aparantly */

#define IO_6BUTTON_LATCH_WRITE_COUNT	4
#define IO_6BUTTON_LATCH_CYCLES			CYCLES_PER_FRAME_68K/2	//There's no fine art to the timing of this, depends on a capacitor in the controller going flat, differs by manufacturer/parts quality/condition/age

//HI and LO latches as state machines
#define IO_6BUTTON_LATCH_HI_STATE_1 1
#define IO_6BUTTON_LATCH_LO_STATE_1 1
#define IO_6BUTTON_LATCH_LO_STATE_2 0

MEM_ReadByteMap Memory::mem_read[256];
MEM_WriteByteMap Memory::mem_write[256];

MEM_ReadWordMap Memory::mem_read_word[256];
MEM_WriteWordMap Memory::mem_write_word[256];

MEM_DMAMap Memory::mem_dma_addr[256];

U8 Memory::YM2612_Regs[2][256];
U8 Memory::YM2612_AddressLatch1;
U8 Memory::YM2612_AddressLatch2;

unsigned char* Memory::vRam;
unsigned char* Memory::cRam;
unsigned char* Memory::vsRam;
unsigned char* Memory::cartRom;
unsigned char* Memory::cartRam;
unsigned char* Memory::smsBios;
unsigned char* Memory::systemRam;
unsigned char* Memory::z80Ram;

U32 Memory::bankAddress = 0;

U32 cartRomSize = 0;

/*

 Z80 Address Space		- In Genesis mode

 0000 - 1FFF			RAM
 2000 - 3FFF			RAM [mirror]
 4000 - 5FFF			YM2612							->			4000 A0		4001 D0		4002 A1		4003 D1		(mirrored throughout range)
 6000 - 60FF			Bank Address Register				(reads return 0xFF) (writes set bank register)
 6100 - 7EFF			Unmapped										(reads return 0xFF)	(writes are ignored)
 7F00	-	7FFF			VDP													(writes to 7F20-7FFF will lock up machine)
 8000 - FFFF			Bank area

*/

/* 9 bits must be set to set the bank address - bit 0 of bank is next bit of sequence */
void BANK_Set(U8 bank)
{
	Memory::bankAddress>>=1;
	if (bank&1)
	{
		Memory::bankAddress|=0x00800000;
	}

	Memory::bankAddress&=0x00FF8000;
}

U8 BANKED_Read(U32 address)
{
	return MEM_getByte(Memory::bankAddress|address);
}

void BANKED_Write(U32 address, U8 data)
{
	MEM_setByte(Memory::bankAddress|address,data);
}

U8 YM2612_Read(U8 address)
{
	U8 data;
	LJ_YM2612_setAddressPinsCSRDWRA1A0(YM2612::ym2612_chip,0,0,1,(address&0x0002)>>1,(address&0x0001));
	LJ_YM2612_getDataPinsD07(YM2612::ym2612_chip,&data);
	return data;
}

void WriteRawByte(U16 data)
{
	FILE *bob = fopen("test.raw","a");				/* DAC output is unsigned 8 bit 8000khz - apparantly */
	if (bob)
	{
		fwrite(&data,2,1,bob);
		fclose(bob);
	}
}

/* 30h ch1 op 1 */
/* 31h ch2 op 1 */

/* 34h ch1 op 2 */

float step=0.0f;
float step2=0.0f;
int play=0;
float stepRate;

/* Lets start by generating a simple sine wave and jamming it into the dac */


int inited=0;

void YM2612_Write(U8 address,U8 data)
{
	LJ_YM2612_setDataPinsD07(YM2612::ym2612_chip,data);
	LJ_YM2612_setAddressPinsCSRDWRA1A0(YM2612::ym2612_chip,0,1,0,(address&0x0002)>>1,(address&0x0001));
	switch (address)
	{
	case 0:			/* A0 */
		Memory::YM2612_AddressLatch1=data;
		break;
	case 1:			/* D0 */
		Memory::YM2612_Regs[0][Memory::YM2612_AddressLatch1]=data;
		break;
	case 2:			/* A1 */
		Memory::YM2612_AddressLatch2=data;
		break;
	case 3:			/* D1 */
		Memory::YM2612_Regs[1][Memory::YM2612_AddressLatch2]=data;
		break;
	}
}

#if !SMS_MODE

U8 Z80_IO_getByte(U16 address)
{
	UNUSED_ARGUMENT(address);
	return 0xFF;
}

void Z80_IO_setByte(U16 address,U8 data)
{
	UNUSED_ARGUMENT(address);
	UNUSED_ARGUMENT(data);
}

U8 Z80_MEM_getByte(U16 address)
{
	switch (address&0xF000)
	{
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
		return Memory::z80Ram[address&0x1FFF];
	case 0x4000:
	case 0x5000:
		return YM2612_Read(address&0x3);
	case 0x6000:
	case 0x7000:
		return 0xFF;													/* TODO VDP reads */
	case 0x8000:
	case 0x9000:
	case 0xA000:
	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:
		return BANKED_Read(address-0x8000);
	}

	return 0xFF;
}

extern void VDP_WriteByte(U16 offset, U8 byte);

void Z80_MEM_setByte(U16 address,U8 data)
{
	switch (address&0xF000)
	{
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
		Memory::z80Ram[address&0x1FFF] = data;
		break;
	case 0x4000:
	case 0x5000:
		YM2612_Write(address&0x3,data);
		break;
	case 0x6000:
		if (address<0x6100)
		{
			BANK_Set(data);
		}
		break;
	case 0x7000:
		if (address>0x7F00)
		{
			VDP_WriteByte(address & 0xFF, data);
		}
		break;
	case 0x8000:
	case 0x9000:
	case 0xA000:
	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:
		BANKED_Write(address-0x8000,data);
		break;
	}
}
#else

#if ENABLE_SMS_BIOS
U8 SMS_MemPageRegister=0xE0;
#else
U8 SMS_MemPageRegister=0xA8;
#endif
U8 SMS_VDP_Status=0x00;
U8 SMS_VDP_msbWrite=0;
U8 SMS_VDP_DataLatch=0;
U16 SMS_VDP_AddressLatch=0;
U16 SMS_VDP_CodeLatch=0;

extern unsigned long romSize;

extern U8 SMS_KeyJoyA;

void SMS_VDP_Data(U8 data)
{
	SMS_VDP_DataLatch=data;
	switch (SMS_VDP_CodeLatch)
	{
	default:
		Memory::vRam[SMS_VDP_AddressLatch&0x3FFF]=data;
		break;
	case 0xC000:
		Memory::cRam[SMS_VDP_AddressLatch&0x1F]=data;
		break;
	}
	SMS_VDP_AddressLatch++;
}

void SMS_VDP_Address(U8 data)
{
	if (SMS_VDP_msbWrite)
	{
		SMS_VDP_msbWrite=0;
		SMS_VDP_AddressLatch&=0x00FF;
		SMS_VDP_AddressLatch|=(data<<8);

		SMS_VDP_CodeLatch=SMS_VDP_AddressLatch&0xC000;

		/* got complete value... do action*/
		
		switch (SMS_VDP_CodeLatch)
		{
		case 0x0000:
			SMS_VDP_DataLatch=Memory::vRam[SMS_VDP_AddressLatch&0x3FFF];
			SMS_VDP_AddressLatch++;
			break;
		case 0x4000:
			break;
		case 0x8000:
			VDP::VDP_Registers[(SMS_VDP_AddressLatch>>8)&0x0F]=SMS_VDP_AddressLatch&0xFF;
			break;
		case 0xC000:
			break;
		}
	}
	else
	{
		SMS_VDP_AddressLatch&=0xFF00;
		SMS_VDP_AddressLatch|=data;
		SMS_VDP_msbWrite=1;
	}
}

U8 Z80_IO_getByte(U16 address)
{
	address&=0xC1;

	switch (address)
	{
	case 0x40:		/* Vertical Position Latch */
		if (lineNo>0xFF)													/* TODO : this counter should wrap differently depending on screen height and PAL/NTSC*/
			return 0xFF;
		return lineNo&0xFF;
	case 0x41:		/* Horizontal Position Latch*/
		return colNo &0xFF;
	case 0x80:		/* VDP Data*/
		{
			U8 oldVal=SMS_VDP_DataLatch;
		
			SMS_VDP_DataLatch=Memory::vRam[SMS_VDP_AddressLatch&0x3FFF];
			SMS_VDP_AddressLatch++;
			SMS_VDP_msbWrite=0;

			return oldVal;
		}
	case 0x81:		/* VDP Status*/
		{
			U8 oldStatus=SMS_VDP_Status;
			SMS_VDP_msbWrite=0;
			SMS_VDP_Status&=~0xC0;
			return oldStatus;
		}
	case 0xC0:		/* JoyAB*/
		{
			return ~SMS_KeyJoyA;
		}
	}
/*
	EMU_PRINTF("IO Read : %04X\n",address);
*/
	return 0xFF;
}

void Z80_IO_setByte(U16 address,U8 data)
{
	U16 smsAddress=address&0xC1;

	switch (smsAddress)
	{
	case 0x00:		/* Mem Control */
		SMS_MemPageRegister=data;
		return;
	case 0x40:		/* PSG */
	case 0x41:		/* PSG */
		PSG_Write(data);
		return;
	case 0x80:		/* VDP Data */
		SMS_VDP_Data(data);
		return;
	case 0x81:		/* VDP Control */
		SMS_VDP_Address(data);
		return;
	}

	EMU_PRINTF("IO Write : %04X <- %02X\n",address,data);
}

U32 SMS_Slot0_Bank=0x0000;
U32 SMS_Slot1_Bank=0x4000;
U32 SMS_Slot2_Bank=0x8000;
U8 SMS_Ram_Control=0;			/* not supported yet */
U8 SMS_segaMapperSwitched=0;

U8 Z80_MEM_getByte(U16 address)
{
	if (((SMS_MemPageRegister&0x08)==0)&&address<0x4000)
	{
		return Memory::smsBios[address];
	}
	if ((SMS_MemPageRegister&0x40) && address < 0xC000)
	{
		return 0xFF;
	}

	switch (address&0xF000)
	{
	case 0x0000:
		if (address<0x400 && SMS_segaMapperSwitched)
		{
			return Memory::cartRom[address];				/* <1k can't be mapped out */
		}																						/* fall through intended */
	case 0x1000:
	case 0x2000:
	case 0x3000:
#if SMS_CART_MISSING
		return 0xFF;
#else
		return Memory::cartRom[(address&0x3FFF) + SMS_Slot0_Bank];
#endif
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
#if SMS_CART_MISSING
		return 0xFF;
#else
		return Memory::cartRom[(address&0x3FFF) + SMS_Slot1_Bank];
#endif
	case 0x8000:
	case 0x9000:
	case 0xA000:
	case 0xB000:
#if SMS_CART_MISSING
		return 0xFF;
#else
		if (SMS_Ram_Control&0x08)
		{
			return Memory::cartRam[(address&0x3FFF)];
		}
		return Memory::cartRom[(address&0x3FFF) + SMS_Slot2_Bank];
#endif
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:
		if (SMS_MemPageRegister&0x10)
			return 0xFF;
		return Memory::z80Ram[address&0x1FFF];
	}

	return 0xFF;
}

void Z80_MEM_setByte(U16 address,U8 data)
{
	if (address==0xFFFC)
	{
		SMS_Ram_Control=data;
		if (SMS_Ram_Control&0x17)
		{
			EMU_PRINTF("Unhandled RAM Control : %02X\n",SMS_Ram_Control);
			DEB_PauseEmulation(DebugMode::Z80,"RAM Mapper unhandled");
		}
		if (SMS_Ram_Control&0x08)
		{
			SRAM=Memory::cartRam;
			SRAM_Size=32*1024;
		}
	}
	if (address==0xFFFD || address==0x0000)
	{
		SMS_Slot0_Bank=data;
		SMS_Slot0_Bank&=(romSize/16384)-1;			/* won't work for non power 2 - does that exist? */
		SMS_Slot0_Bank<<=14;
		if (address<0xF000)
		{
			SMS_segaMapperSwitched=0;
		}
		else
		{
			SMS_segaMapperSwitched=1;
		}
	}
	if (address==0xFFFE || address==0x4000)
	{
		SMS_Slot1_Bank=data;
		SMS_Slot1_Bank&=(romSize/16384)-1;			/* won't work for non power 2 - does that exist? */
		SMS_Slot1_Bank<<=14;
	}
	if (address==0xFFFF || address==0x8000)
	{
		SMS_Slot2_Bank=data;
		SMS_Slot2_Bank&=(romSize/16384)-1;			/* won't work for non power 2 - does that exist? */
		SMS_Slot2_Bank<<=14;
	}

	switch (address&0xF000)
	{
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
		break;
	case 0x8000:
	case 0x9000:
	case 0xA000:
	case 0xB000:
		if (SMS_Ram_Control&0x08)
		{
			Memory::cartRam[(address&0x3FFF)]=data;
		}
		return;
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:
		if (SMS_MemPageRegister&0x10)
			return;
		Memory::z80Ram[address&0x1FFF]=data;
		break;
	}
}
#endif

/* 

 MEGA Memory Map

1.) 68000 memory map

 000000-3FFFFFh : ROM
 400000-7FFFFFh : Unused (1)
 800000-9FFFFFh : Unused (2)
 A00000-A0FFFFh : Z80 address space (3)
 A10000-A1001Fh : I/O
 A10020-BFFFFFh : Internal registers and expansion (4)
 C00000-DFFFFFh : VDP (5)
 E00000-FFFFFFh : RAM (6)

 1. Reads return the MSB of the next instruction to be fetched, with the
    LSB set to zero. Writes do nothing.

 2. Reading or writing any address will lock up the machine.
    This area is used for the 32X adapter.

 3. Addresses A08000-A0FFFFh mirror A00000-A07FFFh, so the 68000 cannot
    access it's own banked memory. All addresses are valid except for
    the VDP which is at A07F00-A07FFFh and A0FF00-A0FFFFh, writing or
    reading those addresses will lock up the machine.

 4. Reading some addresses lock up the machine, others return the MSB
    of the next instruction to be fetched with the LSB is set to zero.

    The latter applies when reading A11100h (except for bit 0 reflects
    the state of the bus request) and A11200h.

    Valid addresses in this area change depending on the peripherals
    and cartridges being used; the 32X, Sega CD, and games like Super
    Street Fighter II and Phantasy Star 4 use addresses within this range.

 5. The VDP is mirrored at certain locations from C00000h to DFFFFFh. In
    order to explain what addresses are valid, here is a layout of each
    address bit:

    MSB                       LSB
    110n n000 nnnn nnnn 000m mmmm

    '1' - This bit must be 1.
    '0' - This bit must be 0.
    'n' - This bit can have any value.
    'm' - VDP addresses (00-1Fh)

    For example, you could read the status port at D8FF04h or C0A508h,
    but not D00084h or CF00008h. Accessing invalid addresses will
    lock up the machine.

 6. The RAM is 64K in size and is repeatedly mirrored throughout the entire
    range it appears in. Most games only access it at FF0000-FFFFFFh.

*/

/*TODO MOVE THESE INTO OWN MODULES*/

/*
 VDP Quirks (expects to be word written.. will need special handling for below

The lower five bits of the address specify what memory mapped VDP register
 to access:

 The VDP occupies addresses C00000h to C0001Fh.

 C00000h    -   Data port (8=r/w, 16=r/w)
 C00002h    -   Data port (mirror)
 C00004h    -   Control port (8=r/w, 16=r/w)
 C00006h    -   Control port (mirror)
 C00008h    -   HV counter (8/16=r/o)
 C0000Ah    -   HV counter (mirror)
 C0000Ch    -   HV counter (mirror)
 C0000Eh    -   HV counter (mirror)
 C00011h    -   SN76489 PSG (8=w/o)
 C00013h    -   SN76489 PSG (mirror)
 C00015h    -   SN76489 PSG (mirror)
 C00017h    -   SN76489 PSG (mirror) 
 00h : Data port
 02h : Data port
 04h : Control port (1)
 06h : Control port
 08h : HV counter (2)
 0Ah : HV counter
 0Ch : HV counter
 0Eh : HV counter
 11h : SN76489 PSG (3)
 13h : SN76489 PSG
 15h : SN76489 PSG
 17h : SN76489 PSG
 18h : Unused (4)
 1Ah : Unused
 1Ch : Unused
 1Eh : Unused

 1. For reads, the upper six bits of the status flags are set to the
    value of the next instruction to be fetched. Bit 6 is always zero.
    For example:

    move.w $C00004, d0  ; Next word is $4E71
    nop                 ; d0 = -1-- 11?? ?0?? ????

    When reading the status flags through the Z80's banked memory area,
    the upper six bits are set to one.

 2. Writing to the HV counter will cause the machine to lock up.

 3. Reading the PSG addresses will cause the machine to lock up.

    Doing byte-wide writes to even PSG addresses has no effect.

    If you want to write to the PSG via word-wide writes, the data
    must be in the LSB. For instance:

    move.b      (a4)+, d0       ; PSG data in LSB
    move.w      d0, $C00010     ; Write to PSG

 4. Reading the unused addresses returns the next instruction to be fetched.
    For example:

    move.w $C00018, d0  ; Next word is $4E71
    nop                 ; d0 = $4E71

    When reading these addresses through the Z80's banked memory area,
    the value returned is always FFh.

    Writing to C00018h and C0001Ah has no effect.

    Writing to C0001Ch and C0001Eh seem to corrupt the internal state
    of the VDP. Here's what each bit does: (assuming word-wide writes)

    8E9Fh : These bits cause brief flicker in the current 8 pixels
            being drawn when the write occurs.

    5040h : These bits blank the display like bit 6 of register #1 when set.

    2000h : This bit makes every line show the same random garbage data.

    0100h : This bit makes random pattern data appear in the upper eight
            and lower ten lines of the display, with the normal 224 lines
            in the middle untouched. For those of you interested, the
            display is built up like so:

            224 lines for the active display
             10 lines unused (can show pattern data here with above bit)
              3 lines vertical blank (no border color shown)
              3 lines vertical retrace (no picture shown at all)
             13 lines vertical blank (no border color shown)
              8 lines unused (can show pattern data here with above bit)

            I know that comes up to 261 lines and not 262. But that's
            all my monitor shows.

            Turning the display off makes the screen roll vertically,
            and random pattern data is displayed in all lines when
            this bit is set.

    0020h : This bit causes the name table and pattern data shown to
            become corrupt. Not sure if the VRAM is bad or the VDP is
            just showing the wrong data.

 

 Byte-wide writes

 Writing to the VDP control or data ports is interpreted as a 16-bit
 write, with the LSB duplicated in the MSB. This is regardless of writing
 to an even or odd address:

 move.w #$A5, $C00003   ; Same as 'move.w #$A5A5, $C00002'
 move.w #$87, $C00004   ; Same as 'move.w #$8787, $C00004'

 Byte-wide reads

 Reading from even VDP addresses returns the MSB of the 16-bit data,
 and reading from odd address returns the LSB:

 move.b $C00008, d0     ; D0 = V counter
 move.b $C00001, d0     ; D0 = LSB of current VDP data word
 move.b $C0001F, d0     ; D0 = $71
 nop
 move.b $C00004, d0     ; D0 = MSB of status flags
*/

/* Status Register

Reading the control port returns a 16-bit word that allows you to observe
 various states of the VDP and physical display.

 d15 - Always 0
 d14 - Always 0
 d13 - Always 1
 d12 - Always 1
 d11 - Always 0
 d10 - Always 1
 d9  - FIFO Empty
 d8  - FIFO Full
 d7  - Vertical interrupt pending
 d6  - Sprite overflow on current scan line
 d5  - Sprite collision
 d4  - Odd frame
 d3  - Vertical blanking
 d2  - Horizontal blanking
 d1  - DMA in progress
 d0  - PAL mode flag

 Presumably bit 0 is set when the system display is PAL; however this same
 information can be read from the version register (part of the I/O
 register group - not the VDP), so maybe this bit reflects the state of
 having a 240-line display enabled.

 Bit 1 is set for the duration of a DMA operation. This is only useful for
 fills and copies, since the 68000 is frozen during 68k -> VDP transfers.

 Bit 2 returns the real-time status of the horizontal blanking signal.
 It is set at H counter cycle E4h and cleared at H counter cycle 08h.

 Bit 3 returns the real-time status of the vertical blanking signal.
 It is set on line E0h, at H counter cycle AAh, and cleared on line FFh,
 at H counter cycle AAh.

 (Note: For both blanking flag descriptions, the H counter values are
 very likely different for 32-cell and interlaced displays; they were
 taken from a test with a 40-cell screen.)

 Bit 4 is set when the display is interlaced, and in the odd frame,
 otherwise it is cleared in the even frame. This applies to both
 interlace modes.

 Bit 5 is set when any sprites have non-transparent pixels overlapping one
 another. This may hold true for sprites outside of the display area too.
 This bit is most likely cleared at the end of the frame.

 Bit 6 is set when too many sprites are on the current scan line, meaning
 when the VDP parses the 21st sprite in 40-cell mode or the 17th sprite in
 32-cell mode from the sprite list.
 This bit is most likely cleared at the end of the frame.

 Bit 7 is set when a vertical interrupt occurs. This happens at line E0h,
 roughly after H counter cycle 08h. I do not know under what conditions
 it is cleared, presumably at the end of the frame. Reading the control
 port does not clear this bit. (this could be incorrect)

 Bit 8 and 9 (FULL and EMPTY flags, respectively) are related to the FIFO;
 here's what Flavio Morsoletto has to say about their use:

    "The FIFO can hold up to four 16-bit words while the VDP's busy
    parsing data from VRAM. Once the 68K has written the fourth word,
    FULL is raised. If the processor attempts to write one more time, it
    will be frozen (/DTACK high) until the FIFO unit manages to deliver
    the first stacked word to its rightful owner. EMPTY only goes 1 when
    there is nothing on the stack. The intermediate state is signaled by
    both of them showing 0."

 This situation only occurs during the active display period, as the
 data port can be written to as many times as needed during blanking.

 I've noticed most emulators keep the EMPTY flag set, so it appears as
 if the FIFO was always empty instead of being in the neutral state. This
 is probably for games that would normally check and find the FIFO in a
 neutral state, then write data and expect to poll the FULL flag afterwards

*/

U8 VDP_StatusRegisterHI()
{
/*	
	EMU_PRINTF("TODO : Status Read HI\n");
*/
	return 0x36;		/* 0011 01   Fifo Empty set, fifo full clear  (TODO implement FIFO) */
}

U8 VDP_StatusRegisterLO()
{
/*
	EMU_PRINTF("TODO : Status Read LO\n");
*/

	return 0x00 | (Globals::inVBlank<<3) | (Globals::inHBlank<<2);			/* TODO (rest - h and vblank okish) */
}

U32	longWriteMode=0;
U32	doingRegisterMode=0;
U32	VDP_ControlPortLatch=0;						/* TODO - save state problem here */

U32	VDP_latchIdCode;
U32	VDP_latchDstAddress;

U16	VDP_readDataLatch=0;

void VDP_ExecuteDataPortRead()			/* Need to check this implementation at some point */
{
	longWriteMode=0;
	switch (VDP_latchIdCode)
	{
	case 0x00:				/* VRAM Read */

#if VRAM_128KB_MODE
		if (VDP_latchDstAddress & 0x0001)
		{
			VDP_readDataLatch = Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 0];
			VDP_readDataLatch |= Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 1] << 8;
		}
		else
		{
			VDP_readDataLatch = Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 0] << 8;
			VDP_readDataLatch |= Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 1];
		}
#else
		if (VDP_latchDstAddress & 0x0001)
		{
			VDP_readDataLatch = Memory::vRam[(VDP_latchDstAddress & 0xFFFE)+0];
			VDP_readDataLatch|= Memory::vRam[(VDP_latchDstAddress & 0xFFFE)+1]<<8;
		}
		else
		{
			VDP_readDataLatch = Memory::vRam[(VDP_latchDstAddress & 0xFFFE)+0]<<8;
			VDP_readDataLatch|= Memory::vRam[(VDP_latchDstAddress & 0xFFFE)+1];
		}
#endif
		//Auto-increment reg
		VDP_latchDstAddress += VDP::VDP_Registers[0x0F];

		return;

	case 0x08:				/* CRAM Read */

		VDP_readDataLatch = Memory::cRam[(VDP_latchDstAddress & 0x7E)+0]<<8;
		VDP_readDataLatch|= Memory::cRam[(VDP_latchDstAddress & 0x7E)+1];

		//Auto-increment reg
		VDP_latchDstAddress += VDP::VDP_Registers[0x0F];

		return;

	case 0x04:				/* SVRAM Read */

		VDP_readDataLatch = Memory::vsRam[(VDP_latchDstAddress & 0x4E)+0]<<8;
		VDP_readDataLatch|= Memory::vsRam[(VDP_latchDstAddress & 0x4E)+1];

		//Auto-increment reg
		VDP_latchDstAddress += VDP::VDP_Registers[0x0F];

		return;
	}

#if defined _DEBUG
	EMU_PRINTF("Reading Data via unsupported mode : %02X\n",VDP_latchIdCode);
#endif
	VDP_readDataLatch=0xFFFF;
}


U8 VDP_ReadDataPortHI()
{
	VDP_ExecuteDataPortRead();				/* cache value on read hi - return cached value */

	return (VDP_readDataLatch&0xFF00)>>8;
}

U8 VDP_ReadDataPortLO()
{
	return VDP_readDataLatch&0xFF;
}

U8 VDP_ReadByte(U16 offset)			/* see vdp quirks */
{
	switch (offset)
	{
	case 0x00:		/* Data Port */
		return VDP_ReadDataPortHI();
	case 0x01:
		return VDP_ReadDataPortLO();
	case 0x02:		/* Data Port (mirror) */
		return VDP_ReadDataPortHI();
	case 0x03:
		return VDP_ReadDataPortLO();

	case 0x04:		/* Control Port */
		return VDP_StatusRegisterHI();
	case 0x05:
		return VDP_StatusRegisterLO();
	case 0x06:		/* Control Port (mirror) */
		return VDP_StatusRegisterHI();
	case 0x07:
		return VDP_StatusRegisterLO();
	case 0x08:		/* HV Counter */
	case 0x0A:		/* HV Counter (mirror) */
	case 0x0C:		/* HV Counter (mirror) */
	case 0x0E:		/* HV Counter (mirror) */
		if (Globals::lineNo>255)
			return 255;
		return Globals::lineNo;
	case 0x09:
	case 0x0B:
	case 0x0D:
	case 0x0F:
		if (Globals::colNo>255)
			return 255;
		return Globals::colNo;
	case 0x10:		/* PSG */
	case 0x11:
		DEB_PauseEmulation(DebugMode::M68K,"Lock up on real hardware");
		return 0;
	case 0x12:		/* PSG (mirror) */
	case 0x13:
		DEB_PauseEmulation(DebugMode::M68K,"Lock up on real hardware");
		return 0;
	case 0x14:		/* PSG (mirror) */
	case 0x15:
		DEB_PauseEmulation(DebugMode::M68K,"Lock up on real hardware");
		return 0;
	case 0x16:		/* PSG (mirror) */
	case 0x17:
		DEB_PauseEmulation(DebugMode::M68K,"Lock up on real hardware");
		return 0;
	case 0x18:		/* Unused */
	case 0x19:
		DEB_PauseEmulation(DebugMode::M68K,"TODO Return Prefetch");
		return 0xFF;
	case 0x1A:		/* Unused */
	case 0x1B:
		DEB_PauseEmulation(DebugMode::M68K,"TODO Return Prefetch");
		return 0xFF;
	case 0x1C:		/* Unused	(Corruption on write) */
	case 0x1D:
		DEB_PauseEmulation(DebugMode::M68K,"TODO Return Prefetch");
		return 0xFF;
	case 0x1E:		/* Unused (Corruption on write) */
	case 0x1F:	
		DEB_PauseEmulation(DebugMode::M68K,"TODO Return Prefetch");
		return 0xFF;
	}

	DEB_PauseEmulation(DebugMode::M68K,"DONE SOMETHING WRONG.. VDP READ OUT OF BOUNDS");
	return 0xFF;
}

void VDP_WriteControlPortHI(U8 byte)
{
/*     1   0   ?  R04 R03 R02 R01 R00     (D15-D8)
    D07 D06 D05 D04 D03 D02 D01 D00     (D7-D0)

    Rxx = VDP register select (00-1F)
    Dxx = VDP register data (00-FF)	*/
	
	if (longWriteMode)
	{
		/* second half of register write (hopefully!) */
		longWriteMode=0;
		/* need to set flag to shift bytes correctly into latch*/
		if (byte!=0)
		{
			DEB_PauseEmulation(DebugMode::M68K,"VDP Hi Port Write - second half of transfer should have been 00!");
		}
		VDP_ControlPortLatch&=0xFFFF00FF;
	}
	else
	{
		if ((byte&0xC0) == 0x80)
		{
			doingRegisterMode=1;
			longWriteMode=0;
			VDP_latchIdCode=0;
			/* Good we are trying to do a register thing */
			VDP_ControlPortLatch&=0x00FF;
			VDP_ControlPortLatch|=(byte & 0x1F)<<8;
		}
		else
		{
			doingRegisterMode=0;
			longWriteMode=1;
			VDP_ControlPortLatch&=0x00FFFFFF;
			VDP_ControlPortLatch|=(byte)<<24;
		}
	}
}

void VDP_DataModeStart()
{
/*
	CD1 CD0 A13 A12 A11 A10 A09 A08     (D31-D24)
    A07 A06 A05 A04 A03 A02 A01 A00     (D23-D16)
     ?   ?   ?   ?   ?   ?   ?   ?      (D15-D8)
    CD5 CD4 CD3 CD2  ?   ?  A15 A14     (D7-D0)
*/
	U32 IdCode;
	U32 dstAddress;

	IdCode = VDP_ControlPortLatch&0xC00000F0;
	IdCode = ( (IdCode&0xF0) | ( (IdCode&0xC0000000)>>28) )>>2;

#if VRAM_128KB_MODE
	dstAddress = VDP_ControlPortLatch & 0x3FFF0007;
	dstAddress = ((dstAddress & 0x7) << 14) | ((dstAddress & 0x3FFF0000) >> 16);
#else
	dstAddress = VDP_ControlPortLatch & 0x3FFF0003;
	dstAddress = ((dstAddress & 0x3) << 14) | ((dstAddress & 0x3FFF0000) >> 16);
#endif

	VDP_latchIdCode=IdCode;
	VDP_latchDstAddress=dstAddress;

/*
	EMU_PRINTF("VDP Reg : %08X\n",VDP_ControlPortLatch);
	EMU_PRINTF("Attempting Data Mode : %02X\n",IdCode);
	EMU_PRINTF("Address : %08X\n",dstAddress);
*/
	if (IdCode&0x20)
	{
		/* DMA Transfer.. stage 1 do in place (later need to make it cost time) */

		if (VDP::VDP_Registers[23]&0x80)
		{
			if (VDP::VDP_Registers[23]&0x40)		/* Copy mode */
			{
				#if defined _DEBUG
				EMU_PRINTF("DMA request VRAM COPY.. ignoring for now!\n");
				#endif
				return;
			}
			else
			{
				if ((IdCode&0x1F)!=1)	/* FILL mode...  */
				{
					//DEB_PauseEmulation(DebugMode::M68K,"unsupported setting in fill mode.. probably stuffed something up");
					return;
				}
				else
				{
					/* Need to wait for write to DATA port before performing fill */
					return;
				}
			}
		}

		if (VDP::VDP_Registers[1]&0x10)		/* ensure dma actually enabled */
		{
			U32 srcAddress = ((VDP::VDP_Registers[23]&0x7F) << 16) | ((VDP::VDP_Registers[22]<<8)) | VDP::VDP_Registers[21];
			U16 length = (VDP::VDP_Registers[20]<<8) | VDP::VDP_Registers[19];

			srcAddress<<=1;

			switch (IdCode & 0x7)
			{
				case 1:	/* VRAM */
				{
	#if VDP_DMA_MEMCPY
					if ((VDP::VDP_Registers[0x0F] == 2) && !(dstAddress & 0x0001))
					{
						unsigned char* src = MEM_getDMAAddr(srcAddress);
#if VRAM_128KB_MODE
						unsigned char* dst = &Memory::vRam[dstAddress & 0x1FFFE];
#else
						unsigned char* dst = &Memory::vRam[dstAddress & 0xFFFE];
#endif
						memcpy(dst, src, length * 2);
					}
					else
	#endif
					{
						while (length)
						{
							U16 dataWord = MEM_getWord(srcAddress);

							srcAddress += 2;

#if VRAM_128KB_MODE
							if (dstAddress & 0x0001)
							{
								Memory::vRam[(dstAddress & 0x1FFFE) + 0] = dataWord & 0xFF;
								Memory::vRam[(dstAddress & 0x1FFFE) + 1] = (dataWord & 0xFF00) >> 8;
							}
							else
							{
								Memory::vRam[(dstAddress & 0x1FFFE) + 0] = (dataWord & 0xFF00) >> 8;
								Memory::vRam[(dstAddress & 0x1FFFE) + 1] = dataWord & 0xFF;
							}
#else
							if (dstAddress & 0x0001)
							{
								Memory::vRam[(dstAddress & 0xFFFE) + 0] = dataWord & 0xFF;
								Memory::vRam[(dstAddress & 0xFFFE) + 1] = (dataWord & 0xFF00) >> 8;
							}
							else
							{
								Memory::vRam[(dstAddress & 0xFFFE) + 0] = (dataWord & 0xFF00) >> 8;
								Memory::vRam[(dstAddress & 0xFFFE) + 1] = dataWord & 0xFF;
							}
#endif

							dstAddress += VDP::VDP_Registers[0x0F];

							--length;
						}
					}

					return;
				}
				case 3:	/* CRAM */
				{
	#if VDP_DMA_MEMCPY
					if (VDP::VDP_Registers[0x0F] == 2)
					{
						unsigned char* src = MEM_getDMAAddr(srcAddress);
						unsigned char* dst = &Memory::cRam[dstAddress & 0x7E];
						memcpy(dst, src, length * 2);
					}
					else
	#endif
					{
						while (length)
						{
							U16 dataWord = MEM_getWord(srcAddress);

							srcAddress += 2;

							Memory::cRam[(dstAddress & 0x7E) + 0] = (dataWord & 0xFF00) >> 8;
							Memory::cRam[(dstAddress & 0x7E) + 1] = dataWord & 0xFF;

							dstAddress += VDP::VDP_Registers[0x0F];

							length--;
						}
					}

					VDP::cramCacheDirty = 1;

					return;
				}
			}

			#if defined _DEBUG
			EMU_PRINTF("DMA Transfer (unhandled)- ignoring for now!\n");
			if (IdCode&0x10)
			{
				EMU_PRINTF("VRAM dma copy\n");
			}
			#endif
			/* decodes slightly differently */
		}
		return;
	}
/*	switch(IdCode&0x0F)
	{
	case 0x0:
		EMU_PRINTF("VRAM READ\n");
		break;
	case 0x1:
		EMU_PRINTF("VRAM WRITE\n");
		break;
	case 0x3:
		EMU_PRINTF("CRAM WRITE\n");
		break;
	case 0x4:
		EMU_PRINTF("VSRAM READ\n");
		break;
	case 0x5:
		EMU_PRINTF("VSRAM WRITE\n");
		break;
	case 0x8:
		EMU_PRINTF("CRAM READ\n");
		break;
	default:
		EMU_PRINTF("Unknown idCode value\n");
	}
*/
}

void VDP_WriteControlPortLO(U8 byte)
{
	if (doingRegisterMode)
	{
		VDP_ControlPortLatch&=0xFF00;
		VDP_ControlPortLatch|=byte;
		VDP::VDP_Registers[VDP_ControlPortLatch>>8]=byte;
	}
	else
	{
		if (longWriteMode)
		{
			VDP_ControlPortLatch&=0xFF00FFFF;
			VDP_ControlPortLatch|=(byte)<<16;
		}
		else
		{
			/* Final write - lets see what was requested */
			VDP_ControlPortLatch&=0xFFFFFF00;
			VDP_ControlPortLatch|=(byte);

			VDP_DataModeStart();
		}
	}
}

U16	writeDataLatch=0;

void VDP_WriteDataPortHI(U8 byte)			/* This will be bugged until cpu allows me to see difference between byte/word/long access */
{
	longWriteMode=0;
	writeDataLatch&=0x00FF;
	writeDataLatch|=(byte<<8);
}

void VDP_ExecuteDataPortWrite()
{
	longWriteMode=0;
	switch (VDP_latchIdCode)
	{
	case 0x01:

#if VRAM_128KB_MODE
		if (VDP_latchDstAddress & 0x0001)
		{
			Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 0] = writeDataLatch & 0xFF;
			Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 1] = (writeDataLatch & 0xFF00) >> 8;
		}
		else
		{
			Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 0] = (writeDataLatch & 0xFF00) >> 8;
			Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 1] = writeDataLatch & 0xFF;
		}
#else
		if (VDP_latchDstAddress & 0x0001)
		{
			Memory::vRam[(VDP_latchDstAddress & 0xFFFE)+0]=writeDataLatch&0xFF;
			Memory::vRam[(VDP_latchDstAddress & 0xFFFE)+1]=(writeDataLatch&0xFF00)>>8;
		}
		else
		{
			Memory::vRam[(VDP_latchDstAddress & 0xFFFE)+0]=(writeDataLatch&0xFF00)>>8;
			Memory::vRam[(VDP_latchDstAddress & 0xFFFE)+1]=writeDataLatch&0xFF;
		}
#endif

		VDP_latchDstAddress += VDP::VDP_Registers[0x0F];

		return;

	case 0x03:				/* CRAM Write */

		Memory::cRam[(VDP_latchDstAddress & 0x7E)+0]=(writeDataLatch&0xFF00)>>8;
		Memory::cRam[(VDP_latchDstAddress & 0x7E)+1]=writeDataLatch&0xFF;

		VDP_latchDstAddress += VDP::VDP_Registers[0x0F];

		VDP::cramCacheDirty = 1;

		return;

	case 0x05:				/* SVRAM write */

		Memory::vsRam[(VDP_latchDstAddress & 0x4E)+0]=(writeDataLatch&0xFF00)>>8;
		Memory::vsRam[(VDP_latchDstAddress & 0x4E)+1]=writeDataLatch&0xFF;

		VDP_latchDstAddress += VDP::VDP_Registers[0x0F];

		return;
	}

	#if defined _DEBUG
	EMU_PRINTF("Writing Data via unsupported mode : %02X,%04X\n",VDP_latchIdCode,writeDataLatch);
	#endif
}

void VDP_ExecuteFill()
{
	U16 length = (VDP::VDP_Registers[20]<<8) | VDP::VDP_Registers[19];

#if VRAM_128KB_MODE
	// Fill lo byte into address
	Memory::vRam[VDP_latchDstAddress & 0x1FFFF] = writeDataLatch & 0xFF;

	do
	{
		VDP_latchDstAddress += VDP::VDP_Registers[0x0F];
		Memory::vRam[VDP_latchDstAddress & 0x1FFFF] = (writeDataLatch >> 8) & 0xFF;
	} while (--length); // length is in bytes
#else
	// Fill lo byte into address
	Memory::vRam[VDP_latchDstAddress & 0xFFFF] = writeDataLatch & 0xFF;

	do
	{
		VDP_latchDstAddress += VDP::VDP_Registers[0x0F];
		Memory::vRam[VDP_latchDstAddress & 0xFFFF] = (writeDataLatch >> 8) & 0xFF;
	} while (--length); // length is in bytes
#endif
}

void VDP_WriteDataPortLO(U8 byte)
{
	writeDataLatch&=0xFF00;
	writeDataLatch|=byte;

	if (VDP_latchIdCode==0x21)
	{
		VDP_ExecuteFill();			/* may not be catching all cases here!? */
	}
	else
	{
		VDP_ExecuteDataPortWrite();
	}
}


void VDP_WriteByte(U16 offset,U8 byte)			/* see vdp quirks */
{
	switch (offset)
	{
	case 0x00:		/* Data Port */
		VDP_WriteDataPortHI(byte);
		break;
	case 0x01:
		VDP_WriteDataPortLO(byte);
		break;
	case 0x02:		/* Data Port (mirror) */
		VDP_WriteDataPortHI(byte);
		break;
	case 0x03:
		VDP_WriteDataPortLO(byte);
		break;

	case 0x04:		/* Control Port */
		VDP_WriteControlPortHI(byte);
		break;	
	case 0x05:
		VDP_WriteControlPortLO(byte);
		break;	
	case 0x06:		/* Control Port					(mirror) */
		VDP_WriteControlPortHI(byte);
		break;	
	case 0x07:
		VDP_WriteControlPortLO(byte);
		break;	

	case 0x08:		/* HV Counter */
	case 0x09:
	case 0x0A:		/* HV Counter (mirror) */
	case 0x0B:
	case 0x0C:		/* HV Counter (mirror) */
	case 0x0D:
	case 0x0E:		/* HV Counter (mirror) */
	case 0x0F:
		DEB_PauseEmulation(DebugMode::M68K,"Lock up on real hardware");
		break;
	case 0x10:		/* PSG */
	case 0x11:
	case 0x12:		/* PSG (mirror) */
	case 0x13:
	case 0x14:		/* PSG (mirror) */
	case 0x15:
	case 0x16:		/* PSG (mirror) */
	case 0x17:
		PSG_Write(byte);
		break;
	case 0x18:		/* Unused (write ignored) */
	case 0x19:
	case 0x1A:		/* Unused (write ignored) */
	case 0x1B:
		break;
	case 0x1C:		/* Unused	(Corruption on write) */
	case 0x1D:
	case 0x1E:		/* Unused (Corruption on write) */
	case 0x1F:	
		DEB_PauseEmulation(DebugMode::M68K,"Corruption");	
		break;
	}
}




/*-----------*/


void MEM_SaveState(FILE *outStream)
{
	fwrite(Memory::systemRam,1,SYS_RAM_SIZE,outStream);
}

void MEM_LoadState(FILE *inStream)
{
	if (SYS_RAM_SIZE!=fread(Memory::systemRam,1,SYS_RAM_SIZE,inStream))
	{
		return;
	}
}

#if ENABLE_32X_MODE
extern U8* cpu68kbios;
extern U32 cpu68kbiosSize;
#endif

#define SWAP_WORD(a) ((a&0xFF00) >> 8) | ((a&0x00FF) << 8)

U8 MEM_getByte_CartRom(U32 address, U32 upper24,U32 lower16)
{
	return Memory::cartRom[address];
}

U16 MEM_getWord_CartRom(U32 address)
{
#ifdef DEBUG
	if (address >= cartRomSize)
	{
		EMU_PRINTF("Cart ROM address out of bounds (addr: 0x%08X size: 0x%08X)", address, cartRomSize);
		DEB_PauseEmulation(DebugMode::M68K, "Cart ROM address out of bounds");
	}
#endif

	U16 word = *(U16*)&Memory::cartRom[address];
	return SWAP_WORD(word);
};

unsigned char* MEM_getDMA_CartRom(U32 address)
{
	return &Memory::cartRom[address];
}

U8 MEM_getByte_SystemRam(U32 address, U32 upper24,U32 lower16)
{
	UNUSED_ARGUMENT(upper24);
	return Memory::systemRam[lower16];
}

U16 MEM_getWord_SystemRam(U32 address)
{
	U16 word = *(U16*)&Memory::systemRam[address & 0x0000FFFF];
	return SWAP_WORD(word);
}

unsigned char* MEM_getDMA_SystemRam(U32 address)
{
	return &Memory::systemRam[address & 0x0000FFFF];
}

U8 MEM_getByte_SRAM(U32 address, U32 upper24,U32 lower16)
{
	U8 byte = 0;

	//TODO: Rearrange memory map instead of using a global redirect flag
	if (Globals::SRAM_Lock)
	{
		if ((address >= Globals::SRAM_StartAddress) && (address <= Globals::SRAM_EndAddress))
		{
			if ((Globals::SRAM_OddAccess && (lower16 & 0x1)) || (Globals::SRAM_EvenAccess && ((lower16 & 0x1) == 0)))
			{
				if (Globals::SRAM_EvenAccess ^ Globals::SRAM_OddAccess)
				{
					byte = Globals::SRAM[(address - Globals::SRAM_StartAddress) >> 1];
				}
				else
				{
					byte = Globals::SRAM[address - Globals::SRAM_StartAddress];
				}
			}
		}

		return byte;
	}
	else
	{
		return MEM_getByte_CartRom(address, upper24, lower16);
	}

	#if defined _DEBUG
	EMU_PRINTF("Byteread from unmapped SRAM address 0x%08X\n", (int)upper24 << 16 | lower16);
	#endif
	//DEB_PauseEmulation(DebugMode::M68K,"Unmapped Read - May need to crash/return prefetch queue/implement missing functionality");
	return byte;
}

void MEM_setByte_SRAM(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	//TODO: Rearrange memory map instead of using a global redirect flag
	if (Globals::SRAM_Lock)
	{
		if ((address >= Globals::SRAM_StartAddress) && (address <= Globals::SRAM_EndAddress))
		{
			if ((Globals::SRAM_OddAccess && (lower16 & 0x1)) || (Globals::SRAM_EvenAccess && ((lower16 & 0x1) == 0)))
			{
				if (Globals::SRAM_EvenAccess ^ Globals::SRAM_OddAccess)
				{
					Globals::SRAM[(address - Globals::SRAM_StartAddress) >> 1] = byte;
				}
				else
				{
					Globals::SRAM[address - Globals::SRAM_StartAddress] = byte;
				}
				return;
			}
		}
	}

	EMU_PRINTF("Byte 0x%02X written to unmapped SRAM address 0x%08X\n", (int)byte, (int)upper24 << 16 | lower16);
	//DEB_PauseEmulation(DebugMode::M68K,"Unmapped Write - May need to crash/return prefetch queue/implement missing functionality");
}

U8 MEM_getByte_Unmapped(U32 address, U32 upper24,U32 lower16)
{
	UNUSED_ARGUMENT(upper24);
	UNUSED_ARGUMENT(lower16);
	EMU_PRINTF("Byte read from unmapped address 0x%08X\n", (int)upper24 << 16 | lower16);
	//DEB_PauseEmulation(DebugMode::M68K,"Unamapped Read - May need to crash/return prefetch queue/implement missing functionality");
	return 0;
}

unsigned char* MEM_getDMA_Unmapped(U32 address)
{
	EMU_PRINTF("Bad DMA source address: 0x%08X\n", address);
	return NULL;
}

U16 MEM_getWord_Split(U32 address)
{
	return (MEM_getByte(address) << 8) | MEM_getByte(address + 1);
}

void MEM_setWord_Split(U32 address, U16 word)
{
	MEM_setByte(address, word >> 8);
	MEM_setByte(address + 1, word & 0xFF);
}

/*TODO MOVE z80?*/
void Z80_MEM_setByte(U16 address,U8 data);
U8 Z80_MEM_getByte(U16 address);

/*might have probs with recursive memory access 68K->z80[68000]->z80  etc.*/
U8 MEM_getByte_Z80(U32 address, U32 upper24,U32 lower16)
{
	UNUSED_ARGUMENT(upper24);
	return Z80_MEM_getByte(lower16);
}

void MEM_setByte_Z80(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	UNUSED_ARGUMENT(upper24);
	Z80_MEM_setByte(lower16,byte);
}


/*----------*/

U8 ioRegisters[0x20];
S8 io6ButtonLatchHi[3] = { IO_6BUTTON_LATCH_WRITE_COUNT, IO_6BUTTON_LATCH_WRITE_COUNT, IO_6BUTTON_LATCH_WRITE_COUNT };
S8 io6ButtonLatchLo[3] = { IO_6BUTTON_LATCH_WRITE_COUNT, IO_6BUTTON_LATCH_WRITE_COUNT, IO_6BUTTON_LATCH_WRITE_COUNT };
S32 io6buttonLatchTimer[3] = { 0 };

const char *IO_DumpRegisterName(U16 byte)
{
	switch(byte)
	{
	case 0x01:
		return "Version Register      ";
	case 0x03:
		return "Data Register Port A  ";
	case 0x05:
		return "Data Register Port B  ";
	case 0x07:
		return "Data Register Port C  ";
	case 0x09:
		return "Ctrl Register Port A  ";
	case 0x0B:
		return "Ctrl Register Port B  ";
	case 0x0D:
		return "Ctrl Register Port C  ";
	case 0x0F:
		return "TxData Register Port A";
	case 0x11:
		return "RxData Register Port A";
	case 0x13:
		return "S-Ctrl Register Port A";
	case 0x15:
		return "TxData Register Port B";
	case 0x17:
		return "RxData Register Port B";
	case 0x19:
		return "S-Ctrl Register Port B";
	case 0x1B:
		return "TxData Register Port C";
	case 0x1D:
		return "RxData Register Port C";
	case 0x1F:
		return "S-Ctrl Register Port C";
	}

	return "";
}

void IO_GetRegisterContents(U16 offset,char *buffer)
{
	offset&=0x1E;
	offset+=1;
	sprintf(buffer,"%s : %02X",IO_DumpRegisterName(offset),ioRegisters[offset]);
}

void IO_UpdateControllerLatch(U32 cycles)
{
	for (int i = 0; i < 3; i++)
	{
		if (io6buttonLatchTimer[i] > 0)
		{
			io6buttonLatchTimer[i] -= cycles;
			if (io6buttonLatchTimer[i] <= 0)
			{
				io6ButtonLatchHi[i] = IO_6BUTTON_LATCH_WRITE_COUNT;
				io6ButtonLatchLo[i] = IO_6BUTTON_LATCH_WRITE_COUNT;
				io6buttonLatchTimer[i] = 0;
			}
		}
	}
}

#if ENABLE_32X_MODE

extern U8 CRAM[0x200];
extern U8 FRAMEBUFFER[0x00040000];
extern U8 SDRAM[0x00040000];
extern U8 SH2_68K_COMM_REGISTER[16];

extern U16 SH2_PWMControlRegister;
extern U16 SH2_PWMCycleRegister;
extern U16 SH2_PWMLchPulseWidthRegister;
extern U16 SH2_PWMRchPulseWidthRegister;
extern U16 SH2_PWMMonoPulseWidthRegister;

U16 AdapterControlRegister=0x0080;
U16 InterruptControlRegister=0x0000;
U16 BankSetRegister=0x0000;
U16 DREQControlRegister=0x0000;
U16 DREQSourceAddressHi=0x0000;
U16 DREQSourceAddressLo=0x0000;
U16 DREQDestinationAddressHi=0x0000;
U16 DREQDestinationAddressLo=0x0000;
U16 DREQLengthRegister=0x0000;
U16 FIFORegister=0x0000;
U16 SegaTVRegister=0x0000;

void SwapTo32XModeMemoryMap();
void SwapToStandardMemoryMap();

#include "sh2.h"
extern SH2_State* master;
extern SH2_State* slave;

U16 FIFO_BUFFER[8];

U8 curFifoWrite=0;

void FIFO_ADD(U16 data)
{
	if (curFifoWrite==8)
	{
		return;
	}
	FIFO_BUFFER[curFifoWrite]=data;
	curFifoWrite++;

	SH2_SignalDAck(master);
	SH2_SignalDAck(slave);

	if (curFifoWrite==8)
	{
		DREQControlRegister|=0x80;				/* Set FIFO full now */
		return;
	}
}

U16 FIFO_GET()
{
	U16 retVal=0;

	if (curFifoWrite>0)
	{
		retVal=FIFO_BUFFER[0];
		memmove(&FIFO_BUFFER[0],&FIFO_BUFFER[1],2*7);
		curFifoWrite--;
		DREQControlRegister&=~0x80;				/* Set FIFO free now */
	}

	return retVal;
}

U8 MD_SYS_32X_READ(U16 adr)
{
	if (adr>=0x20 && adr<=0x2F)
	{
		return SH2_68K_COMM_REGISTER[adr-0x20]; 
	}

	switch (adr)
	{
	case 0x00:
		return AdapterControlRegister>>8;
	case 0x01:
		return AdapterControlRegister&0xFF;
	case 0x02:
		return InterruptControlRegister>>8;
	case 0x03:
		return InterruptControlRegister&0xFF;
	case 0x04:
		return BankSetRegister>>8;
	case 0x05:
		return BankSetRegister&0xFF;
	case 0x06:
		return DREQControlRegister>>8;
	case 0x07:
		return DREQControlRegister&0xFF;
	case 0x08:
		return DREQSourceAddressHi>>8;
	case 0x09:
		return DREQSourceAddressHi&0xFF;
	case 0x0A:
		return DREQSourceAddressLo>>8;
	case 0x0B:
		return DREQSourceAddressLo&0xFF;
	case 0x0C:
		return DREQDestinationAddressHi>>8;
	case 0x0D:
		return DREQDestinationAddressHi&0xFF;
	case 0x0E:
		return DREQDestinationAddressLo>>8;
	case 0x0F:
		return DREQDestinationAddressLo&0xFF;
	case 0x10:
		return DREQLengthRegister>>8;
	case 0x11:
		return DREQLengthRegister&0xFF;
	case 0x12:
		return 0xFF;		/* Write only	*/
	case 0x13:
		return 0xFF;		/* Write only	*/
	case 0x1A:
		return SegaTVRegister>>8;
	case 0x1B:
		return SegaTVRegister&0xFF;
	case 0x30:
		return SH2_PWMControlRegister>>8;
	case 0x31:
		return SH2_PWMControlRegister&0xFF;
	case 0x32:
		return SH2_PWMCycleRegister>>8;
	case 0x33:
		return SH2_PWMCycleRegister&0xFF;
	case 0x34:
		return (SH2_PWMLchPulseWidthRegister>>8)&0xC0;
	case 0x35:
		return (SH2_PWMLchPulseWidthRegister&0xFF)&0x00;
	case 0x36:
		return (SH2_PWMRchPulseWidthRegister>>8)&0xC0;
	case 0x37:
		return (SH2_PWMRchPulseWidthRegister&0xFF)&0x00;
	case 0x38:
		return (SH2_PWMMonoPulseWidthRegister>>8)&0xC0;
	case 0x39:
		return (SH2_PWMMonoPulseWidthRegister&0xFF)&0x00;
	}

	DEB_PauseEmulation(DebugMode::M68K,"Unhandled Read 32X SYS Register");
	return 0xFF;
}

extern U8 SH2_Master_AdapterControlRegister;
extern U8 SH2_Slave_AdapterControlRegister;

void MD_SYS_32X_WRITE(U16 adr,U8 byte)
{
	if (adr>=0x20 && adr<=0x2F)
	{
		SH2_68K_COMM_REGISTER[adr-0x20]=byte;
		return;
	}

	switch (adr)
	{
	case 0x00:
		AdapterControlRegister&=0x00FF;
		AdapterControlRegister|=byte<<8;
		return;
	case 0x01:
		if (((AdapterControlRegister&0x0002)==0) && ((byte&0x02)==2))
		{
			SH2_Reset(master);
			SH2_Reset(slave);
		}

		AdapterControlRegister&=0xFF00;
		AdapterControlRegister|=byte;

		if ((AdapterControlRegister&0x0001))
		{
			SwapTo32XModeMemoryMap();
		}
		else
		{
			SwapToStandardMemoryMap();
		}
		return;
	case 0x02:
		InterruptControlRegister&=0x00FF;
		InterruptControlRegister|=byte<<8;
		return;
	case 0x03:
		InterruptControlRegister&=0xFF00;
/*		InterruptControlRegister|=byte&0xFF;	*/
		if (byte&0x01)
		{
			if (SH2_Master_AdapterControlRegister&0x02)
			{
				SH2_Interrupt(master,9);
			}
		}
		if (byte&0x02)
		{
			if (SH2_Slave_AdapterControlRegister&0x02)
			{
				SH2_Interrupt(slave,9);
			}
		}
		return;
	case 0x04:
		BankSetRegister&=0x00FF;
		BankSetRegister|=byte<<8;
		return;
	case 0x05:
		BankSetRegister&=0xFF00;
		BankSetRegister|=byte&0xFF;
		return;
	case 0x06:
		DREQControlRegister&=0x00FF;
		DREQControlRegister|=byte<<8;
		return;
	case 0x07:
		DREQControlRegister&=0xFF80;
		DREQControlRegister|=byte&0x7F;
		return;
	case 0x08:
		return;
	case 0x09:
		DREQSourceAddressHi&=0xFF00;
		DREQSourceAddressHi|=byte&0xFF;
		return;
	case 0x0A:
		DREQSourceAddressLo&=0x00FF;
		DREQSourceAddressLo|=byte<<8;
		return;
	case 0x0B:
		DREQSourceAddressLo&=0xFF01;
		DREQSourceAddressLo|=byte&0xFE;
		return;
	case 0x0C:
		return;
	case 0x0D:
		DREQDestinationAddressHi&=0xFF00;
		DREQDestinationAddressHi|=byte&0xFF;
		return;
	case 0x0E:
		DREQDestinationAddressLo&=0x00FF;
		DREQDestinationAddressLo|=byte<<8;
		return;
	case 0x0F:
		DREQDestinationAddressLo&=0xFF00;
		DREQDestinationAddressLo|=byte&0xFF;
		return;
	case 0x10:
		DREQLengthRegister&=0x00FF;
		DREQLengthRegister|=byte<<8;
		return;
	case 0x11:
		DREQLengthRegister&=0xFF03;
		DREQLengthRegister|=byte&0xFC;
		return;
	case 0x12:
		FIFORegister&=0x00FF;
		FIFORegister|=byte<<8;
		return;
	case 0x13:
		FIFORegister&=0xFF00;
		FIFORegister|=byte&0xFF;
		FIFO_ADD(FIFORegister);
		return;
	case 0x1A:
		SegaTVRegister&=0x00FF;
		SegaTVRegister|=byte<<8;
		return;
	case 0x1B:
		SegaTVRegister&=0xFF00;
		SegaTVRegister|=byte&0xFF;
		return;
	case 0x30:
		return;				/* Read only */
	case 0x31:
		SH2_PWMControlRegister&=0xFF80;
		SH2_PWMControlRegister|=byte&0x7F;
		return;
	case 0x32:
		SH2_PWMCycleRegister&=0x00FF;
		SH2_PWMCycleRegister|=byte<<8;
		return;
	case 0x33:
		SH2_PWMCycleRegister&=0xFF00;
		SH2_PWMCycleRegister|=byte&0xFF;
		return;
	case 0x34:
		SH2_PWMLchPulseWidthRegister&=0xC0FF;
		SH2_PWMLchPulseWidthRegister|=(byte&0x3F)<<8;
		return;
	case 0x35:
		SH2_PWMLchPulseWidthRegister&=0xFF00;
		SH2_PWMLchPulseWidthRegister|=byte&0xFF;
		return;
	case 0x36:
		SH2_PWMRchPulseWidthRegister&=0xC0FF;
		SH2_PWMRchPulseWidthRegister|=(byte&0x3F)<<8;
		return;
	case 0x37:
		SH2_PWMRchPulseWidthRegister&=0xFF00;
		SH2_PWMRchPulseWidthRegister|=byte&0xFF;
		return;
	case 0x38:
		SH2_PWMMonoPulseWidthRegister&=0xC0FF;
		SH2_PWMMonoPulseWidthRegister|=(byte&0x3F)<<8;
		return;
	case 0x39:
		SH2_PWMMonoPulseWidthRegister&=0xFF00;
		SH2_PWMMonoPulseWidthRegister|=byte&0xFF;
		return;
	}

	DEB_PauseEmulation(DebugMode::M68K,"Unhandled Write 32X SYS Register");
}

U8 VDP_32X_Read(U16 adr,int accessor);
void VDP_32X_Write(U16 adr,U8 byte,int accessor);
#endif

void MEM_processRead_IO(int gamePadIdx, U8 ctrlPort, U8 dataPort)
{
	if (gamePadIdx >= 0 && gamePadIdx < EMU_MAX_GAMEPADS)
	{
		if (ioRegisters[dataPort] & 0x40)
		{
			ioRegisters[dataPort] &= (ioRegisters[ctrlPort] & 0x7F) | 0x80;
#if EMU_USE_INPUT_CALLBACKS
			//Latched, in 6-button state = ?1CBMXYZ
			//Latched, in 3-button state = ?1CBRLDU
			U8 padBits = ~(Globals::getGamepadState(gamePadIdx, io6ButtonLatchHi[gamePadIdx] == IO_6BUTTON_LATCH_HI_STATE_1) >> 8);

			ioRegisters[dataPort] |= padBits & (~((ioRegisters[ctrlPort] & 0x7F) | 0x80));
#else
			ioRegisters[dataPort] |= (~(Globals::gamepadStates[gamePadIdx] >> 8))& (~((ioRegisters[ctrlPort] & 0x7F) | 0x80));
#endif
		}
		else
		{
			ioRegisters[dataPort] &= (ioRegisters[ctrlPort] & 0x7F) | 0x80;
#if EMU_USE_INPUT_CALLBACKS
			U8 padBits = 0;

			if (io6ButtonLatchLo[gamePadIdx] == IO_6BUTTON_LATCH_LO_STATE_1)
			{
				//Unlatched, in 6-button state 1 = ?0SA0000
				padBits = ~Globals::getGamepadState(gamePadIdx, true) & 0xF0;
			}
			else if (io6ButtonLatchLo[gamePadIdx] == IO_6BUTTON_LATCH_LO_STATE_2)
			{
				//Unlatched, in 6-button state 2 = ?0SA1111
				padBits = ~Globals::getGamepadState(gamePadIdx, true) | 0x0F;
			}
			else
			{
				//Unlatched, in 3-button state = ?0SA00DU
				padBits = ~Globals::getGamepadState(gamePadIdx, false) & 0xF3;
			}

			ioRegisters[dataPort] |= padBits & (~((ioRegisters[ctrlPort] & 0x7F) | 0x80));
#else
			ioRegisters[dataPort] |= (~(Globals::gamepadStates[gamePadIdx] | 0xC)) & (~((ioRegisters[ctrlPort] & 0x7F) | 0x80));
#endif
		}
	}
}

U8 MEM_getByte_IO(U32 address,U32 upper24,U32 lower16)
{
	UNUSED_ARGUMENT(upper24);
#if ENABLE_32X_MODE
	
	if ((lower16&0xFFEF)==0x30EC)
		return 'M';
	if ((lower16&0xFFEF)==0x30ED)
		return 'A';
	if ((lower16&0xFFEF)==0x30EE)
		return 'R';
	if ((lower16&0xFFEF)==0x30EF)
		return 'S';

	if (lower16>=0x5100 && lower16<0x513A)
		return MD_SYS_32X_READ(lower16-0x5100);

	if (lower16>=0x5180 && lower16<=0x51FF)
	{
		if (AdapterControlRegister&0x8000)
			return 0xFF;
		return VDP_32X_Read(lower16-0x5180,DebugMode::M68K);
	}

	if (lower16>=0x5200 && lower16<0x5400)
	{
		if (AdapterControlRegister&0x8000)
			return 0xFF;
		return CRAM[lower16-0x5200];
	}



#endif
	if (lower16 == 0x1100)
	{
/*		EMU_PRINTF("BUS REQUEST FOR Z80 (returning free!)\n");		TODO FIX ME - z80 bus might not always be available!!*/
#if 1
		if (!Z80::Z80_regs.busTaken)
			return 0x01;
#endif
		return 0x00;
	}

	if (lower16 == 0x1101 /*|| lower16 == 0x1201*/)		/* ignored*/
		return 0x00;		/* a lot of roms attempt to CLR.W the bus request - which causes a read / modify / write */
/*
	if (lower16 == 0x1200)
	{
		EMU_PRINTF("Z80 RESET : %02x\n",byte);
		return;
	}*/

	//EMU_PRINTF("IO read: 0x%08X\n", (int)upper24 << 16 | lower16);

	// IO read : 0x00A10008
	// IO read : 0x00A10009
	// IO read : 0x00A1000A
	// IO read : 0x00A1000B
	// IO read : 0x00A1000C
	// IO read : 0x00A1000D
	// IO read : 0x00A10001
	// IO read : 0x00A10003
	// IO read : 0x00A10003
	// IO read : 0x00A10003
	// IO read : 0x00A10003
	// IO read : 0x00A10005

	if (lower16<0x20)
	{
		if (lower16==3)
		{
			MEM_processRead_IO(0, 0x9, 0x3);
		}
		else if (lower16 == 5)
		{
			MEM_processRead_IO(1, 0xB, 0x5);
		}
		else if (lower16 == 7)
		{
			MEM_processRead_IO(2, 0xD, 0x7);
		}

		return ioRegisters[((lower16&0x1E)+1)];
	}

	#if defined _DEBUG
	EMU_PRINTF("Byte read from unmapped address 0x%08X\n", (int)upper24 << 16 | lower16);
	#endif
	//DEB_PauseEmulation(DebugMode::M68K,"Unhandled attempt to Read IO Register");
	return 0xFF;
}

U8 MEM_getByte_VDP(U32 address,U32 upper24,U32 lower16)
{
	if (((upper24 & 0xE7)==0xC0) && ((lower16 & 0xE0)==0x00))					/* Valid mask for VDP register access : 110n n000 nnnn nnnn 000m mmmm  (n = any, m=vdp reg) */
	{
		return VDP_ReadByte(lower16&0x1F);
	}
	#if defined _DEBUG
	EMU_PRINTF("Byte read from unmapped VRAM address 0x%08X\n", (int)upper24 << 16 | lower16);
	#endif
	//DEB_PauseEmulation(DebugMode::M68K,"Invalid VDP area access");
	return 0xFF;
}

unsigned char* MEM_getDMA_VDP(U32 address)
{
	switch (VDP_latchIdCode)
	{
	case 0x00:				/* VRAM Read */
#if VRAM_128KB_MODE
		return &Memory::vRam[(VDP_latchDstAddress & 0x1FFFE) + 0];
#else
		return &Memory::vRam[(VDP_latchDstAddress & 0xFFFE) + 0];
#endif

	case 0x08:				/* CRAM Read */
		return &Memory::cRam[(VDP_latchDstAddress & 0x7E) + 0];

	case 0x04:				/* SVRAM Read */
		return &Memory::vsRam[(VDP_latchDstAddress & 0x4E) + 0];
	}

#if defined _DEBUG
	EMU_PRINTF("Reading Data via unsupported mode : %02X\n", VDP_latchIdCode);
#endif

	return NULL;
}

void MEM_setByte_CartRom(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	UNUSED_ARGUMENT(upper24);
	UNUSED_ARGUMENT(lower16);
	UNUSED_ARGUMENT(byte);
	#if defined _DEBUG
	EMU_PRINTF("Byte 0x%02X written to cart address 0x%08X\n", (int)byte, (int)upper24 << 16 | lower16);
	#endif
#if EMU_ENABLE_68K_DEBUGGER
	DEB_PauseEmulation(DebugMode::M68K,"Write to cart rom... !");
#endif
}

void MEM_setWord_CartRom(U32 address, U16 word)
{
	#if defined _DEBUG
	EMU_PRINTF("Byte 0x%02X written to cart address 0x%08X\n", (int)word, (int)address);
	#endif
#if EMU_ENABLE_68K_DEBUGGER
	DEB_PauseEmulation(DebugMode::M68K,"Write to cart rom... !");
#endif
}

void MEM_setByte_SystemRam(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	UNUSED_ARGUMENT(upper24);
	Memory::systemRam[lower16]=byte;
}

void MEM_setWord_SystemRam(U32 address, U16 word)
{
	*(U16*)&Memory::systemRam[address & 0x0000FFFF] = word;
}

void MEM_setByte_Pad(int padIdx, U16 reg, U8 byte)
{
	if (byte & 0x40)
	{
		//Decrement 6-button hi latch
		if (--io6ButtonLatchHi[padIdx] < 0)
			io6ButtonLatchHi[padIdx] = IO_6BUTTON_LATCH_WRITE_COUNT;
	}
	else
	{
		//Decrement 6-button lo latch
		if (--io6ButtonLatchLo[padIdx] < 0)
			io6ButtonLatchLo[padIdx] = IO_6BUTTON_LATCH_WRITE_COUNT;
	}

	//Reset capacitor timer
	io6buttonLatchTimer[padIdx] = IO_6BUTTON_LATCH_CYCLES;

	byte &= 0x80 | (ioRegisters[reg + 6] & 0x7F);
	ioRegisters[reg] &= 0x7F & (~(ioRegisters[reg + 6] & 0x7F));
	ioRegisters[reg] |= byte;
}

void MEM_setByte_IO(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	UNUSED_ARGUMENT(upper24);
#if ENABLE_32X_MODE

	if (lower16>=0x5100 && lower16<0x513A)
	{
		MD_SYS_32X_WRITE(lower16-0x5100,byte);
		return;
	}

	if (lower16>=0x5180 && lower16<=0x51FF)
	{
		if (AdapterControlRegister&0x8000)
			return;
		VDP_32X_Write(lower16-0x5180,byte,DebugMode::M68K);
		return;
	}

	if (lower16>=0x5200 && lower16<0x5400)
	{
		if (AdapterControlRegister&0x8000)
			return;
		CRAM[lower16-0x5200]=byte;
		return;
	}

#endif

	if (lower16 == 0x1100)
	{
		//EMU_PRINTF("Z80 bus 0x%02X\n", byte);
		Z80::Z80_regs.busRequested = byte&0x01;
		return;
	}

	if (lower16 == 0x1101 || lower16 == 0x1201)		/* ignored */
		return;

	if (lower16 == 0x1200)
	{
		//EMU_PRINTF("Z80 reset 0x%02X\n", byte);

		if (byte&0x1)
		{
			Z80::Z80_regs.resetLine=0;
		}
		else
		{
			Z80::Z80_regs.resetLine=1;
		}
		return;
	}

	if (lower16 == 0x30F1)
	{
		//TODO: Rearrange memory map instead of using a global redirect flag
		Globals::SRAM_Lock = byte & 0x01;
		EMU_PRINTF("SRAM lock %d\n", (int)byte);
		return;
	}

	if (lower16 < 0x20)
	{
		U16 reg = (lower16&0x1E)+1;
		switch (reg)			/* even mirrors odd */
		{
		case 0x11:					/* RxData Register Port A */
		case 0x17:					/* RxData Register Port B */
		case 0x1D:					/* RxData Register Port C */
		case 0x01:					/* Version Register					(assuming read only)*/
			return;
		case 0x03:					/* Data Register Port A		(bit 7 writeable + 6-0 if bit set in ctrl) */
			MEM_setByte_Pad(0, reg, byte);
			return;
		case 0x05:					/* Data Register Port B		(bit 7 writeable) */
			MEM_setByte_Pad(1, reg, byte);
			return;
		case 0x07:					/* Data Register Port C		(bit 7 writeable) */
			MEM_setByte_Pad(2, reg, byte);
			return;
		case 0x09:					/* Ctrl Register Port A */
		case 0x0B:					/* Ctrl Register Port B */
		case 0x0D:					/* Ctrl Register Port C */
		case 0x0F:					/* TxData Register Port A */
		case 0x15:					/* TxData Register Port B */
		case 0x1B:					/* TxData Register Port C */
			ioRegisters[reg]=byte;
			return;
		case 0x13:					/* S-Ctrl Register Port A */
		case 0x19:					/* S-Ctrl Register Port B */
		case 0x1F:					/* S-Ctrl Register Port C */
			byte&=0xF0;
			ioRegisters[reg]&=0x0F;
			ioRegisters[reg]|=byte;
			return;
		}
	}

	#if defined _DEBUG
	EMU_PRINTF("Byte 0x%02X written to unmapped address 0x%08X\n", (int)byte, (int)upper24<<16|lower16);
	#endif

	// DEB_PauseEmulation(DebugMode::M68K,"OOB IO Writes currently unhandled");
}

void MEM_setByte_VDP(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	if (((upper24 & 0xE7)==0xC0) && ((lower16 & 0xE0)==0x00))					/* Valid mask for VDP register access : 110n n000 nnnn nnnn 000m mmmm  (n = any, m=vdp reg) */
	{
		VDP_WriteByte(lower16&0x1F,byte);
		return;
	}
	#if defined _DEBUG
	EMU_PRINTF("Byte 0x%02X written to unmapped VRAM address 0x%08X\n", (int)byte, (int)upper24 << 16 | lower16);
	#endif
	//DEB_PauseEmulation(DebugMode::M68K,"Invalid VDP area access");
}

void MEM_setByte_Unmapped(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	UNUSED_ARGUMENT(upper24);
	UNUSED_ARGUMENT(lower16);
	UNUSED_ARGUMENT(byte);
	#if defined _DEBUG
	EMU_PRINTF("Byte 0x%02X written to unmapped address 0x%08X\n", (int)byte, (int)upper24 << 16 | lower16);
	#endif
	//DEB_PauseEmulation(DebugMode::M68K,"Unamapped Write - May need to crash/return prefetch queue/implement missing functionality");
}

U32 numBanks;

#if ENABLE_32X_MODE

extern U32 ActiveFrameBuffer;

U8	Vector70[4]={0xFF,0xFF,0xFF,0xFF};			/* Not sure what its default should be! */

U8 MEM_getByte_BiosOrCartRom(U32 address, U32 upper24,U32 lower16)
{
	UNUSED_ARGUMENT(upper24);
	if (lower16>=0x70 && lower16<=0x73)
		return Vector70[lower16-0x70];

	if (lower16<cpu68kbiosSize)
	{
		return cpu68kbios[lower16];
	}
	return Memory::cartRom[lower16];
}

void MEM_setByte_BiosOrCartRom(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	UNUSED_ARGUMENT(upper24);
	if (lower16>=0x70 && lower16<=0x73)
	{
		Vector70[lower16-0x70]=byte;
		return;
	}

	DEB_PauseEmulation(DebugMode::M68K,"Unmapped rom write");
}

void MEM_setByte_FRAMEBUFFER(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	if (AdapterControlRegister&0x8000)
		return;
	if (ActiveFrameBuffer)
		FRAMEBUFFER[((upper24-0x84)<<16) + lower16 + 0x20000]=byte;
	else
		FRAMEBUFFER[((upper24-0x84)<<16) + lower16]=byte;
}

U8 MEM_getByte_FRAMEBUFFER(U32 address, U32 upper24,U32 lower16)
{
	if (AdapterControlRegister&0x8000)
		return 0xFF;
	if (ActiveFrameBuffer)
		return FRAMEBUFFER[((upper24-0x84)<<16) + lower16 + 0x20000];
	else
		return FRAMEBUFFER[((upper24-0x84)<<16) + lower16];
}

void MEM_setByte_CartRomUpper(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	UNUSED_ARGUMENT(upper24);
	UNUSED_ARGUMENT(lower16);
	UNUSED_ARGUMENT(byte);

	DEB_PauseEmulation(DebugMode::M68K,"Write to upper rom");
}

U8 MEM_getByte_CartRomUpper(U32 address, U32 upper24,U32 lower16)
{
	return Memory::cartRom[((upper24-0x88)<<16) + lower16];
}

void MEM_setByte_CartRomBanked(U32 address, U32 upper24,U32 lower16,U8 byte)
{
	UNUSED_ARGUMENT(upper24);
	UNUSED_ARGUMENT(lower16);
	UNUSED_ARGUMENT(byte);

	DEB_PauseEmulation(DebugMode::M68K,"Write to rom banked");
}

U8 MEM_getByte_CartRomBanked(U32 address, U32 upper24,U32 lower16)
{
	U32 bankedAddress=((upper24-0x90)<<16) + lower16;

	switch (BankSetRegister&0x03)
	{
	case 0:
		return Memory::cartRom[bankedAddress];
	case 1:
		return Memory::cartRom[0x100000+bankedAddress];
	case 2:
		return Memory::cartRom[0x200000+bankedAddress];
	case 3:
		return Memory::cartRom[0x300000+bankedAddress];
	}

	return 0xFF;			/* We can't reach here */
}


void SwapTo32XModeMemoryMap()
{
	unsigned int a;

	Memory::mem_read[0] = MEM_getByte_BiosOrCartRom;
	Memory::mem_write[0]= MEM_setByte_BiosOrCartRom;

	Memory::mem_read[0x84] = MEM_getByte_FRAMEBUFFER;
	Memory::mem_read[0x85] = MEM_getByte_FRAMEBUFFER;
/*	Memory::mem_read[0x86] = MEM_getByte_FRAMEBUFFER;				DON'T HANDLE FRAME OVER STUFF YET
	Memory::mem_read[0x87] = MEM_getByte_FRAMEBUFFER;*/
	Memory::mem_write[0x84] = MEM_setByte_FRAMEBUFFER;
	Memory::mem_write[0x85] = MEM_setByte_FRAMEBUFFER;
/*	Memory::mem_write[0x86] = MEM_setByte_FRAMEBUFFER;
	Memory::mem_write[0x87] = MEM_setByte_FRAMEBUFFER;*/

	for (a=0;a<8;a++)
	{
		Memory::mem_read[0x88+a] = MEM_getByte_CartRomUpper;
		Memory::mem_write[0x88+a]= MEM_setByte_CartRomUpper;
	}
	for (a=0;a<16;a++)
	{
		Memory::mem_read[0x90+a] = MEM_getByte_CartRomBanked;
		Memory::mem_write[0x90+a]= MEM_setByte_CartRomBanked;
	}
}

void SwapToStandardMemoryMap()
{
	unsigned int a;

	Memory::mem_read[0] = MEM_getByte_CartRom;
	Memory::mem_write[0]= MEM_setByte_CartRom;

	Memory::mem_read[0x84] = MEM_getByte_Unmapped;
	Memory::mem_read[0x85] = MEM_getByte_Unmapped;
	Memory::mem_read[0x86] = MEM_getByte_Unmapped;
	Memory::mem_read[0x87] = MEM_getByte_Unmapped;
	Memory::mem_write[0x84] = MEM_setByte_Unmapped;
	Memory::mem_write[0x85] = MEM_setByte_Unmapped;
	Memory::mem_write[0x86] = MEM_setByte_Unmapped;
	Memory::mem_write[0x87] = MEM_setByte_Unmapped;

	for (a=0;a<8;a++)
	{
		Memory::mem_read[0x88+a] = MEM_getByte_Unmapped;
		Memory::mem_write[0x88+a]= MEM_setByte_Unmapped;
	}
	for (a=0;a<16;a++)
	{
		Memory::mem_read[0x90+a] = MEM_getByte_Unmapped;
		Memory::mem_write[0x90+a]= MEM_setByte_Unmapped;
	}
}
#endif

void InitialiseStandardMemoryMap()
{
	unsigned int a;

	for (a=0;a<256;a++)
	{
		Memory::mem_read[a] = MEM_getByte_Unmapped;
		Memory::mem_write[a] = MEM_setByte_Unmapped;
		Memory::mem_read_word[a] = MEM_getWord_Split;
		Memory::mem_write_word[a] = MEM_setWord_Split;
		Memory::mem_dma_addr[a] = MEM_getDMA_Unmapped;
	}

#if EMU_SUPPORT_8MB_ROM
	if (numBanks >= 0x80)
	{
		numBanks = 0x80;
	}
#else
	if (numBanks>=0x40)
	{
		numBanks=0x40;
	}
#endif

	for (a=0;a<numBanks;a++)
	{
		Memory::mem_read[a] = MEM_getByte_CartRom;
		Memory::mem_write[a]= MEM_setByte_CartRom;
		Memory::mem_read_word[a] = MEM_getWord_CartRom;
		Memory::mem_write_word[a] = MEM_setWord_CartRom;
		Memory::mem_dma_addr[a] = MEM_getDMA_CartRom;
	}

	for (a=0xC0;a<0xE0;a++)
	{
		Memory::mem_read[a] = MEM_getByte_VDP;
		Memory::mem_write[a]= MEM_setByte_VDP;
		Memory::mem_dma_addr[a] = MEM_getDMA_VDP;
	}

	for (a=0xE0;a<0x100;a++)
	{
		Memory::mem_read[a] = MEM_getByte_SystemRam;
		Memory::mem_write[a]= MEM_setByte_SystemRam;
		Memory::mem_read_word[a] = MEM_getWord_SystemRam;
		Memory::mem_write_word[a] = MEM_setWord_SystemRam;
		Memory::mem_dma_addr[a] = MEM_getDMA_SystemRam;
	}

	Memory::mem_read[0xA0] = MEM_getByte_Z80;
	Memory::mem_write[0xA0]= MEM_setByte_Z80;
	Memory::mem_read[0xA1] = MEM_getByte_IO;
	Memory::mem_write[0xA1]= MEM_setByte_IO;

	if (Globals::SRAM)
	{
		for (a=0;a<=(Globals::SRAM_Size/65536);a++)
		{
			/* TODO add sanity check that SRAM not mapping ontop of some other allocated region	-- e.g. ps4 */
			Memory::mem_read[((Globals::SRAM_StartAddress&0x00FF0000)>>16)+a]=MEM_getByte_SRAM;
			Memory::mem_write[((Globals::SRAM_StartAddress&0x00FF0000)>>16)+a]=MEM_setByte_SRAM;
		}
	}

}

void MEM_Initialise(unsigned char *_romPtr, U32 romSize, unsigned int num64Banks)
{
	unsigned int a=0;

	numBanks=num64Banks;
	Memory::systemRam = (unsigned char*)malloc(SYS_RAM_SIZE);
	Memory::z80Ram = (unsigned char*)malloc(Z80_RAM_SIZE);
	Memory::cartRom = _romPtr;
	cartRomSize = romSize;
	Memory::vRam = (unsigned char*)malloc(VRAM_SIZE);
	Memory::vsRam = (unsigned char*)malloc(VSRAM_SIZE);
	Memory::cRam = (unsigned char*)malloc(CRAM_SIZE);
	Memory::cartRam = (unsigned char*)malloc(CARTRAM_SIZE);

#if EMU_CLEAR_MEMORY
	memset(Memory::systemRam, 0, SYS_RAM_SIZE);
	memset(Memory::z80Ram, 0, Z80_RAM_SIZE);
	memset(Memory::vRam, 0, VRAM_SIZE);
	memset(Memory::vsRam, 0, VSRAM_SIZE);
	memset(Memory::cRam, 0, CRAM_SIZE);
	memset(Memory::cartRam, 0, CARTRAM_SIZE);
#endif

	/* Initialise IO */
	for (a=0;a<0x20;a++)
	{
		ioRegisters[a]=0x00;
	}

#if PAL_PRETEND
	ioRegisters[0x01]=0xE0;				/* MDL (0 - domestic , 1 - overseas) VMD (0 - NTSC clock, 1 - PAL clock) DSK (0 floppy connected, 1 floppy disconnected) RSV(reserved) (VR3-VR0 version) */
#else
	ioRegisters[0x01]=0xA0;				/* MDL (0 - domestic , 1 - overseas) VMD (0 - NTSC clock, 1 - PAL clock) DSK (0 floppy connected, 1 floppy disconnected) RSV(reserved) (VR3-VR0 version) */
#endif
	ioRegisters[0x03]=0x7F;
	ioRegisters[0x05]=0x7F;
	ioRegisters[0x07]=0x7F;
	ioRegisters[0x0F]=0xFF;
	ioRegisters[0x15]=0xFF;
	ioRegisters[0x1B]=0xFF;


	InitialiseStandardMemoryMap();

	VDP::cramCacheDirty = 1;
}
