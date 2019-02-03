/*
 *  z80_dis.c

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

#include "z80.h"
#include "z80_dis.h"

static char mnemonicData_z80[256];

U8 Z80_MEM_getByte(U16 address);

const char *decodeRP(U16 rp)
{
	switch(rp)
	{
	case 0:
		return "BC";
	case 1:
		return "DE";
	case 2:
		if (Z80::Z80_regs.ixDisAdjust)
			return "IX";
		if (Z80::Z80_regs.iyDisAdjust)
			return "IY";
		return "HL";
	case 3:
		return "SP";
	}
	return "";
}

const char *decodeRPAF(U16 rp)
{
	switch(rp)
	{
	case 0:
		return "BC";
	case 1:
		return "DE";
	case 2:
		if (Z80::Z80_regs.ixDisAdjust)
			return "IX";
		if (Z80::Z80_regs.iyDisAdjust)
			return "IY";
		return "HL";
	case 3:
		return "AF";
	}
	return "";
}

const char *decodeR(U16 r,int IXIYPossible)
{
	switch(r)
	{
	case 0:
		return "B";
	case 1:
		return "C";
	case 2:
		return "D";
	case 3:
		return "E";
	case 4:
		if (Z80::Z80_regs.ixDisAdjust && IXIYPossible)
			return "IXh";
		if (Z80::Z80_regs.iyDisAdjust && IXIYPossible)
			return "IYh";
		return "H";
	case 5:
		if (Z80::Z80_regs.ixDisAdjust && IXIYPossible)
			return "IXl";
		if (Z80::Z80_regs.iyDisAdjust && IXIYPossible)
			return "IYl";
		return "L";
	case 6:
		return "F";																	/* should only be seen rarely */
	case 7:
		return "A";
	}
	return "unknown";
}

int _HL_adjust()
{
	if (Z80::Z80_regs.ixDisAdjust || Z80::Z80_regs.iyDisAdjust)
		return 1;
	return 0;
}

const char *decode_HL_(U32 adr)
{
	static char tmp[32];

	if (Z80::Z80_regs.ixDisAdjust)
	{
		sprintf(tmp,"IX+%02X",(S8)Z80_MEM_getByte(adr));
		return tmp;
	}
	if (Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(tmp,"IY+%02X",(S8)Z80_MEM_getByte(adr));
		return tmp;
	}
	return "HL";
}

const char *decodeHL()
{
	if (Z80::Z80_regs.ixDisAdjust)
	{
		return "IX";
	}
	if (Z80::Z80_regs.iyDisAdjust)
	{
		return "IY";
	}
	return "HL";
}

const char *decodeCC(U16 cc)
{
	switch(cc)
	{
	case 0:
		return "NZ";
	case 1:
		return "Z";
	case 2:
		return "NC";
	case 3:
		return "C";
	case 4:
		return "PO";
	case 5:
		return "PE";
	case 6:
		return "P";
	case 7:
		return "M";
	}
	return "";
}

U16 Z80_DIS_UNKNOWN(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	strcpy(mnemonicData_z80,"Unknown Instruction");
	
	return 0;
}

U16 Z80_DIS_DI(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"DI");
	
	return 0;
}

U16 Z80_DIS_LDRP(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD %s,%02X%02X",decodeRP(operands[0]),Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_ED_IM(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"IM %d",operands[0] ? operands[0]-1 : operands[0]);

	return 0;
}

U16 Z80_DIS_JP(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"JP %02X%02X",Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_CALL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CALL %02X%02X",Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_LDn(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD %s,%02X",decodeR(operands[0],1),Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_XOR(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"XOR %s",decodeR(operands[0],1));

	return 0;
}

U16 Z80_DIS_CB_BITm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"BIT %d,(%s)",operands[0],decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_JPcc(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"JP %s,%02X%02X",decodeCC(operands[0]),Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_LDm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD (%s),%s",decode_HL_(adr),decodeR(operands[0],0));

	return _HL_adjust();
}

U16 Z80_DIS_LD_nn_A(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD (%02X%02X),A",Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_RET(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RET");

	return 0;
}

U16 Z80_DIS_INC(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"INC %s",decodeR(operands[0],1));

	return 0;
}

U16 Z80_DIS_LDA_nn(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD A,(%02X%02X)",Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_OR(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"OR %s",decodeR(operands[0],1));

	return 0;
}

U16 Z80_DIS_ED_LDIR(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LDIR");

	return 0;
}

U16 Z80_DIS_LDrr(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD %s,%s",decodeR(operands[0],1),decodeR(operands[1],1));

	return 0;
}

U16 Z80_DIS_DEC(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"DEC %s",decodeR(operands[0],1));

	return 0;
}

U16 Z80_DIS_RLCA(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RLCA");

	return 0;
}

U16 Z80_DIS_RRCA(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RRCA");

	return 0;
}

U16 Z80_DIS_ORA_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"OR %02X",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_LD_HL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD %s,(%s)",decodeR(operands[0],0),decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_ADDA_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"ADD A,%02X",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_ADCA_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"ADC A,%02X",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_INCRP(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"INC %s",decodeRP(operands[0]));

	return 0;
}

U16 Z80_DIS_DECRP(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"DEC %s",decodeRP(operands[0]));

	return 0;
}

U16 Z80_DIS_NOP(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"NOP");

	return 0;
}

U16 Z80_DIS_LDSP_HL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD SP,%s",decodeHL());

	return 0;
}

U16 Z80_DIS_ED_LDI_A(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD I,A");

	return 0;
}

U16 Z80_DIS_ED_LDR_A(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD R,A");

	return 0;
}

U16 Z80_DIS_POPRP(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"POP %s",decodeRPAF(operands[0]));

	return 0;
}

U16 Z80_DIS_EXAF(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"EX AF,AF'");

	return 0;
}	

U16 Z80_DIS_EXX(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"EXX");

	return 0;
}

U16 Z80_DIS_LDHL_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD (%s),%02X",decode_HL_(adr),Z80_MEM_getByte(adr+_HL_adjust()));

	return 1+_HL_adjust();
}

U16 Z80_DIS_JPHL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"JP (%s)",decodeHL());

	return 0;
}

U16 Z80_DIS_CP(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CP %s",decodeR(operands[0],1));

	return 0;
}

U16 Z80_DIS_RRA(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RRA");

	return 0;
}

U16 Z80_DIS_RLA(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RLA");

	return 0;
}

U16 Z80_DIS_ANDA_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"AND %02X",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_CB_SLA(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust || Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"SLA (%s),%s",decode_HL_(adr-2),decodeR(operands[0],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"SLA %s",decodeR(operands[0],0));
	}

	return 0;
}

U16 Z80_DIS_ADDHL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"ADD %s,%s",decodeHL(),decodeRP(operands[0]));

	return 0;
}

U16 Z80_DIS_DJNZ(U32 adr,U16* operands)
{
	U32 relJump = adr +1 + (S8)Z80_MEM_getByte(adr);
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"DJNZ %04X",relJump);

	return 1;
}

U16 Z80_DIS_CPHL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CP (%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_INCHL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"INC (%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_JR(U32 adr,U16* operands)
{
	U32 relJump = adr +1 + (S8)Z80_MEM_getByte(adr);
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"JR %04X",relJump);

	return 1;
}

U16 Z80_DIS_SUBA_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SUB %02X",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_CP_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CP %02X",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_JRcc(U32 adr,U16* operands)
{
	U32 relJump = adr +1 + (S8)Z80_MEM_getByte(adr);
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"JR %s,%04X",decodeCC(operands[0]),relJump);

	return 1;
}

U16 Z80_DIS_PUSHRP(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"PUSH %s",decodeRPAF(operands[0]));

	return 0;
}

U16 Z80_DIS_CB_BIT(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust||Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"BIT %d,(%s)",operands[0],decode_HL_(adr-2));
	}
	else
	{
		sprintf(mnemonicData_z80,"BIT %d,%s",operands[0],decodeR(operands[1],0));
	}

	return 0;
}

U16 Z80_DIS_LDA_DE(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD A,(DE)");

	return 0;
}

U16 Z80_DIS_CB_SRL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust || Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"SRL (%s),%s",decode_HL_(adr-2),decodeR(operands[0],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"SRL %s",decodeR(operands[0],0));
	}

	return 0;
}

U16 Z80_DIS_EI(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"EI");
	
	return 0;
}

U16 Z80_DIS_ADDA_r(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"ADD A,%s",decodeR(operands[0],1));
	
	return 0;
}

U16 Z80_DIS_RST(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RST %02X",operands[0]<<3);
	
	return 0;
}

U16 Z80_DIS_LDDE_A(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD (DE),A");

	return 0;
}

U16 Z80_DIS_OUTn_A(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"OUT (%02X),A",Z80_MEM_getByte(adr));
	
	return 1;
}

U16 Z80_DIS_AND(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"AND %s",decodeR(operands[0],1));

	return 0;
}

U16 Z80_DIS_ED_SBCHL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SBC HL,%s",decodeRP(operands[0]));

	return 0;
}

U16 Z80_DIS_DECHL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"DEC (%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_ED_LDnn_dd(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD (%02X%02X),%s",Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr),decodeRP(operands[0]));

	return 2;
}

U16 Z80_DIS_LDnn_HL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD (%02X%02X),%s",Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr),decodeHL());

	return 2;
}

U16 Z80_DIS_EXDE_HL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"EX DE,HL");

	return 0;
}

U16 Z80_DIS_ED_LDDR(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LDDR");

	return 0;
}

U16 Z80_DIS_LDHL_nn(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD %s,(%02X%02X)",decodeHL(),Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_CB_SETm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SET %d,(%s)",operands[0],decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_CB_RESm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RES %d,(%s)",operands[0],decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_SUBA_r(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SUB A,%s",decodeR(operands[0],1));
	
	return 0;
}

U16 Z80_DIS_RETcc(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RET %s",decodeCC(operands[0]));

	return 0;
}

U16 Z80_DIS_SCF(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SCF");

	return 0;
}

U16 Z80_DIS_XORm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"XOR (%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_CCF(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CCF");

	return 0;
}

U16 Z80_DIS_ADDm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"ADD A,(%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_EXSP_HL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"EX (SP),%s",decodeHL());

	return 0;
}

U16 Z80_DIS_ED_LDdd_nn(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD %s,(%02X%02X)",decodeRP(operands[0]),Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_ADCA_r(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"ADC A,%s",decodeR(operands[0],1));
	
	return 0;
}

U16 Z80_DIS_CPL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CPL");

	return 0;
}

U16 Z80_DIS_XORA_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"XOR %02X",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_HALT(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"HALT");

	return 0;
}

U16 Z80_DIS_ED_INr_C(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"IN %s,(C)",decodeR(operands[0],0));
	
	return 0;
}

U16 Z80_DIS_CB_RLC(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust || Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"RLC (%s),%s",decode_HL_(adr-2),decodeR(operands[0],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"RLC %s",decodeR(operands[0],0));
	}

	return 0;
}

U16 Z80_DIS_CALLcc(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CALL %s,%02X%02X",decodeCC(operands[0]),Z80_MEM_getByte(adr+1),Z80_MEM_getByte(adr));

	return 2;
}

U16 Z80_DIS_SBCA_r(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SBC A,%s",decodeR(operands[0],1));
	
	return 0;
}

U16 Z80_DIS_CB_RES(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust||Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"RES %d,(%s),%s",operands[0],decode_HL_(adr-2),decodeR(operands[1],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"RES %d,%s",operands[0],decodeR(operands[1],0));
	}

	return 0;
}

U16 Z80_DIS_CB_SET(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust||Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"SET %d,(%s),%s",operands[0],decode_HL_(adr-2),decodeR(operands[1],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"SET %d,%s",operands[0],decodeR(operands[1],0));
	}

	return 0;
}

U16 Z80_DIS_SUBm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SUB A,(%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_INA_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"IN A,(%02X)",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_ANDm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"AND (%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_ORm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"OR (%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_CB_RL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust || Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"RL (%s),%s",decode_HL_(adr-2),decodeR(operands[0],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"RL %s",decodeR(operands[0],0));
	}

	return 0;
}

U16 Z80_DIS_ADCm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"ADC A,(%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_CB_RRC(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust || Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"RRC (%s),%s",decode_HL_(adr-2),decodeR(operands[0],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"RRC %s",decodeR(operands[0],0));
	}

	return 0;
}

U16 Z80_DIS_CB_RR(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust || Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"RR (%s),%s",decode_HL_(adr-2),decodeR(operands[0],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"RR %s",decodeR(operands[0],0));
	}

	return 0;
}

U16 Z80_DIS_ED_ADCHL(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"ADC HL,%s",decodeRP(operands[0]));

	return 0;
}

U16 Z80_DIS_CB_RLm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RL (%s)",decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_CB_SRA(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (Z80::Z80_regs.ixDisAdjust || Z80::Z80_regs.iyDisAdjust)
	{
		sprintf(mnemonicData_z80,"SRA (%s),%s",decode_HL_(adr-2),decodeR(operands[0],0));
	}
	else
	{
		sprintf(mnemonicData_z80,"SRA %s",decodeR(operands[0],0));
	}

	return 0;
}

U16 Z80_DIS_ED_NEG(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"NEG");

	return 0;
}

U16 Z80_DIS_LDBC_A(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD (BC),A");

	return 0;
}

U16 Z80_DIS_ED_LDI(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LDI");

	return 0;
}

U16 Z80_DIS_ED_RLD(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RLD");

	return 0;
}

U16 Z80_DIS_ED_LDA_R(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD A,R");

	return 0;
}

U16 Z80_DIS_DAA(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"DAA");

	return 0;
}

U16 Z80_DIS_ED_CPIR(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CPIR");

	return 0;
}

U16 Z80_DIS_ED_OUTC_r(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	if (operands[0]==6)
	{
		sprintf(mnemonicData_z80,"OUT (C),0");
	}
	else
	{
		sprintf(mnemonicData_z80,"OUT (C),%s",decodeR(operands[0],0));
	}
	
	return 0;
}

U16 Z80_DIS_LDA_BC(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD A,(BC)");

	return 0;
}

U16 Z80_DIS_SBCA_n(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SBC %02X",Z80_MEM_getByte(adr));

	return 1;
}

U16 Z80_DIS_CB_RLCm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RLC (%s)",decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_CB_RRm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RR (%s)",decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_CB_SLAm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SLA (%s)",decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_CB_RRCm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RLC (%s)",decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_CB_SRLm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SRL (%s)",decode_HL_(adr-2));

	return 0;
}

U16 Z80_DIS_ED_RETI(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RETI");

	return 0;
}

U16 Z80_DIS_ED_OUTD(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"OUTD");

	return 0;
}

U16 Z80_DIS_ED_LDA_I(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LD A,I");

	return 0;
}

U16 Z80_DIS_ED_LDD(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"LDD");

	return 0;
}

U16 Z80_DIS_ED_OTDR(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"OTDR");

	return 0;
}

U16 Z80_DIS_ED_INI(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"INI");

	return 0;
}

U16 Z80_DIS_ED_RRD(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RRD");

	return 0;
}

U16 Z80_DIS_SBCm(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"SBC A,(%s)",decode_HL_(adr));

	return _HL_adjust();
}

U16 Z80_DIS_ED_OUTI(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"OUTI");

	return 0;
}

U16 Z80_DIS_ED_OTIR(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"OTIR");

	return 0;
}

U16 Z80_DIS_ED_CPI(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"CPI");

	return 0;
}

U16 Z80_DIS_ED_RETN(U32 adr,U16* operands)
{
	UNUSED_ARGUMENT(adr);
	
	
	
	
	
	
	
	
	sprintf(mnemonicData_z80,"RETN");

	return 0;
}


