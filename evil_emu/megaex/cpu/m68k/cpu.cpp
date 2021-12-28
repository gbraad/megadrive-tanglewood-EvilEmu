/*
 *  cpu.c

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
#include <assert.h>

#include "config.h"

#include "megaex/cpu/m68k/cpu_dis.h"
#include "megaex/cpu/m68k/cpu_ops.h"
#include "megaex/cpu/m68k/cpu.h"
#include "megaex/memory.h"
#include "megaex/gui/debugger.h"
#include "megaex/vdp/vdp.h"

#include "megaex/cpu/m68k/cpu_ops.inl"

CPU_Regs M68K::cpu_regs;
char M68K::mnemonicData[256];

U8	cpuUsedTable[65536];

int		startDebug=0;

void CPU_SaveState(FILE *outStream)
{
	fwrite(&M68K::cpu_regs,1,sizeof(CPU_Regs),outStream);
}

void CPU_LoadState(FILE *inStream)
{
	if (sizeof(CPU_Regs)!=fread(&M68K::cpu_regs,1,sizeof(CPU_Regs),inStream))
	{
		return;
	}
}

void CPU_Reset()
{
	int a;
	
	for (a=0;a<8;a++)
	{
		M68K::cpu_regs.D[a]=0;
		M68K::cpu_regs.A[a]=0;
	}
	M68K::cpu_regs.SR = CPU_STATUS_S | CPU_STATUS_I2 | CPU_STATUS_I1 | CPU_STATUS_I0;
	
	M68K::cpu_regs.A[7] = MEM_getLong(/*0xFC0000*/0);
	M68K::cpu_regs.PC = MEM_getLong(/*0xFC0004*/4);

	#if defined _DEBUG
	EMU_PRINTF("A7 Reset %08x : %08x\n",MEM_getLong(0xFC0000),M68K::cpu_regs.A[7]);
	EMU_PRINTF("PC Reset %08x : %08x\n",MEM_getLong(0xFC0004),M68K::cpu_regs.PC);
	#endif

	for (a=0;a<65536;a++)
		cpuUsedTable[a]=0;
		
	M68K::cpu_regs.stage=0;

#if CPU_DEBUG_CALLTRACE
	M68K::cpu_regs.calltraceIdx = 0;
#endif
}

const char *decodeWord(U32 address)
{
	static char buffer[256];
	
	sprintf(buffer,"%04X",MEM_getWord(address));
	
	return buffer;
}

const char *decodeLong(U32 address)
{
	static char buffer[256];
	
	sprintf(buffer,"%04X %04X",MEM_getWord(address),MEM_getWord(address+2));
	
	return buffer;
}

/*///////////////////////////////////////////////////////////////////////*/
/*               MC68000 Instruction Execution Times

               -----------------------------------



  These are the times for instructons, most of it is self explainitory.
  On the ST at 8 Mhz you need to round all times to multiples of four.
  i.e 10 becomes 12. Please note that  execution instruction times are
  generally irrelevant when you have a instruction cache, ie a greater
  than 68000  processor and i doubt that these numbers will hold true
  for anything except a 68000 even if you turn the cache off. Also note
  that it isn't usually worth spending ages trying to optimise your code
  by using faster instructions. If you're code is too slow, then you will
  probably need to use a different method to achieve you're aims.



MOVE Instructions:

                                               d(an
.b.w/.l    dn   an     (an)  (an)+ -(an) d(an) .Ri)  abs.s abs.l



dn         4/4   4/4    8/12  8/12  8/14 12/16 14/18 12/16 16/20
an         4/4   4/4    8/12  8/12  8/14 12/16 14/18 12/16 16/20
(an)       8/12  8/12  12/20 12/20 12/20 16/24 18/26 16/24 20/28
(an)+      8/12  8/12  12/20 12/20 12/20 16/24 18/26 16/24 20/28
-(an)     10/14  10/14 14/22 14/22 14/22 18/26 20/28 18/26 22/30
d(an)     12/16  12/16 16/24 16/24 16/24 20/28 22/30 20/28 24/32
d(an,Ri)  14/18  14/18 18/26 18/26 18/26 22/30 24/32 22/30 26/34
Abs.s     12/16  12/16 16/24 16/24 16/24 20/28 22/30 20/28 24/32
Abs.l     16/20  16/20 20/28 20/28 20/28 24/32 26/34 24/32 28/36
d(pc)     12/16  12/16 16/24 16/24 16/24 20/28 22/30 20/28 24/32
d(pc,Ri)  14/18  14/18 18/26 18/26 18/26 22/30 24/32 22/30 26/34
Immediate  8/12   8/12 12/20 12/20 12/20 16/24 18/26 16/24 20/28



----------------------------------------------------------------------

Time to calculate effective addresses.

                                 d(an                    d(pc
          (an) (an)+ -(an) d(an) .Ri)  abs.s abs.l d(pc) .ri)  Imm
.b.w/.l  4/8  4/8   6/10  8/12  10/14 8/12  12/16 8/12  10/14 4/8


The time taken to calculate the effective address must be added to
       instructions that affect a memory address.

----------------------------------------------------------------------

Standard Instructions:

.b.w/.l   ea,an   ea,dn   dn,mem

add       8/6(8)  4/6(8)  8/12    (8) time if effective address
and        -      4/6(8)  8/12    is direct
cmp       6/6     4/6      -
divs       -      158max   -          Add effective address times
divu       -      140max   -          from above for memory
eor        -      4/8     8/12        addresses.
muls       -      70max    -
mulu       -      70max    -
or         -      4/6(8)  8/12
sub       8/6(8)  4/6(8)  8/12



Immediate Instructions


.b.w/.l  #,dn  #,an  #,mem

addi     8/16   -    12/20
addq     4/8   8/8    8/12   Moveq.l only
andi     8/16   -    12/20   nbcd+tas.b only
cmpi     8/14  8/14   8/12
eori     8/16   -    12/20   scc false/true
moveq     4     -      -
ori      8/16   -    12/20   add effective address
subi     8/16   -    12/20   times from above
subq     4/8   8/8    8/12   for mem addresses
clr      4/6   4/6   8/12    single operand
nbcd      6     6     8      instructions
neg      4/6   4/6   8/12
negx     4/6   4/6   8/12
not      4/6   4/6   8/12
scc      4/6   4/6   8/8
tas       4     4    10
tst      4/4   4/4   4/4



Shift/rotate instructions.


.b.w/.l   dn    an   mem

asr,asl   6/8   6/8   8      memory is byte only
lsr,lsl   6/8   6/8   8      register add 2x
ror,rol   6/8   6/8   8      shift count
roxr,roxl 6/8   6/8   8



                                  d(an                   d(pc
         (an)  (an)+  -(an) d(an) .ri) abs.s abs.l d(pc) .ri)

jmp      8     -      -     10    14   10    12    10    14
jsr      16    -      -     18    22   18    20    18    22
lea      4     -      -     8     12   8     12    8     12
pea      12    -      -     16    20   16    20    16    20

movem t=4
m>r      12    12     -     16    18   16    20    16    18

movem t=5
r>m      8     -      8     12    14   12    16    -     -


movem   add t x number of registers for .w
movem   add 2t x number of registers for .l



Bit Instructions


.b/.l   register .l    memory .b
           only        only

bchg     8/12          8/12
bclr    10/14          8/12
bset     8/12          8/12
btst     6/10          4/8



Exceptions       Periods

Address Error    50
Bus Error        50
Interrupt        44
Illegal Instr.   34
Privilege Viol.  34
Trace            34



Other Instructions

add effective address times from above for memory addresses


.b.w/.l  dn,dn    m,m

addx      4/8    18/30
cmpm       -     12/20
subx      4/8    18/30
abcd       6      18      .b only
sbcd       6      18      .b only
Bcc      .b/.w   10/10      8/12
bra      .b/.w   10/10       -
bsr      .b/.w   18/18       -
DBcc      t/f      10      12/14
chk        -       40 max    8
trap       -       34        -
trapv      -       34        4

                 reg<>mem

movep   .w/.l   16/24

              Reg   Mem                     Reg

andi to ccr   20     -       move from usp    4
andi to sr    20     -       nop              4
eori to ccr   20     -       ori to ccr      20
eori to sr    20     -       ori to sr       20
exg            6     -       reset          132
ext            4     -       rte             20
link          18     -       rtr             20
move to ccr   12    12       rts             16
move to sr    12    12       stop             4
move from sr   6     8       swap             4
move to usp    4     -       unlk            12
*/

CPU_Ins cpu_instructions[] = 
{
/* Supervisor instructions */
{ 0,"0100111001110010","STOP",CPU_STOP,CPU_DIS_STOP,0,{0},{0},{0},{{""}}},
{ 1,"0100011011aaaaaa","MOVESR",CPU_MOVETOSR,CPU_DIS_MOVETOSR,1,{0x003F},{0},{11},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{ 2,"0000001001111100","ANDSR",CPU_ANDSR,CPU_DIS_ANDSR,0,{0},{0},{0},{{""}}},
{ 3,"0100111001110011","RTE",CPU_RTE,CPU_DIS_RTE,0,{0},{0},{0},{{""}}},
{ 4,"0000000001111100","ORSR",CPU_ORSR,CPU_DIS_ORSR,0,{0},{0},{0},{{""}}},
{ 5,"010011100110mrrr","MOVEUSP",CPU_MOVEUSP,CPU_DIS_MOVEUSP,2,{0x0008,0x0007},{3,0},{1,1},{{"r"},{"rrr"}}},
{ 6,"0000101001111100","EORISR",CPU_EORISR,CPU_DIS_EORISR,0,{0},{0},{0},{{""}}},
{ 7,"0100111001110000","RESET",CPU_RESET,CPU_DIS_RESET,0,{0},{0},{0},{{""}}},
/* Illegal instructions - that programs use - need to retest the 2 below, because of change to process exception to push pc-2 and not pc for linea */
/*{"0100001011aaaaaa","MOVEfromCCR",CPU_ILLEGAL,CPU_DIS_ILLEGAL,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},	// ODYSSEY demo uses this*/
/*{"0100111001111011","ILLEGAL",CPU_ILLEGAL,CPU_DIS_ILLEGAL,0},		// KS does this one*/
{ 8,"1111aaaaaaaaaaaa","LINEF",CPU_LINEF,CPU_DIS_LINEF,1,{0x0FFF},{0},{1},{{"rrrrrrrrrrrr"}}},					/* Zaxxon 32X */
{ 9,"1010aaaaaaaaaaaa","LINEA",CPU_LINEA,CPU_DIS_LINEA,1,{0x0FFF},{0},{1},{{"rrrrrrrrrrrr"}}},					/* Star cruiser megadrive */
/* User instructions */
{ 10,"1110011011aaaaaa","ROR",CPU_RORm,CPU_DIS_RORm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 11,"1110011111aaaaaa","ROL",CPU_ROLm,CPU_DIS_ROLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 12,"1110010011aaaaaa","ROXR",CPU_ROXRm,CPU_DIS_ROXRm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 13,"0100111001110110","TRAPV",CPU_TRAPV,CPU_DIS_TRAPV,0,{0},{0},{0},{{""}}},
{ 14,"1110001111aaaaaa","LSL",CPU_LSLm,CPU_DIS_LSLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 15,"0100111001110111","RTR",CPU_RTR,CPU_DIS_RTR,0,{0},{0},{0},{{""}}},
{ 16,"1000rrr100001aaa","SBCD",CPU_SBCDm,CPU_DIS_SBCDm,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{ 17,"0000ddd101001aaa","MOVEP",CPU_MOVEP_m_L,CPU_DIS_MOVEP_m_L,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{ 18,"1110010111aaaaaa","ROXL",CPU_ROXLm,CPU_DIS_ROXLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 19,"1100rrr100001aaa","ABCD",CPU_ABCDm,CPU_DIS_ABCDm,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{ 20,"0000ddd100001aaa","MOVEP",CPU_MOVEP_m_W,CPU_DIS_MOVEP_m_W,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{ 21,"0000ddd111001aaa","MOVEP",CPU_MOVEP_L_m,CPU_DIS_MOVEP_L_m,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{ 22,"0000ddd110001aaa","MOVEP",CPU_MOVEP_W_m,CPU_DIS_MOVEP_W_m,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{ 23,"1100rrr100000ddd","ABCD",CPU_ABCD,CPU_DIS_ABCD,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{ 24,"1000rrr100000ddd","SBCD",CPU_SBCD,CPU_DIS_SBCD,2,{0x0E00,0x0007},{9,0},{1,1},{{"rrr","rrr"}}},
{ 25,"01000000zzaaaaaa","NEGX",CPU_NEGX,CPU_DIS_NEGX,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 26,"0000000000111100","ORCCR",CPU_ORICCR,CPU_DIS_ORICCR,0,{0},{0},{0},{{""}}},
{ 27,"0000001000111100","ANDCCR",CPU_ANDICCR,CPU_DIS_ANDICCR,0,{0},{0},{0},{{""}}},
{ 28,"1110000011aaaaaa","ASR",CPU_ASRm,CPU_DIS_ASRm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 29,"1110000111aaaaaa","ASL",CPU_ASLm,CPU_DIS_ASLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
/* 1001rrr1zz001ddd  9100 -> 9FCF	SUBX */
{ 30,"1001rrr1zz000ddd","SUBX",CPU_SUBX,CPU_DIS_SUBX,3,{0x0E00,0x00C0,0x0007},{9,6,0},{1,3,1},{{"rrr"},{"00","01","10"},{"rrr"}}},
/*{"1101rrr1zz001ddd","ADDX",CPU_ADDXm,CPU_DIS_ADDXm,  D100 -> DFC0      ADDX*/
{ 41,"0100000011aaaaaa","MOVESR",CPU_MOVEFROMSR,CPU_DIS_MOVEFROMSR,1,{ 0x003F },{ 0 },{ 8 },{ { "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 42,"1000rrr111aaaaaa","DIVS",CPU_DIVS,CPU_DIS_DIVS,2,{ 0x0E00,0x003F },{ 9,0 },{ 1,11 },{ { "rrr" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 43,"1101rrr1zz000ddd","ADDX",CPU_ADDX,CPU_DIS_ADDX,3,{ 0x0E00,0x00C0,0x0007 },{ 9,6,0 },{ 1,3,1 },{ { "rrr" },{ "00","01","10" },{ "rrr" } } },
{ 44,"010011100100rrrr","TRAP",CPU_TRAP,CPU_DIS_TRAP,1,{ 0x000F },{ 0 },{ 1 },{ { "rrrr" } } },
{ 45,"0100010011aaaaaa","MOVECCR",CPU_MOVETOCCR,CPU_DIS_MOVETOCCR,1,{ 0x003F },{ 0 },{ 11 },{ { "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 46,"1110ccc0zzm10rrr","ROXR",CPU_ROXR,CPU_DIS_ROXR,4,{ 0x0E00,0x000C0,0x0020,0x0007 },{ 9,6,5,0 },{ 1,3,1,1 },{ { "rrr" },{ "00","01","10" },{ "r" },{ "rrr" } } },
{ 47,"1110ccc1zzm10rrr","ROXL",CPU_ROXL,CPU_DIS_ROXL,4,{ 0x0E00,0x000C0,0x0020,0x0007 },{ 9,6,5,0 },{ 1,3,1,1 },{ { "rrr" },{ "00","01","10" },{ "r" },{ "rrr" } } },
{ 48,"0000101000111100","EORICCR",CPU_EORICCR,CPU_DIS_EORICCR,0,{ 0 },{ 0 },{ 0 },{ { "" } } },
{ 49,"00001010zzaaaaaa","EORI",CPU_EORI,CPU_DIS_EORI,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 50,"1110001011aaaaaa","LSR",CPU_LSRm,CPU_DIS_LSRm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 51,"0000100001aaaaaa","BCHGI",CPU_BCHGI,CPU_DIS_BCHGI,1,{ 0x003F },{ 0 },{ 8 },{ { "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 52,"0000rrr101aaaaaa","BCHG",CPU_BCHG,CPU_DIS_BCHG,2,{ 0x0E00,0x003F },{ 9,0 },{ 1,8 },{ { "rrr" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 53,"0100111001110001","NOP",CPU_NOP,CPU_DIS_NOP,0,{ 0 },{ 0 },{ 0 },{ { "" } } },
{ 54,"1110ccc0zzm11rrr","ROR",CPU_ROR,CPU_DIS_ROR,4,{ 0x0E00,0x000C0,0x0020,0x0007 },{ 9,6,5,0 },{ 1,3,1,1 },{ { "rrr" },{ "00","01","10" },{ "r" },{ "rrr" } } },
{ 55,"1110ccc1zzm11rrr","ROL",CPU_ROL,CPU_DIS_ROL,4,{ 0x0E00,0x000C0,0x0020,0x0007 },{ 9,6,5,0 },{ 1,3,1,1 },{ { "rrr" },{ "00","01","10" },{ "r" },{ "rrr" } } },
{ 56,"0000rrr100aaaaaa","BTST",CPU_BTST,CPU_DIS_BTST,2,{ 0x0E00,0x003F },{ 9,0 },{ 1,11 },{ { "rrr" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 57,"1011rrr1mmaaaaaa","EOR",CPU_EORd,CPU_DIS_EORd,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,8 },{ { "rrr" },{ "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 58,"0000rrr110aaaaaa","BCLR",CPU_BCLR,CPU_DIS_BCLR,2,{ 0x0E00,0x003F },{ 9,0 },{ 1,8 },{ { "rrr" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 59,"1000rrr011aaaaaa","DIVU",CPU_DIVU,CPU_DIS_DIVU,2,{ 0x0E00,0x003F },{ 9,0 },{ 1,11 },{ { "rrr" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 60,"1110ccc0zzm00rrr","ASR",CPU_ASR,CPU_DIS_ASR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{ 61,"1110ccc1zzm00rrr","ASL",CPU_ASL,CPU_DIS_ASL,4,{ 0x0E00,0x000C0,0x0020,0x0007 },{ 9,6,5,0 },{ 1,3,1,1 },{ { "rrr" },{ "00","01","10" },{ "r" },{ "rrr" } } },
{ 62,"00000000zzaaaaaa","ORI",CPU_ORI,CPU_DIS_ORI,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 63,"1100rrr1mmaaaaaa","AND",CPU_ANDd,CPU_DIS_ANDd,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,7 },{ { "rrr" },{ "00","01","10" },{ "010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 64,"1000rrr0mmaaaaaa","OR",CPU_ORs,CPU_DIS_ORs,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,11 },{ { "rrr" },{ "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 65,"0100111001011rrr","UNLK",CPU_UNLK,CPU_DIS_UNLK,1,{ 0x0007 },{ 0 },{ 1 },{ { "rrr" } } },
{ 66,"1101rrr1mmaaaaaa","ADD",CPU_ADDd,CPU_DIS_ADDd,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,7 },{ { "rrr" },{ "00","01","10" },{ "010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 67,"1011rrr1zz001ddd","CMPM",CPU_CMPM,CPU_DIS_CMPM,3,{ 0x0E00,0x00C0,0x0007 },{ 9,6,0 },{ 1,3,1 },{ { "rrr" },{ "00","01","10" },{ "rrr" } } },
{ 68,"0100111001010rrr","LINK",CPU_LINK,CPU_DIS_LINK,1,{ 0x0007 },{ 0 },{ 1 },{ { "rrr" } } },
{ 69,"0100100001aaaaaa","PEA",CPU_PEA,CPU_DIS_PEA,1,{ 0x003F },{ 0 },{ 7 },{ { "010rrr","101rrr","110rrr","111000","111001","111010","111011" } } },
{ 70,"0101cccc11aaaaaa","Scc",CPU_SCC,CPU_DIS_SCC,2,{0x0F00,0x003F},{8,0},{1,8},{{"rrrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 71,"01000100zzaaaaaa","NEG",CPU_NEG,CPU_DIS_NEG,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 72,"1100rrr111aaaaaa","MULS",CPU_MULS,CPU_DIS_MULS,2,{ 0x0E00,0x003F },{ 9,0 },{ 1,11 },{ { "rrr" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 73,"01001000mm000rrr","EXT",CPU_EXT,CPU_DIS_EXT,2,{ 0x00C0,0x0007 },{ 6,0 },{ 2,1 },{ { "10","11" },{ "rrr" } } },
{ 74,"00000110zzaaaaaa","ADDI",CPU_ADDI,CPU_DIS_ADDI,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 75,"1110ccc1zzm01rrr","LSL",CPU_LSL,CPU_DIS_LSL,4,{ 0x0E00,0x000C0,0x0020,0x0007 },{ 9,6,5,0 },{ 1,3,1,1 },{ { "rrr" },{ "00","01","10" },{ "r" },{ "rrr" } } },
{ 76,"1100rrr011aaaaaa","MULU",CPU_MULU,CPU_DIS_MULU,2,{ 0x0E00,0x003F },{ 9,0 },{ 1,11 },{ { "rrr" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 77,"0000100011aaaaaa","BSETI",CPU_BSETI,CPU_DIS_BSETI,1,{ 0x003F },{ 0 },{ 8 },{ { "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 78,"0000rrr111aaaaaa","BSET",CPU_BSET,CPU_DIS_BSET,2,{ 0x0E00,0x003F },{ 9,0 },{ 1,8 },{ { "rrr" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 79,"1001rrr1mmaaaaaa","SUB",CPU_SUBd,CPU_DIS_SUBd,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,7 },{ { "rrr" },{ "00","01","10" },{ "010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 80,"1100rrr0mmaaaaaa","AND",CPU_ANDs,CPU_DIS_ANDs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,11},{{"rrr"},{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{ 81,"0000100010aaaaaa","BCLRI",CPU_BCLRI,CPU_DIS_BCLRI,1,{ 0x003F },{ 0 },{ 8 },{ { "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 82,"0100111010aaaaaa","JSR",CPU_JSR,CPU_DIS_JSR,1,{ 0x003F },{ 0 },{ 7 },{ { "010rrr","101rrr","110rrr","111000","111001","111010","111011" } } },
{ 83,"1100rrr1mmmmmddd","EXG",CPU_EXG,CPU_DIS_EXG,3,{ 0x0E00,0x00F8,0x0007 },{ 9,3,0 },{ 1,3,1 },{ { "rrr" },{ "01000","01001","10001" },{ "rrr" } } },
{ 84,"00000010zzaaaaaa","ANDI",CPU_ANDI,CPU_DIS_ANDI,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 85,"01000010zzaaaaaa","CLR",CPU_CLEAR,CPU_DIS_CLR,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 86,"0101ddd0zzaaaaaa","ADDQ",CPU_ADDQ,CPU_DIS_ADDQ,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,9 },{ { "rrr" },{ "00","01","10" },{ "000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 87,"1000rrr1mmaaaaaa","OR",CPU_ORd,CPU_DIS_ORd,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,7 },{ { "rrr" },{ "00","01","10" },{ "010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 88,"0100111001110101","RTS",CPU_RTS,CPU_DIS_RTS,0,{ 0 },{ 0 },{ 0 },{ { "" } } },
{ 89,"01100001dddddddd","BSR",CPU_BSR,CPU_DIS_BSR,1,{ 0x00FF },{ 0 },{ 1 },{ { "rrrrrrrr" } } },
{ 90,"00000100zzaaaaaa","SUBI",CPU_SUBI,CPU_DIS_SUBI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 91,"010010001zaaaaaa","MOVEM",CPU_MOVEMd,CPU_DIS_MOVEMd,2,{ 0x0040,0x003F },{ 6,0 },{ 1,6 },{ { "r" },{ "010rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 92,"010011001zaaaaaa","MOVEM",CPU_MOVEMs,CPU_DIS_MOVEMs,2,{ 0x0040,0x003F },{ 6,0 },{ 1,8 },{ { "r" },{ "010rrr","011rrr","101rrr","110rrr","111000","111001","111010","111011" } } },
{ 93,"0100100001000rrr","SWAP",CPU_SWAP,CPU_DIS_SWAP,1,{ 0x0007 },{ 0 },{ 1 },{ { "rrr" } } },
{ 94,"1110ccc0zzm01rrr","LSR",CPU_LSR,CPU_DIS_LSR,4,{ 0x0E00,0x000C0,0x0020,0x0007 },{ 9,6,5,0 },{ 1,3,1,1 },{ { "rrr" },{ "00","01","10" },{ "r" },{ "rrr" } } },
{ 95,"1001rrr0mmaaaaaa","SUB",CPU_SUBs,CPU_DIS_SUBs,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,12 },{ { "rrr" },{ "00","01","10" },{ "000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 96,"0111rrr0dddddddd","MOVEQ",CPU_MOVEQ,CPU_DIS_MOVEQ,2,{ 0x0E00,0x00FF },{ 9,0 },{ 1,1 },{ { "rrr" },{ "rrrrrrrr" } } },
{ 97,"0100111011aaaaaa","JMP",CPU_JMP,CPU_DIS_JMP,1,{ 0x003F },{ 0 },{ 7 },{ { "010rrr","101rrr","110rrr","111000","111001","111010","111011" } } },
{ 98,"01001010zzaaaaaa","TST",CPU_TST,CPU_DIS_TST,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 99,"1101rrrm11aaaaaa","ADDA",CPU_ADDA,CPU_DIS_ADDA,3,{ 0x0E00,0x0100,0x003F },{ 9,8,0 },{ 1,1,12 },{ { "rrr" },{ "r" },{ "000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 100,"1001rrrm11aaaaaa","SUBA",CPU_SUBA,CPU_DIS_SUBA,3,{0x0E00,0x0100,0x003F},{9,8,0},{1,1,12},{{"rrr"},{"r"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{ 101,"01000110zzaaaaaa","NOT",CPU_NOT,CPU_DIS_NOT,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 102,"1101rrr0mmaaaaaa","ADD",CPU_ADDs,CPU_DIS_ADDs,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,12 },{ { "rrr" },{ "00","01","10" },{ "000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 103,"0000100000aaaaaa","BTSTI",CPU_BTSTI,CPU_DIS_BTSTI,1,{ 0x003F },{ 0 },{ 10 },{ { "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111010","111011" } } },
{ 104,"01100000dddddddd","BRA",CPU_BRA,CPU_DIS_BRA,1,{ 0x00FF },{ 0 },{ 1 },{ { "rrrrrrrr" } } },
{ 105,"0101cccc11001rrr","DBCC",CPU_DBCC,CPU_DIS_DBCC,2,{ 0x0F00,0x0007 },{ 8,0 },{ 1,1 },{ { "rrrr" },{ "rrr" } } },
{ 106,"00zzddd001aaaaaa","MOVEA",CPU_MOVEA,CPU_DIS_MOVEA,3,{ 0x3000,0x0E00,0x003F },{ 12,9,0 },{ 2,1,12 },{ { "11","10" },{ "rrr" },{ "000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 107,"00001100zzaaaaaa","CMPI",CPU_CMPI,CPU_DIS_CMPI,2,{ 0x00C0,0x003F },{ 6,0 },{ 3,8 },{ { "00","01","10" },{ "000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001" } } },
{ 108,"1011rrr0mmaaaaaa","CMP",CPU_CMP,CPU_DIS_CMP,3,{ 0x0E00,0x00C0,0x003F },{ 9,6,0 },{ 1,3,12 },{ { "rrr" },{ "00","01","10" },{ "000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 109,"1011rrrm11aaaaaa","CMPA",CPU_CMPA,CPU_DIS_CMPA,3,{ 0x0E00,0x0100,0x003F },{ 9,8,0 },{ 1,1,12 },{ { "rrr" },{ "r" },{ "000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011" } } },
{ 110,"0110ccccdddddddd","BCC",CPU_BCC,CPU_DIS_BCC,2,{0x0F00,0x00FF},{8,0},{3,1},{{"rr1r","r1rr","1rrr"},{"rrrrrrrr"}}},
{ 111,"0101ddd1zzaaaaaa","SUBQ",CPU_SUBQ,CPU_DIS_SUBQ,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,9},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{ 112,"00zzaaaaaaAAAAAA","MOVE",CPU_MOVE,CPU_DIS_MOVE,3,{0x3000,0x0FC0,0x003F},{12,6,0},{3,8,12},{{"01","10","11"},{"rrr000","rrr010","rrr011","rrr100","rrr101","rrr110","000111","001111"},{"000rrr","001???","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{ 113,"0100rrr111aaaaaa","LEA",CPU_LEA,CPU_DIS_LEA,2,{0x0E00,0x003F},{9,0},{1,7},{{"rrr"},{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{ 114,"","",0,0,0,{0},{0},{0},{{""}}}
};

U8				CPU_JumpTableIdx[65536];
CPU_Function	CPU_JumpTable[65536];
CPU_Decode		CPU_DisTable[65536];
CPU_Ins*		CPU_Information[65536];

/*
/// 0100100001001vvv  4848 -> 484F	 BKPT
/// 0100rrrss0aaaaaa  4000 -> 4FBF	 CHK
/// 0100101011111100  4AFC -> 4AFC	 ILLEGAL
/// 0100100000aaaaaa  4800 -> 483F	 NBCD
/// 0100101011aaaaaa  4AC0 -> 4AFF	 TAS
*/
U32 CPU_UNKNOWN(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);
	UNUSED_ARGUMENT(operands);
	M68K::cpu_regs.PC-=2;	/* account for prefetch */
	#if defined _DEBUG
	EMU_PRINTF("ILLEGAL INSTRUCTION %08x\n",M68K::cpu_regs.PC);
	#endif
	DEB_PauseEmulation(DebugMode::M68K,"ILLEGAL INSTRUCTION");
	return 0;
}

const char *byte_to_binary(U32 x)
{
    U32 z;
    static char b[17] = {0};
	
    b[0]=0;
    for (z = 32768; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }
	
    return b;
}

U8 ValidateOpcode(int insNum,U16 opcode)
{
    U8 invalidMask=0;
    int a,b,c;
    char *mask;
    int operandNum=0;
    U8 lastBase=0;
	
    for (a=0;a<16;a++)
    {
		if (cpu_instructions[insNum].baseTable[a]!=lastBase && cpu_instructions[insNum].baseTable[a]!='0' && cpu_instructions[insNum].baseTable[a]!='1')
		{
			U16 operand = (opcode & cpu_instructions[insNum].operandMask[operandNum]);
			operand >>= cpu_instructions[insNum].operandShift[operandNum];
			lastBase = cpu_instructions[insNum].baseTable[a];
			
			for (b=0;b<cpu_instructions[insNum].numValidMasks[operandNum];b++)
			{
				invalidMask = 0;
				mask = cpu_instructions[insNum].validEffectiveAddress[operandNum][b];
				while (*mask!=0)
				{
					c=(int)strlen(mask)-1;
					switch (*mask)
					{
						case '0':
							if ((operand & (1<<c)) != 0)
								invalidMask=1;
							break;
						case '1':
							if ((operand & (1<<c)) == 0)
								invalidMask=1;
							break;
						case 'r':
							break;
						case '?':
							if ( (opcode & 0x3000) == 0x1000)
								invalidMask=1;			/* special case - address register direct not supported for byte size operations */
							break;
						case '!':
							if ( (opcode & 0x00C0) == 0x0000)
								invalidMask=1;			/* special case - address register direct not supported for byte size operations */
					}
					mask++;
				}
				if (!invalidMask)
					break;
			}
			if (invalidMask)
				return 0;
			operandNum++;
		}
    }
	
    return 1;
}

void CPU_BuildTable()
{
	int a,b,c,d;
	int numInstructions=0;
	
	for (a=0;a<65536;a++)
	{
		CPU_JumpTableIdx[a]=0;
		CPU_JumpTable[a]=CPU_UNKNOWN;
		CPU_DisTable[a]=CPU_DIS_UNKNOWN;
		CPU_Information[a]=0;
	}
	
	a=0;
	while (cpu_instructions[a].opcode)
	{
	    int modifiableBitCount=0;
		
	    /* precount modifiable bits */
	    for (b=0;b<16;b++)
	    {
			switch (cpu_instructions[a].baseTable[b])
			{
				case '0':
				case '1':
					/* Fixed code */
					break;
				default:
					modifiableBitCount++;
			}
	    }
	    modifiableBitCount=1<<modifiableBitCount;
		
	    b=0;
	    while (1)
	    {
			U8 needValidation=0;
			U8 validOpcode=1;
			U16 opcode=0;
			
			d=0;
			/* Create instruction code */
			for (c=0;c<16;c++)
			{
				switch (cpu_instructions[a].baseTable[c])
				{
					case '0':
						break;
					case '1':
						opcode|=1<<(15-c);
						break;
					case 'r':
					case 'd':
					case 'a':
					case 'A':
					case 'z':
					case 'm':
					case 'c':
						opcode|=((b&(1<<d))>>d)<<(15-c);
						d++;
						needValidation=1;
						break;
				}
			}
			if (needValidation)
			{
				if (!ValidateOpcode(a,opcode))
				{
					validOpcode=0;
				}
			}
			if (validOpcode)
			{
/*
				EMU_PRINTF("Opcode Coding : %s : %04X %s\n", cpu_instructions[a].opcodeName, opcode,byte_to_binary(opcode));
*/
				if (CPU_JumpTable[opcode]!=CPU_UNKNOWN)
				{
					#if defined _DEBUG
					EMU_PRINTF("[ERR] Cpu Coding For Instruction Overlap\n");
					#endif
					exit(-1);
				}
				CPU_JumpTableIdx[opcode]=cpu_instructions[a].index;
				CPU_JumpTable[opcode]=cpu_instructions[a].opcode;
				CPU_DisTable[opcode]=cpu_instructions[a].decode;
				CPU_Information[opcode]=&cpu_instructions[a];
				numInstructions++;
			}
			b++;
			if (b>=modifiableBitCount)
				break;
	    }
		
	    a++;
	}

	#if defined _DEBUG
	EMU_PRINTF("68000 %d out of %d instructions\n",numInstructions,65536);
	#endif
}

void DumpEmulatorState()
{
    EMU_PRINTF("\n");
	EMU_PRINTF("PC=%08X\n", M68K::cpu_regs.PC);
    EMU_PRINTF("D0=%08X\tD1=%08X\tD2=%08x\tD3=%08x\n",M68K::cpu_regs.D[0],M68K::cpu_regs.D[1],M68K::cpu_regs.D[2],M68K::cpu_regs.D[3]);
    EMU_PRINTF("D4=%08X\tD5=%08X\tD6=%08x\tD7=%08x\n",M68K::cpu_regs.D[4],M68K::cpu_regs.D[5],M68K::cpu_regs.D[6],M68K::cpu_regs.D[7]);
    EMU_PRINTF("A0=%08X\tA1=%08X\tA2=%08x\tA3=%08x\n",M68K::cpu_regs.A[0],M68K::cpu_regs.A[1],M68K::cpu_regs.A[2],M68K::cpu_regs.A[3]);
    EMU_PRINTF("A4=%08X\tA5=%08X\tA6=%08x\tA7=%08x\n",M68K::cpu_regs.A[4],M68K::cpu_regs.A[5],M68K::cpu_regs.A[6],M68K::cpu_regs.A[7]);
    EMU_PRINTF("USP=%08X,ISP=%08x\n",M68K::cpu_regs.USP,M68K::cpu_regs.ISP);
    EMU_PRINTF("\n");
    EMU_PRINTF("          [ T1:T0: S: M:  :I2:I1:I0:  :  :  : X: N: Z: V: C ]\n");
    EMU_PRINTF("SR = %04X [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]\n", M68K::cpu_regs.SR, 
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
    EMU_PRINTF("\n");
}

S8 CPU_signal = -1;

void CPU_SignalInterrupt(S8 level)
{
	if (level > CPU_signal)
	{
		CPU_signal=level;
	}
}

static inline void CPU_CheckForInterrupt()
{
	/* Level 6 */
	/* Frame interrupt */
	if ((M68K::cpu_regs.SR & 0x0700) >= 0x0600)
		return;

	if ((VDP::VDP_Registers[1]&0x20) && CPU_signal==6)	/* Can't do EXACT check here due to instruction */
	{						/* lengths, so move external and flag! */
		CPU_GENERATE_EXCEPTION(0x78);
		M68K::cpu_regs.SR&=0xF8FF;
		M68K::cpu_regs.SR|=0x0600;
		CPU_signal=-1;
		M68K::cpu_regs.stopped=0;
		return;
	}

	/* Level 4 */
	/* Line interrupt */
	if ((M68K::cpu_regs.SR & 0x0700) >= 0x0400)
		return;

	if ((VDP::VDP_Registers[0]&0x10) && CPU_signal==4)
	{
		CPU_GENERATE_EXCEPTION(0x70);
		M68K::cpu_regs.SR&=0xF8FF;
		M68K::cpu_regs.SR|=0x0400;
		CPU_signal=-1;
		M68K::cpu_regs.stopped=0;
		return;
	}
}

U32 CPU_Step()
{
	int cycles = 0;

	CPU_Ins* cpu_ins;

#if DEBUG
	static const int historyMax = 32;
	static CPU_Regs history[historyMax];
	static int historyIdx = 0;
#endif

#if !CPU_COMBINE_STAGES
	if (!M68K::cpu_regs.stage)
#endif
	{
		CPU_CheckForInterrupt();

#if CPU_DEBUG_INFO
		if (M68K::cpu_regs.stopped)
			return 0;
#endif

#if CPU_DEBUG_CALLTRACE
		M68K::cpu_regs.lastInstruction = M68K::cpu_regs.PC;
		M68K::cpu_regs.calltrace[M68K::cpu_regs.calltraceIdx] = M68K::cpu_regs.PC;
		M68K::cpu_regs.calltraceIdx = (M68K::cpu_regs.calltraceIdx + 1) % CPU_CALLTRACE_SIZE;
#endif

#if DEBUG
		history[historyIdx++ % historyMax] = M68K::cpu_regs;
#endif

		/* Fetch next instruction */
		M68K::cpu_regs.opcode = MEM_getWord(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC+=2;

#if CPU_DEBUG_INFO
		assert(CPU_Information[M68K::cpu_regs.opcode]);
#endif

		cpu_ins = CPU_Information[M68K::cpu_regs.opcode];

		if (!cpu_ins)
		{
			DumpEmulatorState();
		}

#if DEBUG
		assert(cpu_ins);
#endif

#if CPU_UNROLL_OPERANDS
#if CPU_DEBUG_INFO
		assert(cpu_ins->numOperands <= CPU_MAX_OPERANDS);
#endif

		M68K::cpu_regs.operands[0] = (M68K::cpu_regs.opcode & cpu_ins->operandMask[0]) >> cpu_ins->operandShift[0];
		M68K::cpu_regs.operands[1] = (M68K::cpu_regs.opcode & cpu_ins->operandMask[1]) >> cpu_ins->operandShift[1];
		M68K::cpu_regs.operands[2] = (M68K::cpu_regs.opcode & cpu_ins->operandMask[2]) >> cpu_ins->operandShift[2];
		M68K::cpu_regs.operands[3] = (M68K::cpu_regs.opcode & cpu_ins->operandMask[3]) >> cpu_ins->operandShift[3];
#else
		for (a = 0; a < cpu_ins->numOperands; a++)
		{
			M68K::cpu_regs.operands[a] = (M68K::cpu_regs.opcode & cpu_ins->operandMask[a]) >> cpu_ins->operandShift[a];
		}
#endif
		
#if CPU_DEBUG_INFO
		if (startDebug)
		{
			U32	insCount;
			U32 b;
			
			EMU_PRINTF("Cycles %d\n",cycles);
			DumpEmulatorState();
			
			insCount=CPU_DisTable[M68K::cpu_regs.opcode](M68K::cpu_regs.PC,M68K::cpu_regs.operands);
			
			EMU_PRINTF("%08X\t",M68K::cpu_regs.PC-2);
			
			for (b=0;b<(insCount+2)/2;b++)
			{
				EMU_PRINTF("%s ",decodeWord(M68K::cpu_regs.PC-2+b*2));
			}
			
			EMU_PRINTF("\t%s\n",M68K::mnemonicData);
			
			startDebug=0;	/* just so i can break after disassembly. */
		}
#endif
	}

#if CPU_USE_JUMP_INDEX_TABLE
	int result = 0;

	const U8 opcodeIdx = CPU_JumpTableIdx[M68K::cpu_regs.opcode];
	switch (opcodeIdx)
	{
		case 0:   result = CPU_STOP(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 1:   result = CPU_MOVETOSR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 2:   result = CPU_ANDSR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 3:   result = CPU_RTE(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 4:   result = CPU_ORSR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 5:   result = CPU_MOVEUSP(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 6:   result = CPU_EORISR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 7:   result = CPU_RESET(M68K::cpu_regs.stage,M68K::cpu_regs.operands); break;
		case 8:   result = CPU_LINEF(M68K::cpu_regs.stage,M68K::cpu_regs.operands); break;
		case 9:   result = CPU_LINEA(M68K::cpu_regs.stage,M68K::cpu_regs.operands); break;
		case 10:  result = CPU_RORm(M68K::cpu_regs.stage,M68K::cpu_regs.operands); break;
		case 11:  result = CPU_ROLm(M68K::cpu_regs.stage,M68K::cpu_regs.operands); break;
		case 12:  result = CPU_ROXRm(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 13:  result = CPU_TRAPV(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 14:  result = CPU_LSLm(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 15:  result = CPU_RTR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 16:  result = CPU_SBCDm(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 17:  result = CPU_MOVEP_m_L(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 18:  result = CPU_ROXLm(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 19:  result = CPU_ABCDm(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 20:  result = CPU_MOVEP_m_W(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 21:  result = CPU_MOVEP_L_m(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 22:  result = CPU_MOVEP_W_m(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 23:  result = CPU_ABCD(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 24:  result = CPU_SBCD(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 25:  result = CPU_NEGX(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 26:  result = CPU_ORICCR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 27:  result = CPU_ANDICCR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 28:  result = CPU_ASRm(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 29:  result = CPU_ASLm(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 30:  result = CPU_SUBX(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 41:  result = CPU_MOVEFROMSR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 42:  result = CPU_DIVS(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 43:  result = CPU_ADDX(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 44:  result = CPU_TRAP(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 45:  result = CPU_MOVETOCCR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 46:  result = CPU_ROXR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 47:  result = CPU_ROXL(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 48:  result = CPU_EORICCR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 49:  result = CPU_EORI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 50:  result = CPU_LSRm(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 51:  result = CPU_BCHGI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 52:  result = CPU_BCHG(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 53:  result = CPU_NOP(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 54:  result = CPU_ROR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 55:  result = CPU_ROL(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 56:  result = CPU_BTST(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 57:  result = CPU_EORd(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 58:  result = CPU_BCLR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 59:  result = CPU_DIVU(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 60:  result = CPU_ASR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 61:  result = CPU_ASL(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 62:  result = CPU_ORI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 63:  result = CPU_ANDd(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 64:  result = CPU_ORs(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 65:  result = CPU_UNLK(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 66:  result = CPU_ADDd(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 67:  result = CPU_CMPM(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 68:  result = CPU_LINK(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 69:  result = CPU_PEA(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 70:  result = CPU_SCC(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 71:  result = CPU_NEG(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 72:  result = CPU_MULS(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 73:  result = CPU_EXT(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 74:  result = CPU_ADDI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 75:  result = CPU_LSL(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 76:  result = CPU_MULU(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 77:  result = CPU_BSETI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 78:  result = CPU_BSET(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 79:  result = CPU_SUBd(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 80:  result = CPU_ANDs(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 81:  result = CPU_BCLRI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 82:  result = CPU_JSR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 83:  result = CPU_EXG(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 84:  result = CPU_ANDI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 85:  result = CPU_CLEAR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 86:  result = CPU_ADDQ(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 87:  result = CPU_ORd(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 88:  result = CPU_RTS(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 89:  result = CPU_BSR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 90:  result = CPU_SUBI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 91:  result = CPU_MOVEMd(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 92:  result = CPU_MOVEMs(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 93:  result = CPU_SWAP(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 94:  result = CPU_LSR(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 95:  result = CPU_SUBs(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 96:  result = CPU_MOVEQ(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 97:  result = CPU_JMP(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 98:  result = CPU_TST(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 99:  result = CPU_ADDA(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 100: result = CPU_SUBA(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 101: result = CPU_NOT(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 102: result = CPU_ADDs(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 103: result = CPU_BTSTI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 104: result = CPU_BRA(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 105: result = CPU_DBCC(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 106: result = CPU_MOVEA(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 107: result = CPU_CMPI(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 108: result = CPU_CMP(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 109: result = CPU_CMPA(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 110: result = CPU_BCC(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 111: result = CPU_SUBQ(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 112: result = CPU_MOVE(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		case 113: result = CPU_LEA(M68K::cpu_regs.stage, M68K::cpu_regs.operands); break;
		default:
			//Error
			break;
	}

#if CPU_COMBINE_STAGES
	cycles = result;
#else
	M68K::cpu_regs.stage = result;
#endif

#else
	M68K::cpu_regs.stage = CPU_JumpTable[M68K::cpu_regs.opcode](M68K::cpu_regs.stage,M68K::cpu_regs.operands);
#endif

	return cycles;
}
