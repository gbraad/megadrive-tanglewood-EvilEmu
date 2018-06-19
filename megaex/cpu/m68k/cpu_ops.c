/*
 *  cpu_ops.c

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

#include "cpu_ops.h"

#include "gui/debugger.h"
#include "cpu.h"
#include "memory.h"

typedef U32(*Address_Handler)(U32, U32, U32, U32*, U16, int, int);

/*New support functionality for cpu with stage and bus arbitration */

U32 BUS_Available(U32 ea)			/* TODO - Need to implement bus arbitration */
{
	UNUSED_ARGUMENT(ea);
	return 1;
}

/* Applies a fudge to correct the cycle timings for some odd instructions (JMP) */
U32 FUDGE_EA_CYCLES(U32 endCase,U32 offs,U32 stage,U16 operand)
{
	switch (operand)
	{
	case 0x00:        /* Dx */
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		break;
	case 0x08:        /* Ax */
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
		break;
	case 0x10:        /* (Ax) */
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		switch (stage)
		{
		case 0:
			return 1 + offs;
		case 1:
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 0x18:        /* (Ax)+ */
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		break;
	case 0x20:        /* -(Ax) */
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		break;
	case 0x28:        /* (XXXX,Ax) */
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
		switch (stage)
		{
		case 0:
			return 1 + offs;
		case 1:
			break;
		}
		break;
	case 0x30:        /* (XX,Ax,Xx) */
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		switch (stage)
		{
		case 0:
			return 1 + offs;
		case 1:
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 0x38:        /* (XXXX).W */
		switch (stage)
		{
		case 0:
			return 1 + offs;
		case 1:
			break;
		}
		break;
	case 0x39:        /* (XXXXXXXX).L */
		break;
	case 0x3A:        /* (XXXX,PC) */
		switch (stage)
		{
		case 0:
			return 1 + offs;
		case 1:
			break;
		}
		break;
	case 0x3B:        /* (XX,PC,Xx) */
		switch (stage)
		{
		case 0:
			return 1 + offs;
		case 1:
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 0x3C:        /* #XX.B #XXXX.W #XXXXXXXX.L */
		break;
	}

	return endCase;
}

int COMPUTE_CONDITION(U16 op)
{
    switch (op)
    {
		case 0x00:
			return 1;
		case 0x01:
			return 0;
		case 0x02:
			return ((~cpu_regs.SR)&CPU_STATUS_C) && ((~cpu_regs.SR)&CPU_STATUS_Z);
		case 0x03:
			return (cpu_regs.SR & CPU_STATUS_C) || (cpu_regs.SR & CPU_STATUS_Z);
		case 0x04:
			return !(cpu_regs.SR & CPU_STATUS_C);
		case 0x05:
			return (cpu_regs.SR & CPU_STATUS_C);
		case 0x06:
			return !(cpu_regs.SR & CPU_STATUS_Z);
		case 0x07:
			return (cpu_regs.SR & CPU_STATUS_Z);
		case 0x08:
			return !(cpu_regs.SR & CPU_STATUS_V);
		case 0x09:
			return (cpu_regs.SR & CPU_STATUS_V);
		case 0x0A:
			return !(cpu_regs.SR & CPU_STATUS_N);
		case 0x0B:
			return (cpu_regs.SR & CPU_STATUS_N);
		case 0x0C:
			return ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V)) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)));
		case 0x0D:
			return ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
		case 0x0E:
			return ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V) && (!(cpu_regs.SR & CPU_STATUS_Z))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)) && (!(cpu_regs.SR & CPU_STATUS_Z)));
		case 0x0F:
			return (cpu_regs.SR & CPU_STATUS_Z) || ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
    }
	return 0;
}

#define MAX_VALUE_HANDLERS 0x4
#define MAX_VALUE_STAGES   0x5

typedef U32(*Value_Handler)(U32, U32, U32, U16, U32*, U32*, int);

U32 Value_Handler_Byte_0(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	if (!BUS_Available(*eas))
		return 0 + offs;
	*ead = MEM_getByte(*eas);
	return 1 + offs;
}

U32 Value_Handler_Byte_1(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	return 2 + offs;
}

U32 Value_Handler_Word_0(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	if (!BUS_Available(*eas))
		return 0 + offs;
	*ead = MEM_getWord(*eas);
	return 1 + offs;
}

U32 Value_Handler_Word_1(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	return 2 + offs;
}

U32 Value_Handler_Long_0(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	cpu_regs.tmpL = *eas;
	if (!BUS_Available(cpu_regs.tmpL))
		return 0 + offs;
	*ead = MEM_getWord(cpu_regs.tmpL) << 16;
	return 1 + offs;
}

U32 Value_Handler_Long_1(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	cpu_regs.tmpL += 2;
	return 2 + offs;
}

U32 Value_Handler_Long_2(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	if (!BUS_Available(cpu_regs.tmpL))
		return 2 + offs;
	*ead |= MEM_getWord(cpu_regs.tmpL);
	return 3 + offs;
}

U32 Value_Handler_Long_3(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	return 4 + offs;
}

U32 Value_Handler_EndCase(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
	return endCase;
}

Value_Handler Value_Handler_Table[MAX_VALUE_HANDLERS][MAX_VALUE_STAGES] =
{
	{	// Byte fetch, stages 0 - 4
		Value_Handler_Byte_0,
		Value_Handler_Byte_1,
		Value_Handler_EndCase,
		Value_Handler_EndCase,
		Value_Handler_EndCase,
	},
	
	{	// Word fetch, stages 0 - 4
		Value_Handler_Word_0,
		Value_Handler_Word_1,
		Value_Handler_EndCase,
		Value_Handler_EndCase,
		Value_Handler_EndCase,
	},
	
	{	// Doesn't exist
		Value_Handler_EndCase,
		Value_Handler_EndCase,
		Value_Handler_EndCase,
		Value_Handler_EndCase,
		Value_Handler_EndCase,
	},
	
	{	// Long fetch, stages 0 - 4
		Value_Handler_Long_0,
		Value_Handler_Long_1,
		Value_Handler_Long_2,
		Value_Handler_Long_3,
		Value_Handler_EndCase,
	}
};

U32 LOAD_EFFECTIVE_VALUE(U32 endCase,U32 offs,U32 stage,U16 op,U32 *ead,U32 *eas,int length)
{
	if (op<0x10)
	{
		*ead=*eas;
		return endCase;
	}

	return Value_Handler_Table[length-1][stage](endCase, offs, stage, op, ead, eas, length);
}

U32 STORE_EFFECTIVE_VALUE(U32 endCase,U32 offs,U32 stage,U16 op,U32 *ead,U32 *eas)
{
	if (op<0x08)
	{
		cpu_regs.D[op]&=cpu_regs.iMask;
		cpu_regs.D[op]|=(*eas)&cpu_regs.zMask;
		return endCase;
	}
	if (op<0x10)
	{
		cpu_regs.A[op-0x08]&=cpu_regs.iMask;
		cpu_regs.A[op-0x08]|=(*eas)&cpu_regs.zMask;
		return endCase;
	}
	switch (cpu_regs.len)
	{
		case 1:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(*ead))
						return 0+offs;
					MEM_setByte(*ead,(*eas)&cpu_regs.zMask);
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 2:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(*ead))
						return 0+offs;
					MEM_setWord(*ead,(*eas)&cpu_regs.zMask);
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 4:
			switch (stage)
			{
				case 0:
					cpu_regs.tmpL=*ead;
					if (!BUS_Available(cpu_regs.tmpL))
						return 0+offs;
					MEM_setWord(cpu_regs.tmpL,((*eas)&cpu_regs.zMask)>>16);
					return 1+offs;
				case 1:
					cpu_regs.tmpL+=2;
					return 2+offs;
				case 2:
					if (!BUS_Available(cpu_regs.tmpL))
						return 2+offs;
					MEM_setWord(cpu_regs.tmpL,(*eas)&cpu_regs.zMask);
					return 3+offs;
				case 3:
					return 4+offs;
				case 4:
					break;
			}
			break;
	}
	return endCase;
}

U32 PUSH_VALUE(U32 endCase,U32 offs,U32 stage,U32 val,int length)
{
	switch (length)
	{
		case 2:
			switch (stage)
			{
				case 0:
					cpu_regs.A[7]-=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(cpu_regs.A[7]))
						return 1+offs;
					MEM_setWord(cpu_regs.A[7],val);
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 4:
			switch (stage)
			{
				case 0:
					cpu_regs.A[7]-=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(cpu_regs.A[7]))
						return 1+offs;
					MEM_setWord(cpu_regs.A[7],val&0xFFFF);
					return 2+offs;
				case 2:
					cpu_regs.A[7]-=2;
					return 3+offs;
				case 3:
					if (!BUS_Available(cpu_regs.A[7]))
						return 3+offs;
					MEM_setWord(cpu_regs.A[7],val>>16);
					return 4+offs;
				case 4:
					break;
			}
			break;
	}
	return endCase;
}

U32 POP_VALUE(U32 endCase,U32 offs,U32 stage,U32 *val,int length)
{
	switch (length)
	{
		case 2:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(cpu_regs.A[7]))
						return 0+offs;
					*val = MEM_getWord(cpu_regs.A[7]);
					return 1+offs;
				case 1:
					cpu_regs.A[7]+=2;
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 4:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(cpu_regs.A[7]))
						return 0+offs;
					*val = MEM_getWord(cpu_regs.A[7])<<16;
					return 1+offs;
				case 1:
					cpu_regs.A[7]+=2;
					return 2+offs;
				case 2:
					if (!BUS_Available(cpu_regs.A[7]))
						return 2+offs;
					*val |= MEM_getWord(cpu_regs.A[7]);
					return 3+offs;
				case 3:
					cpu_regs.A[7]+=2;
					return 4+offs;
				case 4:
					break;
			}
			break;
	}
	return endCase;
}

void OPCODE_SETUP_LENGTHM(U16 op)
{
    switch(op)
    {
		case 0x01:
			cpu_regs.iMask=0xFFFFFF00;
			cpu_regs.nMask=0x80;
			cpu_regs.zMask=0xFF;
			cpu_regs.len=1;
			break;
		case 0x03:
			cpu_regs.iMask=0xFFFF0000;
			cpu_regs.nMask=0x8000;
			cpu_regs.zMask=0xFFFF;
			cpu_regs.len=2;
			break;
		case 0x02:
			cpu_regs.iMask=0x00000000;
			cpu_regs.nMask=0x80000000;
			cpu_regs.zMask=0xFFFFFFFF;
			cpu_regs.len=4;
			break;
	}
}

void OPCODE_SETUP_LENGTH(U16 op)
{
    switch(op)
    {
		case 0x00:
			cpu_regs.iMask=0xFFFFFF00;
			cpu_regs.nMask=0x80;
			cpu_regs.zMask=0xFF;
			cpu_regs.len=1;
			break;
		case 0x01:
			cpu_regs.iMask=0xFFFF0000;
			cpu_regs.nMask=0x8000;
			cpu_regs.zMask=0xFFFF;
			cpu_regs.len=2;
			break;
		case 0x02:
			cpu_regs.iMask=0x00000000;
			cpu_regs.nMask=0x80000000;
			cpu_regs.zMask=0xFFFFFFFF;
			cpu_regs.len=4;
			break;
    }
}

void OPCODE_SETUP_LENGTHLW(U16 op)
{
    switch(op)
    {
		case 0x00:
			cpu_regs.iMask=0xFFFF0000;
			cpu_regs.nMask=0x8000;
			cpu_regs.zMask=0xFFFF;
			cpu_regs.len=2;
			break;
		case 0x01:
			cpu_regs.iMask=0x00000000;
			cpu_regs.nMask=0x80000000;
			cpu_regs.zMask=0xFFFFFFFF;
			cpu_regs.len=4;
			break;
    }
}

void COMPUTE_Z_BIT(U32 eas,U32 ead)
{
	if (ead & eas)
		cpu_regs.SR&=~CPU_STATUS_Z;
	else
		cpu_regs.SR|=CPU_STATUS_Z;
}

void COMPUTE_ZN_TESTS(U32 ea)
{
	if (ea & cpu_regs.nMask)
		cpu_regs.SR|=CPU_STATUS_N;
	else
		cpu_regs.SR&=~CPU_STATUS_N;
	if (ea & cpu_regs.zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
	else
		cpu_regs.SR|=CPU_STATUS_Z;
}

void COMPUTE_ADD_XCV_TESTS(U32 eas,U32 ead,U32 ear)
{
	ear&=cpu_regs.nMask;
	eas&=cpu_regs.nMask;
	ead&=cpu_regs.nMask;
	
	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
}

void COMPUTE_SUB_XCV_TESTS(U32 eas,U32 ead,U32 ear)
{
	ear&=cpu_regs.nMask;
	eas&=cpu_regs.nMask;
	ead&=cpu_regs.nMask;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
}

void CPU_CHECK_SP(U16 old,U16 new)
{
	if (old & CPU_STATUS_S)
	{
		if (!(new & CPU_STATUS_S))
		{
			cpu_regs.ISP = cpu_regs.A[7];
			cpu_regs.A[7] = cpu_regs.USP;
		}
	}
	else
	{
		if (new & CPU_STATUS_S)
		{
			cpu_regs.USP = cpu_regs.A[7];
			cpu_regs.A[7]=cpu_regs.ISP;
		}
	}
}

/* No end case on exception because its final act will be to return 0 */
U32 PROCESS_EXCEPTION(U32 offs,U32 stage,U32 vector)
{
	switch (stage)
	{
		case 0:
			cpu_regs.tmpW = cpu_regs.SR;
			return 1+offs;
		case 1:
			cpu_regs.SR|=CPU_STATUS_S;
			CPU_CHECK_SP(cpu_regs.tmpW,cpu_regs.SR);
			return 2+offs;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			return offs+PUSH_VALUE(7, 2, stage-2, cpu_regs.PC-2,4);
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			return offs+PUSH_VALUE(12,7, stage-7, cpu_regs.SR, 2);
		case 12:
			cpu_regs.tmpL=vector;
			return 13+offs;
		case 13:
			if (!BUS_Available(cpu_regs.tmpL))
				return 13+offs;
			cpu_regs.PC=MEM_getWord(cpu_regs.tmpL)<<16;
			return 14+offs;
		case 14:
			cpu_regs.tmpL+=2;
			return 15+offs;
		case 15:
			if (!BUS_Available(cpu_regs.tmpL))
				return 15+offs;
			cpu_regs.PC|=MEM_getWord(cpu_regs.tmpL);
			return 16+offs;
		case 16:
			return 17+offs;
		case 17:
			break;
	}

	return 0;
}

/* OLD SUPPORT TO BE REMOVED WHEN ALL INSTRUCTIONS WORKING */

U32 getEffectiveAddress(U16 operand,int length)
{
	U16 tmp;
    U32 ea=0;
    switch (operand)
    {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			ea = cpu_regs.D[operand];
			break;
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			ea = cpu_regs.A[operand-0x08];
			break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			ea = cpu_regs.A[operand-0x10];
			break;
		case 0x18:
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
			if ((operand==0x1F) && (length==1))
			{
				DEB_PauseEmulation(DEB_Mode_68000,"Byte size post decrement to stack");
				length=2;
			}
			ea = cpu_regs.A[operand-0x18];
			cpu_regs.A[operand-0x18]+=length;
			break;
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			if ((operand==0x27) && (length==1))
			{
				DEB_PauseEmulation(DEB_Mode_68000,"Byte size preincrement to stack");
				length=2;
			}
			cpu_regs.A[operand-0x20]-=length;
			ea = cpu_regs.A[operand-0x20];
			break;
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			ea = cpu_regs.A[operand-0x28] + (S16)MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x30:	/* 110rrr */
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			tmp = MEM_getWord(cpu_regs.PC);
			if (tmp&0x8000)
			{
				ea = cpu_regs.A[(tmp>>12)&0x7];
				if (!(tmp&0x0800))
					ea=(S16)ea;
				ea += (S8)(tmp&0xFF);
				ea += cpu_regs.A[operand-0x30];
			}
			else
			{
				ea = cpu_regs.D[(tmp>>12)];
				if (!(tmp&0x0800))
					ea=(S16)ea;
				ea += (S8)(tmp&0xFF);
				ea += cpu_regs.A[operand-0x30];
			}
			cpu_regs.PC+=2;
			break;
		case 0x38:		/* 111000 */
			ea = (S16)MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x39:		/* 111001 */
			ea = MEM_getLong(cpu_regs.PC);
			cpu_regs.PC+=4;
			break;
		case 0x3A:		/* 111010 */
			ea = cpu_regs.PC+(S16)MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x3B:		/* 111100 */
			tmp = MEM_getWord(cpu_regs.PC);
			if (tmp&0x8000)
			{
				ea = cpu_regs.A[(tmp>>12)&0x7];
				if (!(tmp&0x0800))
					ea=(S16)ea;
				ea += (S8)(tmp&0xFF);
				ea += cpu_regs.PC;
			}
			else
			{
				ea = cpu_regs.D[(tmp>>12)];
				if (!(tmp&0x0800))
					ea=(S16)ea;
				ea += (S8)(tmp&0xFF);
				ea += cpu_regs.PC;
			}
			cpu_regs.PC+=2;
			break;
		case 0x3C:		/* 111100 */
			switch (length)
	    {
			case 1:
				ea = MEM_getByte(cpu_regs.PC+1);
				cpu_regs.PC+=2;
				break;
			case 2:
				ea = MEM_getWord(cpu_regs.PC);
				cpu_regs.PC+=2;
				break;
			case 4:
				ea = MEM_getLong(cpu_regs.PC);
				cpu_regs.PC+=4;
				break;
	    }
			break;
		default:
			printf("[ERR] Unsupported effective addressing mode : %04X\n", operand);
			SOFT_BREAK;
			break;
    }
    return ea;
}

U32 getSourceEffectiveAddress(U16 operand,int length)
{
	U16 opt;
	U32 eas;

	eas = getEffectiveAddress(operand,length);
	opt=operand&0x38;
	if ( (opt < 0x10) || (operand == 0x3C) )
	{
	}
	else
	{
		switch (length)
		{
			case 1:
				return MEM_getByte(eas);
				break;
			case 2:
				return MEM_getWord(eas);
				break;
			case 4:
				return MEM_getLong(eas);
				break;
		}
	}
	return eas;
}

void CPU_GENERATE_EXCEPTION(U32 exceptionAddress)
{
	U16 oldSR;
	
	oldSR=cpu_regs.SR;
	cpu_regs.SR|=CPU_STATUS_S;
	CPU_CHECK_SP(oldSR,cpu_regs.SR);
		
	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],cpu_regs.PC);
	cpu_regs.A[7]-=2;
	MEM_setWord(cpu_regs.A[7],oldSR);

	cpu_regs.PC=MEM_getLong(exceptionAddress);
}

/*---------------------------------------------------------*/
