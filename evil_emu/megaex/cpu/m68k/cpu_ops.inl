
/*New support functionality for cpu with stage and bus arbitration */

static inline U32 BUS_Available(U32 ea)			/* TODO - Need to implement bus arbitration */
{
	UNUSED_ARGUMENT(ea);
	return 1;
}

/* Applies a fudge to correct the cycle timings for some odd instructions (JMP) */
static inline U32 FUDGE_EA_CYCLES(U32 endCase, U32 offs, U32 stage, U16 operand)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

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
		cycles += 2;
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
		cycles += 1;
		break;
	case 0x30:        /* (XX,Ax,Xx) */
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		cycles += 2;
		break;
	case 0x38:        /* (XXXX).W */
		cycles += 1;
		break;
	case 0x39:        /* (XXXXXXXX).L */
		break;
	case 0x3A:        /* (XXXX,PC) */
		cycles += 1;
		break;
	case 0x3B:        /* (XX,PC,Xx) */
		cycles += 2;
		break;
	case 0x3C:        /* #XX.B #XXXX.W #XXXXXXXX.L */
		break;
	}

	return cycles;
#else
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
#endif
}

static inline int COMPUTE_CONDITION(U16 op)
{
	switch (op)
	{
	case 0x00:
		return 1;
	case 0x01:
		return 0;
	case 0x02:
		return ((~M68K::cpu_regs.SR)&CPU_STATUS_C) && ((~M68K::cpu_regs.SR)&CPU_STATUS_Z);
	case 0x03:
		return (M68K::cpu_regs.SR & CPU_STATUS_C) || (M68K::cpu_regs.SR & CPU_STATUS_Z);
	case 0x04:
		return !(M68K::cpu_regs.SR & CPU_STATUS_C);
	case 0x05:
		return (M68K::cpu_regs.SR & CPU_STATUS_C);
	case 0x06:
		return !(M68K::cpu_regs.SR & CPU_STATUS_Z);
	case 0x07:
		return (M68K::cpu_regs.SR & CPU_STATUS_Z);
	case 0x08:
		return !(M68K::cpu_regs.SR & CPU_STATUS_V);
	case 0x09:
		return (M68K::cpu_regs.SR & CPU_STATUS_V);
	case 0x0A:
		return !(M68K::cpu_regs.SR & CPU_STATUS_N);
	case 0x0B:
		return (M68K::cpu_regs.SR & CPU_STATUS_N);
	case 0x0C:
		return ((M68K::cpu_regs.SR & CPU_STATUS_N) && (M68K::cpu_regs.SR & CPU_STATUS_V)) || ((!(M68K::cpu_regs.SR & CPU_STATUS_N)) && (!(M68K::cpu_regs.SR & CPU_STATUS_V)));
	case 0x0D:
		return ((M68K::cpu_regs.SR & CPU_STATUS_N) && (!(M68K::cpu_regs.SR & CPU_STATUS_V))) || ((!(M68K::cpu_regs.SR & CPU_STATUS_N)) && (M68K::cpu_regs.SR & CPU_STATUS_V));
	case 0x0E:
		return ((M68K::cpu_regs.SR & CPU_STATUS_N) && (M68K::cpu_regs.SR & CPU_STATUS_V) && (!(M68K::cpu_regs.SR & CPU_STATUS_Z))) || ((!(M68K::cpu_regs.SR & CPU_STATUS_N)) && (!(M68K::cpu_regs.SR & CPU_STATUS_V)) && (!(M68K::cpu_regs.SR & CPU_STATUS_Z)));
	case 0x0F:
		return (M68K::cpu_regs.SR & CPU_STATUS_Z) || ((M68K::cpu_regs.SR & CPU_STATUS_N) && (!(M68K::cpu_regs.SR & CPU_STATUS_V))) || ((!(M68K::cpu_regs.SR & CPU_STATUS_N)) && (M68K::cpu_regs.SR & CPU_STATUS_V));
	}
	return 0;
}

static inline U32 LOAD_EFFECTIVE_VALUE(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas, int length)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	if (op<0x10)
	{
		*ead = *eas;
		cycles++;
		return cycles;
	}

	switch (length)
	{
	case 1:
		if (!BUS_Available(*eas))
			cycles++;

		*ead = MEM_getByte(*eas);
		cycles++;
		cycles++;

		break;
	case 2:
		if (!BUS_Available(*eas))
			cycles++;

		*ead = MEM_getWord(*eas);
		cycles++;
		cycles++;

		break;
	case 4:

		M68K::cpu_regs.tmpL = *eas;

		if (!BUS_Available(M68K::cpu_regs.tmpL))
			cycles++;

		*ead = MEM_getWord(M68K::cpu_regs.tmpL) << 16;
		cycles++;
		M68K::cpu_regs.tmpL += 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.tmpL))
			cycles++;

		*ead |= MEM_getWord(M68K::cpu_regs.tmpL);
		cycles++;
		cycles++;

		break;
	}

	return cycles;
#else
	if (op<0x10)
	{
		*ead = *eas;
		return endCase;
	}

	switch (length)
	{
	case 1:
		switch (stage)
		{
		case 0:
			if (!BUS_Available(*eas))
				return 0 + offs;
			*ead = MEM_getByte(*eas);
			return 1 + offs;
		case 1:
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 2:
		switch (stage)
		{
		case 0:
			if (!BUS_Available(*eas))
				return 0 + offs;
			*ead = MEM_getWord(*eas);
			return 1 + offs;
		case 1:
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 4:
		switch (stage)
		{
		case 0:
			M68K::cpu_regs.tmpL = *eas;
			if (!BUS_Available(M68K::cpu_regs.tmpL))
				return 0 + offs;
			*ead = MEM_getWord(M68K::cpu_regs.tmpL) << 16;
			return 1 + offs;
		case 1:
			M68K::cpu_regs.tmpL += 2;
			return 2 + offs;
		case 2:
			if (!BUS_Available(M68K::cpu_regs.tmpL))
				return 2 + offs;
			*ead |= MEM_getWord(M68K::cpu_regs.tmpL);
			return 3 + offs;
		case 3:
			return 4 + offs;
		case 4:
			break;
		}
		break;
	}
	return endCase;
#endif
}

static inline U32 STORE_EFFECTIVE_VALUE(U32 endCase, U32 offs, U32 stage, U16 op, U32 *ead, U32 *eas)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	if (op<0x08)
	{
		M68K::cpu_regs.D[op] &= M68K::cpu_regs.iMask;
		M68K::cpu_regs.D[op] |= (*eas)&M68K::cpu_regs.zMask;
		cycles++;
		return cycles;
	}
	if (op<0x10)
	{
		M68K::cpu_regs.A[op - 0x08] &= M68K::cpu_regs.iMask;
		M68K::cpu_regs.A[op - 0x08] |= (*eas)&M68K::cpu_regs.zMask;
		cycles++;
		return cycles;
	}

	switch (M68K::cpu_regs.len)
	{
	case 1:
		if (!BUS_Available(*ead))
			cycles++;

		MEM_setByte(*ead, (*eas)&M68K::cpu_regs.zMask);
		cycles++;
		cycles++;

		break;
	case 2:
		if (!BUS_Available(*ead))
			cycles++;

		MEM_setWord(*ead, (*eas)&M68K::cpu_regs.zMask);
		cycles++;
		cycles++;

		break;
	case 4:
		M68K::cpu_regs.tmpL = *ead;

		if (!BUS_Available(M68K::cpu_regs.tmpL))
			cycles++;

		MEM_setWord(M68K::cpu_regs.tmpL, ((*eas)&M68K::cpu_regs.zMask) >> 16);
		cycles++;

		M68K::cpu_regs.tmpL += 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.tmpL))
			cycles++;

		MEM_setWord(M68K::cpu_regs.tmpL, (*eas)&M68K::cpu_regs.zMask);
		cycles++;
		cycles++;

		break;
	}

	return cycles;
#else
	if (op<0x08)
	{
		M68K::cpu_regs.D[op] &= M68K::cpu_regs.iMask;
		M68K::cpu_regs.D[op] |= (*eas)&M68K::cpu_regs.zMask;
		return endCase;
	}
	if (op<0x10)
	{
		M68K::cpu_regs.A[op - 0x08] &= M68K::cpu_regs.iMask;
		M68K::cpu_regs.A[op - 0x08] |= (*eas)&M68K::cpu_regs.zMask;
		return endCase;
	}
	switch (M68K::cpu_regs.len)
	{
	case 1:
		switch (stage)
		{
		case 0:
			if (!BUS_Available(*ead))
				return 0 + offs;
			MEM_setByte(*ead, (*eas)&M68K::cpu_regs.zMask);
			return 1 + offs;
		case 1:
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 2:
		switch (stage)
		{
		case 0:
			if (!BUS_Available(*ead))
				return 0 + offs;
			MEM_setWord(*ead, (*eas)&M68K::cpu_regs.zMask);
			return 1 + offs;
		case 1:
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 4:
		switch (stage)
		{
		case 0:
			M68K::cpu_regs.tmpL = *ead;
			if (!BUS_Available(M68K::cpu_regs.tmpL))
				return 0 + offs;
			MEM_setWord(M68K::cpu_regs.tmpL, ((*eas)&M68K::cpu_regs.zMask) >> 16);
			return 1 + offs;
		case 1:
			M68K::cpu_regs.tmpL += 2;
			return 2 + offs;
		case 2:
			if (!BUS_Available(M68K::cpu_regs.tmpL))
				return 2 + offs;
			MEM_setWord(M68K::cpu_regs.tmpL, (*eas)&M68K::cpu_regs.zMask);
			return 3 + offs;
		case 3:
			return 4 + offs;
		case 4:
			break;
		}
		break;
	}
	return endCase;
#endif
}

static inline U32 PUSH_VALUE(U32 endCase, U32 offs, U32 stage, U32 val, int length)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	switch (length)
	{
	case 2:
		M68K::cpu_regs.A[7] -= 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.A[7]))
			cycles++;

		MEM_setWord(M68K::cpu_regs.A[7], val);
		cycles++;

		break;
	case 4:

		M68K::cpu_regs.A[7] -= 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.A[7]))
			cycles++;

		MEM_setWord(M68K::cpu_regs.A[7], val & 0xFFFF);
		cycles++;

		M68K::cpu_regs.A[7] -= 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.A[7]))
			cycles++;

		MEM_setWord(M68K::cpu_regs.A[7], val >> 16);
		cycles++;

		break;
	}

	return cycles;
#else
	switch (length)
	{
	case 2:
		switch (stage)
		{
		case 0:
			M68K::cpu_regs.A[7] -= 2;
			return 1 + offs;
		case 1:
			if (!BUS_Available(M68K::cpu_regs.A[7]))
				return 1 + offs;
			MEM_setWord(M68K::cpu_regs.A[7], val);
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 4:
		switch (stage)
		{
		case 0:
			M68K::cpu_regs.A[7] -= 2;
			return 1 + offs;
		case 1:
			if (!BUS_Available(M68K::cpu_regs.A[7]))
				return 1 + offs;
			MEM_setWord(M68K::cpu_regs.A[7], val & 0xFFFF);
			return 2 + offs;
		case 2:
			M68K::cpu_regs.A[7] -= 2;
			return 3 + offs;
		case 3:
			if (!BUS_Available(M68K::cpu_regs.A[7]))
				return 3 + offs;
			MEM_setWord(M68K::cpu_regs.A[7], val >> 16);
			return 4 + offs;
		case 4:
			break;
		}
		break;
	}
	return endCase;
#endif
}

static inline U32 POP_VALUE(U32 endCase, U32 offs, U32 stage, U32 *val, int length)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	switch (length)
	{
	case 2:
		if (!BUS_Available(M68K::cpu_regs.A[7]))
			cycles++;

		*val = MEM_getWord(M68K::cpu_regs.A[7]);
		cycles++;

		M68K::cpu_regs.A[7] += 2;
		cycles++;

		break;
	case 4:
		if (!BUS_Available(M68K::cpu_regs.A[7]))
			cycles++;

		*val = MEM_getWord(M68K::cpu_regs.A[7]) << 16;
		cycles++;

		M68K::cpu_regs.A[7] += 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.A[7]))
			cycles++;

		*val |= MEM_getWord(M68K::cpu_regs.A[7]);
		cycles++;

		M68K::cpu_regs.A[7] += 2;
		cycles++;

		break;
	}

	return cycles;
#else
	switch (length)
	{
	case 2:
		switch (stage)
		{
		case 0:
			if (!BUS_Available(M68K::cpu_regs.A[7]))
				return 0 + offs;
			*val = MEM_getWord(M68K::cpu_regs.A[7]);
			return 1 + offs;
		case 1:
			M68K::cpu_regs.A[7] += 2;
			return 2 + offs;
		case 2:
			break;
		}
		break;
	case 4:
		switch (stage)
		{
		case 0:
			if (!BUS_Available(M68K::cpu_regs.A[7]))
				return 0 + offs;
			*val = MEM_getWord(M68K::cpu_regs.A[7]) << 16;
			return 1 + offs;
		case 1:
			M68K::cpu_regs.A[7] += 2;
			return 2 + offs;
		case 2:
			if (!BUS_Available(M68K::cpu_regs.A[7]))
				return 2 + offs;
			*val |= MEM_getWord(M68K::cpu_regs.A[7]);
			return 3 + offs;
		case 3:
			M68K::cpu_regs.A[7] += 2;
			return 4 + offs;
		case 4:
			break;
		}
		break;
	}
	return endCase;
#endif
}

static inline U32 OPCODE_SETUP_LENGTHM(U16 op)
{
	switch (op)
	{
	case 0x01:
		M68K::cpu_regs.iMask = 0xFFFFFF00;
		M68K::cpu_regs.nMask = 0x80;
		M68K::cpu_regs.zMask = 0xFF;
		M68K::cpu_regs.len = 1;
		break;
	case 0x03:
		M68K::cpu_regs.iMask = 0xFFFF0000;
		M68K::cpu_regs.nMask = 0x8000;
		M68K::cpu_regs.zMask = 0xFFFF;
		M68K::cpu_regs.len = 2;
		break;
	case 0x02:
		M68K::cpu_regs.iMask = 0x00000000;
		M68K::cpu_regs.nMask = 0x80000000;
		M68K::cpu_regs.zMask = 0xFFFFFFFF;
		M68K::cpu_regs.len = 4;
		break;
	}

	return 1;
}

static inline U32 OPCODE_SETUP_LENGTH(U16 op)
{
	switch (op)
	{
	case 0x00:
		M68K::cpu_regs.iMask = 0xFFFFFF00;
		M68K::cpu_regs.nMask = 0x80;
		M68K::cpu_regs.zMask = 0xFF;
		M68K::cpu_regs.len = 1;
		break;
	case 0x01:
		M68K::cpu_regs.iMask = 0xFFFF0000;
		M68K::cpu_regs.nMask = 0x8000;
		M68K::cpu_regs.zMask = 0xFFFF;
		M68K::cpu_regs.len = 2;
		break;
	case 0x02:
		M68K::cpu_regs.iMask = 0x00000000;
		M68K::cpu_regs.nMask = 0x80000000;
		M68K::cpu_regs.zMask = 0xFFFFFFFF;
		M68K::cpu_regs.len = 4;
		break;
	}

	return 1;
}

static inline U32 OPCODE_SETUP_LENGTHLW(U16 op)
{
	switch (op)
	{
	case 0x00:
		M68K::cpu_regs.iMask = 0xFFFF0000;
		M68K::cpu_regs.nMask = 0x8000;
		M68K::cpu_regs.zMask = 0xFFFF;
		M68K::cpu_regs.len = 2;
		break;
	case 0x01:
		M68K::cpu_regs.iMask = 0x00000000;
		M68K::cpu_regs.nMask = 0x80000000;
		M68K::cpu_regs.zMask = 0xFFFFFFFF;
		M68K::cpu_regs.len = 4;
		break;
	}

	return 1;
}

static inline void COMPUTE_Z_BIT(U32 eas, U32 ead)
{
	if (ead & eas)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;
}

static inline void COMPUTE_ZN_TESTS(U32 ea)
{
	if (ea & M68K::cpu_regs.nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ea & M68K::cpu_regs.zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;
}

static inline void COMPUTE_ADD_XCV_TESTS(U32 eas, U32 ead, U32 ear)
{
	ear &= M68K::cpu_regs.nMask;
	eas &= M68K::cpu_regs.nMask;
	ead &= M68K::cpu_regs.nMask;

	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;
}

static inline void COMPUTE_SUB_XCV_TESTS(U32 eas, U32 ead, U32 ear)
{
	ear &= M68K::cpu_regs.nMask;
	eas &= M68K::cpu_regs.nMask;
	ead &= M68K::cpu_regs.nMask;

	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;
}

static inline void CPU_CHECK_SP(U16 old, U16 new_val)
{
	if (old & CPU_STATUS_S)
	{
		if (!(new_val & CPU_STATUS_S))
		{
			M68K::cpu_regs.ISP = M68K::cpu_regs.A[7];
			M68K::cpu_regs.A[7] = M68K::cpu_regs.USP;
		}
	}
	else
	{
		if (new_val & CPU_STATUS_S)
		{
			M68K::cpu_regs.USP = M68K::cpu_regs.A[7];
			M68K::cpu_regs.A[7] = M68K::cpu_regs.ISP;
		}
	}
}

/* No end case on exception because its final act will be to return 0 */
static inline U32 PROCESS_EXCEPTION(U32 offs, U32 stage, U32 vector)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	M68K::cpu_regs.tmpW = M68K::cpu_regs.SR;
	cycles++;

	M68K::cpu_regs.SR |= CPU_STATUS_S;
	CPU_CHECK_SP(M68K::cpu_regs.tmpW, M68K::cpu_regs.SR);
	cycles++;

	cycles += PUSH_VALUE(7, 2, stage - 2, M68K::cpu_regs.PC - 2, 4);
	cycles += PUSH_VALUE(12, 7, stage - 7, M68K::cpu_regs.SR, 2);

	M68K::cpu_regs.tmpL = vector;
	cycles++;

	if (!BUS_Available(M68K::cpu_regs.tmpL))
		cycles++;
	M68K::cpu_regs.PC = MEM_getWord(M68K::cpu_regs.tmpL) << 16;
	cycles++;

	M68K::cpu_regs.tmpL += 2;
	cycles++;

	if (!BUS_Available(M68K::cpu_regs.tmpL))
		cycles++;

	M68K::cpu_regs.PC |= MEM_getWord(M68K::cpu_regs.tmpL);
	cycles++;

	return cycles;
#else
	switch (stage)
	{
	case 0:
		M68K::cpu_regs.tmpW = M68K::cpu_regs.SR;
		return 1 + offs;
	case 1:
		M68K::cpu_regs.SR |= CPU_STATUS_S;
		CPU_CHECK_SP(M68K::cpu_regs.tmpW, M68K::cpu_regs.SR);
		return 2 + offs;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		return offs + PUSH_VALUE(7, 2, stage - 2, M68K::cpu_regs.PC - 2, 4);
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		return offs + PUSH_VALUE(12, 7, stage - 7, M68K::cpu_regs.SR, 2);
	case 12:
		M68K::cpu_regs.tmpL = vector;
		return 13 + offs;
	case 13:
		if (!BUS_Available(M68K::cpu_regs.tmpL))
			return 13 + offs;
		M68K::cpu_regs.PC = MEM_getWord(M68K::cpu_regs.tmpL) << 16;
		return 14 + offs;
	case 14:
		M68K::cpu_regs.tmpL += 2;
		return 15 + offs;
	case 15:
		if (!BUS_Available(M68K::cpu_regs.tmpL))
			return 15 + offs;
		M68K::cpu_regs.PC |= MEM_getWord(M68K::cpu_regs.tmpL);
		return 16 + offs;
	case 16:
		return 17 + offs;
	case 17:
		break;
	}

	return 0;
#endif
}

/* OLD SUPPORT TO BE REMOVED WHEN ALL INSTRUCTIONS WORKING */

static inline U32 getEffectiveAddress(U16 operand, int length)
{
	U16 tmp;
	U32 ea = 0;
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
		ea = M68K::cpu_regs.D[operand];
		break;
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
		ea = M68K::cpu_regs.A[operand - 0x08];
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		ea = M68K::cpu_regs.A[operand - 0x10];
		break;
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		if ((operand == 0x1F) && (length == 1))
		{
			#if defined _DEBUG
			EMU_PRINTF("Byte size post decrement to stack");
			#endif
			length = 2;
		}
		ea = M68K::cpu_regs.A[operand - 0x18];
		M68K::cpu_regs.A[operand - 0x18] += length;
		break;
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		if ((operand == 0x27) && (length == 1))
		{
			#if defined _DEBUG
			EMU_PRINTF("Byte size preincrement to stack");
			#endif
			length = 2;
		}
		M68K::cpu_regs.A[operand - 0x20] -= length;
		ea = M68K::cpu_regs.A[operand - 0x20];
		break;
	case 0x28:
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
		ea = M68K::cpu_regs.A[operand - 0x28] + (S16)MEM_getWord(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x30:	/* 110rrr */
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		tmp = MEM_getWord(M68K::cpu_regs.PC);
		if (tmp & 0x8000)
		{
			ea = M68K::cpu_regs.A[(tmp >> 12) & 0x7];
			if (!(tmp & 0x0800))
				ea = (S16)ea;
			ea += (S8)(tmp & 0xFF);
			ea += M68K::cpu_regs.A[operand - 0x30];
		}
		else
		{
			ea = M68K::cpu_regs.D[(tmp >> 12)];
			if (!(tmp & 0x0800))
				ea = (S16)ea;
			ea += (S8)(tmp & 0xFF);
			ea += M68K::cpu_regs.A[operand - 0x30];
		}
		M68K::cpu_regs.PC += 2;
		break;
	case 0x38:		/* 111000 */
		ea = (S16)MEM_getWord(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x39:		/* 111001 */
		ea = MEM_getLong(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 4;
		break;
	case 0x3A:		/* 111010 */
		ea = M68K::cpu_regs.PC + (S16)MEM_getWord(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x3B:		/* 111100 */
		tmp = MEM_getWord(M68K::cpu_regs.PC);
		if (tmp & 0x8000)
		{
			ea = M68K::cpu_regs.A[(tmp >> 12) & 0x7];
			if (!(tmp & 0x0800))
				ea = (S16)ea;
			ea += (S8)(tmp & 0xFF);
			ea += M68K::cpu_regs.PC;
		}
		else
		{
			ea = M68K::cpu_regs.D[(tmp >> 12)];
			if (!(tmp & 0x0800))
				ea = (S16)ea;
			ea += (S8)(tmp & 0xFF);
			ea += M68K::cpu_regs.PC;
		}
		M68K::cpu_regs.PC += 2;
		break;
	case 0x3C:		/* 111100 */
		switch (length)
		{
		case 1:
			ea = MEM_getByte(M68K::cpu_regs.PC + 1);
			M68K::cpu_regs.PC += 2;
			break;
		case 2:
			ea = MEM_getWord(M68K::cpu_regs.PC);
			M68K::cpu_regs.PC += 2;
			break;
		case 4:
			ea = MEM_getLong(M68K::cpu_regs.PC);
			M68K::cpu_regs.PC += 4;
			break;
		}
		break;
	default:
		#if defined _DEBUG
		EMU_PRINTF("[ERR] Unsupported effective addressing mode : %04X\n", operand);
		SOFT_BREAK;
		#endif
		break;
	}
	return ea;
}

static inline U32 getSourceEffectiveAddress(U16 operand, int length)
{
	U16 opt;
	U32 eas;

	eas = getEffectiveAddress(operand, length);
	opt = operand & 0x38;
	if ((opt < 0x10) || (operand == 0x3C))
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

static inline void CPU_GENERATE_EXCEPTION(U32 exceptionAddress)
{
	U16 oldSR;

	oldSR = M68K::cpu_regs.SR;
	M68K::cpu_regs.SR |= CPU_STATUS_S;
	CPU_CHECK_SP(oldSR, M68K::cpu_regs.SR);

	M68K::cpu_regs.A[7] -= 4;
	MEM_setLong(M68K::cpu_regs.A[7], M68K::cpu_regs.PC);
	M68K::cpu_regs.A[7] -= 2;
	MEM_setWord(M68K::cpu_regs.A[7], oldSR);

	M68K::cpu_regs.PC = MEM_getLong(exceptionAddress);
}

/*---------------------------------------------------------*/

/*

endCase signifies value when no need to re-enter EA calculation (calc uses upto 4 stages)
offs is amount subtracted from stage to get 0 base (it is added to all return values (except endCase!)
stage should be 0-4 on input

*/

static inline U32 COMPUTE_EFFECTIVE_ADDRESS(U32 endCase, U32 offs, U32 stage, U32 *ea, U16 operand, int length, int asSrc)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

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
		*ea = M68K::cpu_regs.D[operand];
		cycles++;
		return cycles;
	case 0x08:        /* Ax */
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
		*ea = M68K::cpu_regs.A[operand - 0x08];
		cycles++;
		return cycles;
	case 0x10:        /* (Ax) */
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		*ea = M68K::cpu_regs.A[operand - 0x10];
		cycles++;
		return cycles;
	case 0x18:        /* (Ax)+ */
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		if ((operand == 0x1F) && (length == 1))
		{
			length = 2;
		}
		*ea = M68K::cpu_regs.A[operand - 0x18];
		M68K::cpu_regs.A[operand - 0x18] += length;
		cycles++;
		return cycles;
	case 0x20:        /* -(Ax) */
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		if ((operand == 0x27) && (length == 1))
		{
			length = 2;
		}
		M68K::cpu_regs.A[operand - 0x20] -= length;
		cycles++;

		if (asSrc)                                /* When used as src this costs 1 more cycle */
			cycles++;

		*ea = M68K::cpu_regs.A[operand - 0x20];
		cycles++;
		return cycles;
	case 0x28:        /* (XXXX,Ax) */
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
		*ea = M68K::cpu_regs.PC;
		M68K::cpu_regs.PC += 2;
		cycles++;

		if (!BUS_Available(*ea))
			cycles++;

		*ea = M68K::cpu_regs.A[operand - 0x28] + (S16)MEM_getWord(*ea);
		cycles++;

		return cycles;
	case 0x30:        /* (XX,Ax,Xx) */
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		*ea = M68K::cpu_regs.PC;
		M68K::cpu_regs.PC += 2;
		cycles++;

		if (!BUS_Available(*ea))
			cycles++;

		M68K::cpu_regs.tmpW = MEM_getWord(*ea);
		cycles++;

		if (M68K::cpu_regs.tmpW & 0x8000)
		{
			*ea = M68K::cpu_regs.A[(M68K::cpu_regs.tmpW >> 12) & 0x07];
		}
		else
		{
			*ea = M68K::cpu_regs.D[(M68K::cpu_regs.tmpW >> 12) & 0x07];
		}
		cycles++;

		if (!(M68K::cpu_regs.tmpW & 0x0800))
			*ea = (S16)*ea;
		*ea += (S8)(M68K::cpu_regs.tmpW & 0xFF);
		*ea += M68K::cpu_regs.A[operand - 0x30];

		return cycles;
	case 0x38:        /* (XXXX).W */
		*ea = M68K::cpu_regs.PC;
		M68K::cpu_regs.PC += 2;
		cycles++;

		if (!BUS_Available(*ea))
			cycles++;

		*ea = (S16)MEM_getWord(*ea);
		cycles++;

		return cycles;
	case 0x39:        /* (XXXXXXXX).L */
		M68K::cpu_regs.tmpL = M68K::cpu_regs.PC;
		M68K::cpu_regs.PC += 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.tmpL))
			cycles++;

		*ea = MEM_getWord(M68K::cpu_regs.tmpL) << 16;
		cycles++;

		M68K::cpu_regs.tmpL = M68K::cpu_regs.PC;
		M68K::cpu_regs.PC += 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.tmpL))
			cycles++;
		*ea |= MEM_getWord(M68K::cpu_regs.tmpL);
		cycles++;

		return cycles;
	case 0x3A:        /* (XXXX,PC) */
		*ea = M68K::cpu_regs.PC;
		M68K::cpu_regs.PC += 2;
		cycles++;

		if (!BUS_Available(*ea))
			cycles++;

		*ea += (S16)MEM_getWord(*ea);
		cycles++;

		return cycles;
	case 0x3B:        /* (XX,PC,Xx) */
		M68K::cpu_regs.tmpL = M68K::cpu_regs.PC;
		M68K::cpu_regs.PC += 2;
		cycles++;

		if (!BUS_Available(M68K::cpu_regs.tmpL))
			cycles++;

		M68K::cpu_regs.tmpW = MEM_getWord(M68K::cpu_regs.tmpL);
		cycles++;

		if (M68K::cpu_regs.tmpW & 0x8000)
		{
			*ea = M68K::cpu_regs.A[(M68K::cpu_regs.tmpW >> 12) & 0x07];
		}
		else
		{
			*ea = M68K::cpu_regs.D[(M68K::cpu_regs.tmpW >> 12) & 0x07];
		}
		cycles++;

		if (!(M68K::cpu_regs.tmpW & 0x0800))
			*ea = (S16)*ea;
		*ea += (S8)(M68K::cpu_regs.tmpW & 0xFF);
		*ea += M68K::cpu_regs.tmpL;

		return cycles;
	case 0x3C:        /* #XX.B #XXXX.W #XXXXXXXX.L */
		switch (length)
		{
		case 1:
			*ea = M68K::cpu_regs.PC + 1;
			M68K::cpu_regs.PC += 2;
			return cycles;
		case 2:
			*ea = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return cycles;
		case 4:
			*ea = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 4;
			break;
		}
		break;
	}

	return cycles;
#else
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
		*ea = M68K::cpu_regs.D[operand];
		return endCase;
	case 0x08:        /* Ax */
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
		*ea = M68K::cpu_regs.A[operand - 0x08];
		return endCase;
	case 0x10:        /* (Ax) */
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		*ea = M68K::cpu_regs.A[operand - 0x10];
		return endCase;
	case 0x18:        /* (Ax)+ */
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		if ((operand == 0x1F) && (length == 1))
		{
			length = 2;
		}
		*ea = M68K::cpu_regs.A[operand - 0x18];
		M68K::cpu_regs.A[operand - 0x18] += length;
		return endCase;
	case 0x20:        /* -(Ax) */
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		switch (stage)
		{
		case 0:
			if ((operand == 0x27) && (length == 1))
			{
				length = 2;
			}
			M68K::cpu_regs.A[operand - 0x20] -= length;
			if (asSrc)                                /* When used as src this costs 1 more cycle */
				return 1 + offs;
			break;
		case 1:
			break;
		}
		*ea = M68K::cpu_regs.A[operand - 0x20];
		return endCase;
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
			*ea = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return 1 + offs;
		case 1:
			if (!BUS_Available(*ea))
				return 1 + offs;
			*ea = M68K::cpu_regs.A[operand - 0x28] + (S16)MEM_getWord(*ea);
			return 2 + offs;
		case 2:
			break;
		}
		return endCase;
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
			*ea = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return 1 + offs;
		case 1:
			if (!BUS_Available(*ea))
				return 1 + offs;
			M68K::cpu_regs.tmpW = MEM_getWord(*ea);
			return 2 + offs;
		case 2:
			if (M68K::cpu_regs.tmpW & 0x8000)
			{
				*ea = M68K::cpu_regs.A[(M68K::cpu_regs.tmpW >> 12) & 0x07];
			}
			else
			{
				*ea = M68K::cpu_regs.D[(M68K::cpu_regs.tmpW >> 12) & 0x07];
			}
			return 3 + offs;
		case 3:
			if (!(M68K::cpu_regs.tmpW & 0x0800))
				*ea = (S16)*ea;
			*ea += (S8)(M68K::cpu_regs.tmpW & 0xFF);
			*ea += M68K::cpu_regs.A[operand - 0x30];
			break;
		}
		return endCase;
	case 0x38:        /* (XXXX).W */
		switch (stage)
		{
		case 0:
			*ea = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return 1 + offs;
		case 1:
			if (!BUS_Available(*ea))
				return 1 + offs;
			*ea = (S16)MEM_getWord(*ea);
			return 2 + offs;
		case 2:
			break;
		}
		return endCase;
	case 0x39:        /* (XXXXXXXX).L */
		switch (stage)
		{
		case 0:
			M68K::cpu_regs.tmpL = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return 1 + offs;
		case 1:
			if (!BUS_Available(M68K::cpu_regs.tmpL))
				return 1 + offs;
			*ea = MEM_getWord(M68K::cpu_regs.tmpL) << 16;
			return 2 + offs;
		case 2:
			M68K::cpu_regs.tmpL = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return 3 + offs;
		case 3:
			if (!BUS_Available(M68K::cpu_regs.tmpL))
				return 3 + offs;
			*ea |= MEM_getWord(M68K::cpu_regs.tmpL);
			return 4 + offs;
		case 4:
			break;
		}
		return endCase;
	case 0x3A:        /* (XXXX,PC) */
		switch (stage)
		{
		case 0:
			*ea = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return 1 + offs;
		case 1:
			if (!BUS_Available(*ea))
				return 1 + offs;
			*ea += (S16)MEM_getWord(*ea);
			return 2 + offs;
		case 2:
			break;
		}
		return endCase;
	case 0x3B:        /* (XX,PC,Xx) */
		switch (stage)
		{
		case 0:
			M68K::cpu_regs.tmpL = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return 1 + offs;
		case 1:
			if (!BUS_Available(M68K::cpu_regs.tmpL))
				return 1 + offs;
			M68K::cpu_regs.tmpW = MEM_getWord(M68K::cpu_regs.tmpL);
			return 2 + offs;
		case 2:
			if (M68K::cpu_regs.tmpW & 0x8000)
			{
				*ea = M68K::cpu_regs.A[(M68K::cpu_regs.tmpW >> 12) & 0x07];
			}
			else
			{
				*ea = M68K::cpu_regs.D[(M68K::cpu_regs.tmpW >> 12) & 0x07];
			}
			return 3 + offs;
		case 3:
			if (!(M68K::cpu_regs.tmpW & 0x0800))
				*ea = (S16)*ea;
			*ea += (S8)(M68K::cpu_regs.tmpW & 0xFF);
			*ea += M68K::cpu_regs.tmpL;
			break;
		}
		return endCase;
	case 0x3C:        /* #XX.B #XXXX.W #XXXXXXXX.L */
		switch (length)
		{
		case 1:
			*ea = M68K::cpu_regs.PC + 1;
			M68K::cpu_regs.PC += 2;
			return endCase;
		case 2:
			*ea = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 2;
			return endCase;
		case 4:
			*ea = M68K::cpu_regs.PC;
			M68K::cpu_regs.PC += 4;
			break;
		}
		break;
	}

	return endCase;
#endif
}

static inline U32 CPU_LEA(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[1], 4, 1);
	M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.eas;
	cycles++;
	return cycles;
#else
	if (stage == 0)
		return 1;

	U32 ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[1], 4, 1);
	if (ret != 6)
		return ret;

	M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.eas;
	return 0;
#endif
}

static inline U32 CPU_MOVE(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;
	cycles += OPCODE_SETUP_LENGTHM(operands[0]);
	U16 t1, t2;

	t1 = (operands[1] & 0x38) >> 3;			/* needed to swizzle operand for correct later handling (move has inverted bit meanings for dest) */
	t2 = (operands[1] & 0x07) << 3;
	M68K::cpu_regs.operands[1] = t1 | t2;

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);
	cycles += STORE_EFFECTIVE_VALUE(21, 16, stage - 16, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.eas);
	
	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.eas);
	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTHM(operands[0]);
		{
			U16 t1, t2;

			t1 = (operands[1] & 0x38) >> 3;			/* needed to swizzle operand for correct later handling (move has inverted bit meanings for dest) */
			t2 = (operands[1] & 0x07) << 3;
			M68K::cpu_regs.operands[1] = t1 | t2;
		}
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 16)
			return ret;

		stage = 16;			/* Fall through */
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
		ret = STORE_EFFECTIVE_VALUE(21, 16, stage - 16, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.eas);
		if (ret != 21)
			return ret;

		break;
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.eas);
	return 0;
#endif
}

static inline U32 CPU_SUBQ(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(operands[1]);
	if (operands[0] == 0)
		M68K::cpu_regs.operands[0] = 8;			/* 0 means 8 */

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[2], M68K::cpu_regs.len, 0);

	M68K::cpu_regs.eas = operands[0] & M68K::cpu_regs.zMask;

	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	M68K::cpu_regs.ear = (M68K::cpu_regs.eat - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	cycles += STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[2], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);

	M68K::cpu_regs.ead = M68K::cpu_regs.eat;

	if ((operands[2] & 0x38) == 0x00 && M68K::cpu_regs.len == 4)
		cycles+=2;						/* Data register direct long costs 2 cycles more */
	if ((operands[2] & 0x38) == 0x08)	/* Address register direct does not affect flags */
		cycles+=2;						/* also adds 2 cycles (but not worked out reason why yet) */

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);

	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[1]);
		if (operands[0] == 0)
			M68K::cpu_regs.operands[0] = 8;			/* 0 means 8 */
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[2], M68K::cpu_regs.len, 0);

		if (ret != 6)
			return ret;

		M68K::cpu_regs.eas = operands[0] & M68K::cpu_regs.zMask;
		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		M68K::cpu_regs.ear = (M68K::cpu_regs.eat - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;
		stage = 11;
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[2], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);
		if (ret != 16)
			return ret;

		M68K::cpu_regs.ead = M68K::cpu_regs.eat;
		stage = 16;
	case 16:
		if ((operands[2] & 0x38) == 0x00 && M68K::cpu_regs.len == 4)
			return 19;						/* Data register direct long costs 2 cycles more */
		if ((operands[2] & 0x38) == 0x08)				/* Address register direct does not affect flags */
			return 17;						/* also adds 2 cycles (but not worked out reason why yet) */
		break;
	case 17:
		return 18;
	case 18:
		return 0;
	case 19:
		return 20;
	case 20:
		break;

	}

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);

	return 0;
#endif
}

/* Redone timings on this after reaching bra and realising a mistake was made */
static inline U32 CPU_BCC(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles++;

	M68K::cpu_regs.ead = M68K::cpu_regs.PC;
	cycles++;

	if (!COMPUTE_CONDITION(operands[0]))
	{
		cycles+=2;		/* costs 4 if byte form, or 6 if word */

		if (operands[1] == 0)
		{
			M68K::cpu_regs.PC += 2;
			cycles += 2;
		}
		else
		{
			cycles += 4;
		}

		return cycles;
	}
	cycles++;

	M68K::cpu_regs.eas = M68K::cpu_regs.PC;		/* allways reads next word (even if it does not use it!) */
	M68K::cpu_regs.PC += 2;
	cycles++;

	if (!BUS_Available(M68K::cpu_regs.eas))
		cycles++;

	M68K::cpu_regs.eas = MEM_getWord(M68K::cpu_regs.eas);
	cycles++;

	if (operands[1] != 0)
	{
		M68K::cpu_regs.eas = operands[1];
		if (operands[1] & 0x80)
		{
			M68K::cpu_regs.eas += 0xFF00;
		}
	}
	M68K::cpu_regs.PC = M68K::cpu_regs.ead + (S16)M68K::cpu_regs.eas;

	cycles++;
	return cycles;
#else
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
		M68K::cpu_regs.ead = M68K::cpu_regs.PC;
		return 2;
	case 2:
		if (!COMPUTE_CONDITION(operands[0]))
		{
			return 6;		/* costs 4 if byte form, or 6 if word */
		}
		stage = 3;		/* Falls through */
	case 3:
		M68K::cpu_regs.eas = M68K::cpu_regs.PC;		/* allways reads next word (even if it does not use it!) */
		M68K::cpu_regs.PC += 2;
		return 4;
	case 4:
		if (!BUS_Available(M68K::cpu_regs.eas))
			return 4;
		M68K::cpu_regs.eas = MEM_getWord(M68K::cpu_regs.eas);
		return 5;
	case 5:
		if (operands[1] != 0)
		{
			M68K::cpu_regs.eas = operands[1];
			if (operands[1] & 0x80)
			{
				M68K::cpu_regs.eas += 0xFF00;
			}
		}
		M68K::cpu_regs.PC = M68K::cpu_regs.ead + (S16)M68K::cpu_regs.eas;
		break;
	case 6:
		if (operands[1] == 0)
			return 7;
		break;
	case 7:
		M68K::cpu_regs.PC += 2;
		return 8;
	case 8:
		break;
	}

	return 0;
#endif
}

static inline U32 CPU_CMPA(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTHLW(operands[1]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);

	M68K::cpu_regs.ead = M68K::cpu_regs.A[operands[0]];
	if (M68K::cpu_regs.len == 2)
	{
		M68K::cpu_regs.eas = (S16)M68K::cpu_regs.eas;
		cycles += OPCODE_SETUP_LENGTHLW(1);				/* Rest of operation works as long */
	}
	M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	cycles++;
	return cycles;

#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTHLW(operands[1]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);

		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;
	case 11:
		M68K::cpu_regs.ead = M68K::cpu_regs.A[operands[0]];
		if (M68K::cpu_regs.len == 2)
		{
			M68K::cpu_regs.eas = (S16)M68K::cpu_regs.eas;
			OPCODE_SETUP_LENGTHLW(1);				/* Rest of operation works as long */
		}
		return 12;
	case 12:
		M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;
		break;
	}

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);

	return 0;
#endif
}

static inline U32 CPU_CMPI(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(operands[0]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);
	cycles += LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
		cycles++;							/* Long reg direct takes 1 additional cycle */

	M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	cycles++;
	return cycles;

#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 16)
			return ret;

		stage = 16;			/* Fall through */
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
		ret = LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 21)
			return ret;

		if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
			return 21;							/* Long reg direct takes 1 additional cycle */
		stage = 21;			/* Fall through */
	case 21:
		M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;
		break;
	}
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_MOVEA(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTHM(operands[0]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);

	if (M68K::cpu_regs.len == 2)
	{
		M68K::cpu_regs.eas = (S16)M68K::cpu_regs.eas;
	}

	M68K::cpu_regs.A[operands[1]] = M68K::cpu_regs.eas;

	cycles++;

	return cycles;

#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTHM(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		if (M68K::cpu_regs.len == 2)
		{
			M68K::cpu_regs.eas = (S16)M68K::cpu_regs.eas;
		}
		stage = 11;			/* Fall through */
	case 11:
		M68K::cpu_regs.A[operands[1]] = M68K::cpu_regs.eas;
		break;
	}
	return 0;
#endif
}

static inline U32 CPU_DBCC(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;
	
	cycles++;

	M68K::cpu_regs.ead = M68K::cpu_regs.PC;
	cycles++;

	M68K::cpu_regs.eas = M68K::cpu_regs.PC;
	M68K::cpu_regs.PC += 2;
	cycles++;

	if (!BUS_Available(M68K::cpu_regs.eas))
		cycles++;

	M68K::cpu_regs.eas = MEM_getWord(M68K::cpu_regs.eas);
	cycles++;

	if (COMPUTE_CONDITION(operands[0]))
	{
		cycles++;						/* burn one last cycle */
		return cycles;
	}

	operands[3] = (M68K::cpu_regs.D[operands[1]] - 1) & 0xFFFF;
	M68K::cpu_regs.D[operands[1]] &= 0xFFFF0000;
	M68K::cpu_regs.D[operands[1]] |= operands[3];
	if (operands[3] == 0xFFFF)
	{
		cycles += 2;						/* burn two more cycles */
		return cycles;
	}

	M68K::cpu_regs.PC = M68K::cpu_regs.ead + (S16)M68K::cpu_regs.eas;
	cycles++;
	return cycles;
#else
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
		M68K::cpu_regs.ead = M68K::cpu_regs.PC;
		return 2;
	case 2:
		M68K::cpu_regs.eas = M68K::cpu_regs.PC;
		M68K::cpu_regs.PC += 2;
		return 3;
	case 3:
		if (!BUS_Available(M68K::cpu_regs.eas))
			return 3;
		M68K::cpu_regs.eas = MEM_getWord(M68K::cpu_regs.eas);
		return 4;
	case 4:
		if (COMPUTE_CONDITION(operands[0]))
			return 7;						/* burn one last cycle */

		operands[3] = (M68K::cpu_regs.D[operands[1]] - 1) & 0xFFFF;
		M68K::cpu_regs.D[operands[1]] &= 0xFFFF0000;
		M68K::cpu_regs.D[operands[1]] |= operands[3];
		if (operands[3] == 0xFFFF)
			return 6;						/* burn two more cycles */

		M68K::cpu_regs.PC = M68K::cpu_regs.ead + (S16)M68K::cpu_regs.eas;
		break;
	case 6:
		return 7;
	case 7:
		break;
	}

	return 0;
#endif
}

static inline U32 CPU_BRA(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles++;

	M68K::cpu_regs.ead = M68K::cpu_regs.PC;
	cycles++;

	M68K::cpu_regs.eas = M68K::cpu_regs.PC;		/* allways reads next word (even if it does not use it!) */
	M68K::cpu_regs.PC += 2;
	cycles++;

	if (!BUS_Available(M68K::cpu_regs.eas))
		cycles++;

	M68K::cpu_regs.eas = MEM_getWord(M68K::cpu_regs.eas);
	cycles++;

	if (operands[0] != 0)
	{
		M68K::cpu_regs.eas = operands[0];
		if (operands[0] & 0x80)
		{
			M68K::cpu_regs.eas += 0xFF00;
		}
	}
	M68K::cpu_regs.PC = M68K::cpu_regs.ead + (S16)M68K::cpu_regs.eas;
	cycles++;
	return cycles;

#else
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
		M68K::cpu_regs.ead = M68K::cpu_regs.PC;
		return 2;
	case 2:
		M68K::cpu_regs.eas = M68K::cpu_regs.PC;		/* allways reads next word (even if it does not use it!) */
		M68K::cpu_regs.PC += 2;
		return 3;
	case 3:
		if (!BUS_Available(M68K::cpu_regs.eas))
			return 3;
		M68K::cpu_regs.eas = MEM_getWord(M68K::cpu_regs.eas);
		return 4;
	case 4:
		if (operands[0] != 0)
		{
			M68K::cpu_regs.eas = operands[0];
			if (operands[0] & 0x80)
			{
				M68K::cpu_regs.eas += 0xFF00;
			}
		}
		M68K::cpu_regs.PC = M68K::cpu_regs.ead + (S16)M68K::cpu_regs.eas;
		break;
	}

	return 0;
#endif
}

static inline U32 CPU_BTSTI(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	if (operands[0]<0x08)
		cycles += OPCODE_SETUP_LENGTH(2);			/* Register direct uses long */
	else
		cycles += OPCODE_SETUP_LENGTH(0);			/* memory uses byte */

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, 1, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, 1);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[0], M68K::cpu_regs.len, 0);
	cycles += LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[0], &M68K::cpu_regs.ead, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	if ((operands[0] & 0x38) == 0)
		cycles++;							/* Long reg direct takes 1 additional cycle */

	if (operands[0]<0x08)
		M68K::cpu_regs.eas &= 0x1F;
	else
		M68K::cpu_regs.eas &= 0x07;

	M68K::cpu_regs.eas = 1 << M68K::cpu_regs.eas;

	COMPUTE_Z_BIT(M68K::cpu_regs.eas, M68K::cpu_regs.ead);
	cycles++;
	return cycles;

#else
	U32 ret;

	switch (stage)
	{
	case 0:
		if (operands[0]<0x08)
			OPCODE_SETUP_LENGTH(2);			/* Register direct uses long */
		else
			OPCODE_SETUP_LENGTH(0);			/* memory uses byte */
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, 1, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, 1);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[0], M68K::cpu_regs.len, 0);

		if (ret != 16)
			return ret;

		stage = 16;			/* Fall through */
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
		ret = LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[0], &M68K::cpu_regs.ead, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 21)
			return ret;

		if ((operands[0] & 0x38) == 0)
			return 21;							/* Long reg direct takes 1 additional cycle */
		stage = 21;			/* Fall through */
	case 21:
		if (operands[0]<0x08)
			M68K::cpu_regs.eas &= 0x1F;
		else
			M68K::cpu_regs.eas &= 0x07;
		M68K::cpu_regs.eas = 1 << M68K::cpu_regs.eas;
		break;
	}

	COMPUTE_Z_BIT(M68K::cpu_regs.eas, M68K::cpu_regs.ead);
	return 0;
#endif
}

static inline U32 CPU_ADDs(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	OPCODE_SETUP_LENGTH(operands[1]);
	cycles++;

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);

	M68K::cpu_regs.ead = M68K::cpu_regs.D[operands[0]] & M68K::cpu_regs.zMask;

	if (M68K::cpu_regs.len == 4)
		cycles++;							/* Long reg direct takes 1 additional cycle */

	M68K::cpu_regs.ear = (M68K::cpu_regs.ead + M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	M68K::cpu_regs.D[operands[0]] &= M68K::cpu_regs.iMask;
	M68K::cpu_regs.D[operands[0]] |= M68K::cpu_regs.ear;

	if (M68K::cpu_regs.len == 4 && ((operands[2] == 0x3C) || (operands[2]<0x10)))
		cycles++;							/* long source reg direct and immediate take 1 additional cycle */

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_ADD_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	cycles++;
	return cycles;

#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[1]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
		M68K::cpu_regs.ead = M68K::cpu_regs.D[operands[0]] & M68K::cpu_regs.zMask;

		if (M68K::cpu_regs.len == 4)
			return 12;							/* Long reg direct takes 1 additional cycle */
		stage = 12;			/* Fall through */
	case 12:
		M68K::cpu_regs.ear = (M68K::cpu_regs.ead + M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

		M68K::cpu_regs.D[operands[0]] &= M68K::cpu_regs.iMask;
		M68K::cpu_regs.D[operands[0]] |= M68K::cpu_regs.ear;

		if (M68K::cpu_regs.len == 4 && ((operands[2] == 0x3C) || (operands[2]<0x10)))
			return 13;							/* long source reg direct and immediate take 1 additional cycle */
		break;
	case 13:
		break;
	}
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_ADD_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_NOT(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(operands[0]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
	M68K::cpu_regs.ear = (~M68K::cpu_regs.eat)&M68K::cpu_regs.zMask;
	cycles += STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);

	M68K::cpu_regs.ead = M68K::cpu_regs.eat;

	if ((operands[1] & 0x38) == 0x00)
	{
		if (M68K::cpu_regs.len == 4)
			cycles++;						/* Data register direct long costs 1 cycles more */
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		M68K::cpu_regs.ear = (~M68K::cpu_regs.eat)&M68K::cpu_regs.zMask;
		stage = 11;
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);
		if (ret != 16)
			return ret;

		M68K::cpu_regs.ead = M68K::cpu_regs.eat;
		stage = 16;
	case 16:
		if ((operands[1] & 0x38) == 0x00)
		{
			if (M68K::cpu_regs.len == 4)
				return 17;						/* Data register direct long costs 1 cycles more */
		}
		break;
	case 17:
		break;
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_SUBA(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTHLW(operands[1]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
	M68K::cpu_regs.ead = M68K::cpu_regs.A[operands[0]];

	cycles++;

	if (M68K::cpu_regs.len == 2)
	{
		M68K::cpu_regs.eas = (S16)M68K::cpu_regs.eas;		/* sign extension costs an additional cycle */
		cycles++;
	}

	M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas);

	M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.ear;

	if (M68K::cpu_regs.len == 4 && ((operands[2] == 0x3C) || (operands[2]<0x10)))
		cycles++;							/* long source reg direct and immediate take 1 additional cycle */

	cycles++;
	return cycles;

#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTHLW(operands[1]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
		M68K::cpu_regs.ead = M68K::cpu_regs.A[operands[0]];

		return 12;
	case 12:
		if (M68K::cpu_regs.len == 2)
		{
			M68K::cpu_regs.eas = (S16)M68K::cpu_regs.eas;		/* sign extension costs an additional cycle */
			return 13;
		}
		stage = 13;			/* Fall through */
	case 13:
		M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas);

		M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.ear;

		if (M68K::cpu_regs.len == 4 && ((operands[2] == 0x3C) || (operands[2]<0x10)))
			return 14;							/* long source reg direct and immediate take 1 additional cycle */
		break;
	case 14:
		break;
	}
	return 0;
#endif
}

static inline U32 CPU_ADDA(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTHLW(operands[1]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);

	M68K::cpu_regs.ead = M68K::cpu_regs.A[operands[0]];

	cycles++;

	if (M68K::cpu_regs.len == 2)
	{
		M68K::cpu_regs.eas = (S16)M68K::cpu_regs.eas;		/* sign extension costs an additional cycle */
		cycles++;
	}

	M68K::cpu_regs.ear = (M68K::cpu_regs.ead + M68K::cpu_regs.eas);

	M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.ear;

	if (M68K::cpu_regs.len == 4 && ((operands[2] == 0x3C) || (operands[2]<0x10)))
		cycles++;							/* long source reg direct and immediate take 1 additional cycle */

	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTHLW(operands[1]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
		M68K::cpu_regs.ead = M68K::cpu_regs.A[operands[0]];

		return 12;
	case 12:
		if (M68K::cpu_regs.len == 2)
		{
			M68K::cpu_regs.eas = (S16)M68K::cpu_regs.eas;		/* sign extension costs an additional cycle */
			return 13;
		}
		stage = 13;			/* Fall through */
	case 13:
		M68K::cpu_regs.ear = (M68K::cpu_regs.ead + M68K::cpu_regs.eas);

		M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.ear;

		if (M68K::cpu_regs.len == 4 && ((operands[2] == 0x3C) || (operands[2]<0x10)))
			return 14;							/* long source reg direct and immediate take 1 additional cycle */
		break;
	case 14:
		break;
	}
	return 0;
#endif
}

static inline U32 CPU_TST(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(operands[0]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	M68K::cpu_regs.ear = M68K::cpu_regs.eat&M68K::cpu_regs.zMask;

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);

	cycles++;
	return cycles;

#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		M68K::cpu_regs.ear = M68K::cpu_regs.eat&M68K::cpu_regs.zMask;
		break;
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_JMP(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles++;

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[0], 4, 1);
	cycles += FUDGE_EA_CYCLES(9, 6, stage - 6, operands[0]);		/*ODD cycle timings for JMP -this rebalances*/

	M68K::cpu_regs.PC = M68K::cpu_regs.eas;

	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[0], 4, 1);
		if (ret != 6)
			return ret;
		stage = 6;		/* Fall through */
	case 6:
	case 7:
	case 8:
		ret = FUDGE_EA_CYCLES(9, 6, stage - 6, operands[0]);		/*ODD cycle timings for JMP -this rebalances*/
		if (ret != 9)
			return ret;

		break;
	}

	M68K::cpu_regs.PC = M68K::cpu_regs.eas;
	return 0;
#endif
}

static inline U32 CPU_MOVEQ(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(2);

	M68K::cpu_regs.D[operands[0]] = (S8)operands[1];
	cycles++;

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.D[operands[0]]);

	cycles++;
	return cycles;
#else
	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(2);
		return 1;
	case 1:
		M68K::cpu_regs.D[operands[0]] = (S8)operands[1];
		break;
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.D[operands[0]]);
	return 0;
#endif
}

static inline U32 CPU_CMP(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(operands[1]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);

	M68K::cpu_regs.ead = M68K::cpu_regs.D[operands[0]] & M68K::cpu_regs.zMask;
	M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	if ((operands[1] & 0x38) == 0x00 && M68K::cpu_regs.len == 4)
		cycles++;						/* Data register direct long costs 1 cycles more */

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);

	cycles++;
	return cycles;

#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[1]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);

		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		M68K::cpu_regs.ead = M68K::cpu_regs.D[operands[0]] & M68K::cpu_regs.zMask;
		M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;
		stage = 11;
	case 11:
		if ((operands[1] & 0x38) == 0x00 && M68K::cpu_regs.len == 4)
			return 12;						/* Data register direct long costs 1 cycles more */
		break;
	case 12:
		break;
	}

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_SUBs(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(operands[1]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);

	M68K::cpu_regs.ead = M68K::cpu_regs.D[operands[0]] & M68K::cpu_regs.zMask;

	if (M68K::cpu_regs.len == 4)
		cycles++;							/* Long reg direct takes 1 additional cycle */

	M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	M68K::cpu_regs.D[operands[0]] &= M68K::cpu_regs.iMask;
	M68K::cpu_regs.D[operands[0]] |= M68K::cpu_regs.ear;

	if (M68K::cpu_regs.len == 4 && ((operands[2] == 0x3C) || (operands[2]<0x10)))
		cycles++;							/* long source reg direct and immediate take 1 additional cycle */

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	cycles++;

	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[1]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[2], M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
		M68K::cpu_regs.ead = M68K::cpu_regs.D[operands[0]] & M68K::cpu_regs.zMask;

		if (M68K::cpu_regs.len == 4)
			return 12;							/* Long reg direct takes 1 additional cycle */
		stage = 12;			/* Fall through */
	case 12:
		M68K::cpu_regs.ear = (M68K::cpu_regs.ead - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

		M68K::cpu_regs.D[operands[0]] &= M68K::cpu_regs.iMask;
		M68K::cpu_regs.D[operands[0]] |= M68K::cpu_regs.ear;

		if (M68K::cpu_regs.len == 4 && ((operands[2] == 0x3C) || (operands[2]<0x10)))
			return 13;							/* long source reg direct and immediate take 1 additional cycle */
		break;
	case 13:
		break;
	}
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_LSR(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(operands[1]);

	if (operands[2] == 0)
	{
		if (operands[0] == 0)
			M68K::cpu_regs.tmpL = 8;
		else
			M68K::cpu_regs.tmpL = operands[0];
	}
	else
	{
		M68K::cpu_regs.tmpL = M68K::cpu_regs.D[operands[0]] & 0x3F;
	}
	M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_V);
	cycles++;

	if (M68K::cpu_regs.len == 4)
		cycles++;

	while (M68K::cpu_regs.tmpL)
	{
		M68K::cpu_regs.eas = M68K::cpu_regs.D[operands[3]] & M68K::cpu_regs.zMask;
		M68K::cpu_regs.ead = (M68K::cpu_regs.eas >> 1)&M68K::cpu_regs.zMask;
		M68K::cpu_regs.D[operands[3]] &= M68K::cpu_regs.iMask;
		M68K::cpu_regs.D[operands[3]] |= M68K::cpu_regs.ead;

		if (M68K::cpu_regs.eas & 1)
		{
			M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
		}
		else
		{
			M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
		}
		M68K::cpu_regs.tmpL--;
		cycles++;
	}

	COMPUTE_ZN_TESTS(M68K::cpu_regs.D[operands[3]]);

	cycles++;
	return cycles;
#else
	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[1]);
		return 1;
	case 1:
		if (operands[2] == 0)
		{
			if (operands[0] == 0)
				M68K::cpu_regs.tmpL = 8;
			else
				M68K::cpu_regs.tmpL = operands[0];
		}
		else
		{
			M68K::cpu_regs.tmpL = M68K::cpu_regs.D[operands[0]] & 0x3F;
		}
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_V);
		return 2;
	case 2:
		if (M68K::cpu_regs.len == 4)
			return 3;

		stage = 3;		/* Fall through */
	case 3:
		if (M68K::cpu_regs.tmpL)
		{
			M68K::cpu_regs.eas = M68K::cpu_regs.D[operands[3]] & M68K::cpu_regs.zMask;
			M68K::cpu_regs.ead = (M68K::cpu_regs.eas >> 1)&M68K::cpu_regs.zMask;
			M68K::cpu_regs.D[operands[3]] &= M68K::cpu_regs.iMask;
			M68K::cpu_regs.D[operands[3]] |= M68K::cpu_regs.ead;

			if (M68K::cpu_regs.eas & 1)
			{
				M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
			}
			else
			{
				M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
			}
			M68K::cpu_regs.tmpL--;
			return 3;
		}
		break;
	}
	COMPUTE_ZN_TESTS(M68K::cpu_regs.D[operands[3]]);
	return 0;
#endif
}

static inline U32 CPU_SWAP(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTH(2);

	M68K::cpu_regs.tmpL = (M68K::cpu_regs.D[operands[0]] & 0xFFFF0000) >> 16;
	M68K::cpu_regs.D[operands[0]] = (M68K::cpu_regs.D[operands[0]] & 0x0000FFFF) << 16;
	M68K::cpu_regs.D[operands[0]] |= M68K::cpu_regs.tmpL;

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.D[operands[0]]);
	cycles++;
	return cycles;

#else
	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(2);
		return 1;
	case 1:
		M68K::cpu_regs.tmpL = (M68K::cpu_regs.D[operands[0]] & 0xFFFF0000) >> 16;
		M68K::cpu_regs.D[operands[0]] = (M68K::cpu_regs.D[operands[0]] & 0x0000FFFF) << 16;
		M68K::cpu_regs.D[operands[0]] |= M68K::cpu_regs.tmpL;
		break;
	}
	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.D[operands[0]]);
	return 0;
#endif
}

static inline U32 CPU_MOVEMs(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTHLW(operands[0]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, 2, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, 2);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

	M68K::cpu_regs.ear = 0;
	cycles++;
	cycles++;

	while (M68K::cpu_regs.eas)				/* count leading zeros would be better here */
	{
		if (M68K::cpu_regs.eas & 1)
		{
			cycles += LOAD_EFFECTIVE_VALUE(23, 18, stage - 18, 0x3C, &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

			/* load register value */
			M68K::cpu_regs.ead += M68K::cpu_regs.len;
			if (M68K::cpu_regs.len == 2)
				M68K::cpu_regs.eat = (S16)M68K::cpu_regs.eat;
			if (M68K::cpu_regs.ear<8)
			{
				M68K::cpu_regs.D[M68K::cpu_regs.ear] = M68K::cpu_regs.eat;
			}
			else
			{
				M68K::cpu_regs.A[M68K::cpu_regs.ear - 8] = M68K::cpu_regs.eat;
			}
		}
		M68K::cpu_regs.eas >>= 1;
		M68K::cpu_regs.ear++;
	}

	if ((operands[1] & 0x38) == 0x18)			/* handle post increment case */
	{
		M68K::cpu_regs.A[operands[1] - 0x18] = M68K::cpu_regs.ead;
	}

	cycles++;

	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTHLW(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, 2, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, 2);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 16)
			return ret;

		stage = 16;			/* Fall through */
	case 16:
		M68K::cpu_regs.ear = 0;
		return 17;
	case 17:
		return 18;
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
		while (M68K::cpu_regs.eas)				/* count leading zeros would be better here */
		{
			if (M68K::cpu_regs.eas & 1)
			{
				ret = LOAD_EFFECTIVE_VALUE(23, 18, stage - 18, 0x3C, &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

				if (ret != 23)
					return ret;

				stage = 18;
				/* load register value */
				M68K::cpu_regs.ead += M68K::cpu_regs.len;
				if (M68K::cpu_regs.len == 2)
					M68K::cpu_regs.eat = (S16)M68K::cpu_regs.eat;
				if (M68K::cpu_regs.ear<8)
				{
					M68K::cpu_regs.D[M68K::cpu_regs.ear] = M68K::cpu_regs.eat;
				}
				else
				{
					M68K::cpu_regs.A[M68K::cpu_regs.ear - 8] = M68K::cpu_regs.eat;
				}
			}
			M68K::cpu_regs.eas >>= 1;
			M68K::cpu_regs.ear++;
		}
		break;
	}
	if ((operands[1] & 0x38) == 0x18)			/* handle post increment case */
	{
		M68K::cpu_regs.A[operands[1] - 0x18] = M68K::cpu_regs.ead;
	}
	return 0;
#endif
}

static inline U32 CPU_MOVEMd(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles += OPCODE_SETUP_LENGTHLW(operands[0]);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, 2, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, 2);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

	if ((operands[1] & 0x38) == 0x20)			/* handle pre decrement case */
	{
		M68K::cpu_regs.ear = 15;
		M68K::cpu_regs.ead += M68K::cpu_regs.len;
		M68K::cpu_regs.A[operands[1] - 0x20] += M68K::cpu_regs.len;
	}
	else
	{
		M68K::cpu_regs.ear = 0;
	}

	while (M68K::cpu_regs.eas)				/* count leading zeros would be better here */
	{
		if (M68K::cpu_regs.eas & 1)
		{
			if ((operands[1] & 0x38) == 0x20)
			{
				M68K::cpu_regs.ead -= M68K::cpu_regs.len;
			}

			if (M68K::cpu_regs.ear<8)
				M68K::cpu_regs.eat = M68K::cpu_regs.D[M68K::cpu_regs.ear] & M68K::cpu_regs.zMask;
			else
				M68K::cpu_regs.eat = M68K::cpu_regs.A[M68K::cpu_regs.ear - 8] & M68K::cpu_regs.zMask;

			cycles += STORE_EFFECTIVE_VALUE(21, 16, stage - 16, 0x3C, &M68K::cpu_regs.ead, &M68K::cpu_regs.eat);

			/* load register value */
			if ((operands[1] & 0x38) != 0x20)
				M68K::cpu_regs.ead += M68K::cpu_regs.len;
		}
		M68K::cpu_regs.eas >>= 1;
		if ((operands[1] & 0x38) == 0x20)
			M68K::cpu_regs.ear--;
		else
			M68K::cpu_regs.ear++;
	}

	if ((operands[1] & 0x38) == 0x20)			/* handle pre decrement case */
		M68K::cpu_regs.A[operands[1] - 0x20] = M68K::cpu_regs.ead;

	cycles++;

	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTHLW(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, 2, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, 2);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 16)
			return ret;

		if ((operands[1] & 0x38) == 0x20)			/* handle pre decrement case */
		{
			M68K::cpu_regs.ear = 15;
			M68K::cpu_regs.ead += M68K::cpu_regs.len;
			M68K::cpu_regs.A[operands[1] - 0x20] += M68K::cpu_regs.len;
		}
		else
		{
			M68K::cpu_regs.ear = 0;
		}
		stage = 16;			/* Fall through */

	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
		while (M68K::cpu_regs.eas)				/* count leading zeros would be better here */
		{
			if (M68K::cpu_regs.eas & 1)
			{
				if (stage == 16 && ((operands[1] & 0x38) == 0x20))
				{
					M68K::cpu_regs.ead -= M68K::cpu_regs.len;
				}
				if (stage == 16)
				{
					if (M68K::cpu_regs.ear<8)
						M68K::cpu_regs.eat = M68K::cpu_regs.D[M68K::cpu_regs.ear] & M68K::cpu_regs.zMask;
					else
						M68K::cpu_regs.eat = M68K::cpu_regs.A[M68K::cpu_regs.ear - 8] & M68K::cpu_regs.zMask;
				}

				ret = STORE_EFFECTIVE_VALUE(21, 16, stage - 16, 0x3C, &M68K::cpu_regs.ead, &M68K::cpu_regs.eat);

				if (ret != 21)
					return ret;

				stage = 16;
				/* load register value */
				if ((operands[1] & 0x38) != 0x20)
					M68K::cpu_regs.ead += M68K::cpu_regs.len;
			}
			M68K::cpu_regs.eas >>= 1;
			if ((operands[1] & 0x38) == 0x20)
				M68K::cpu_regs.ear--;
			else
				M68K::cpu_regs.ear++;
		}
		break;
	}
	if ((operands[1] & 0x38) == 0x20)			/* handle pre decrement case */
		M68K::cpu_regs.A[operands[1] - 0x20] = M68K::cpu_regs.ead;

	return 0;
#endif
}

static inline U32 CPU_SUBI(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	OPCODE_SETUP_LENGTH(operands[0]);
	cycles++;

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);
	cycles += LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	M68K::cpu_regs.ear = (M68K::cpu_regs.eat - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
		cycles++;							/* Long reg direct takes 1 additional cycle on read and write */

	cycles += STORE_EFFECTIVE_VALUE(26, 21, stage - 21, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);

	M68K::cpu_regs.ead = M68K::cpu_regs.eat;

	if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
		cycles++;							/* Long reg direct takes 1 additional cycle on read and write */

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	cycles++;

	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 16)
			return ret;

		stage = 16;			/* Fall through */
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
		ret = LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 21)
			return ret;

		M68K::cpu_regs.ear = (M68K::cpu_regs.eat - M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

		if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
			return 21;							/* Long reg direct takes 1 additional cycle on read and write */

		stage = 21;			/* Fall through */
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
		ret = STORE_EFFECTIVE_VALUE(26, 21, stage - 21, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);
		if (ret != 26)
			return ret;

		M68K::cpu_regs.ead = M68K::cpu_regs.eat;

		if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
			return 26;							/* Long reg direct takes 1 additional cycle on read and write */
		break;
	case 26:
		break;
	}
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_BSR(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles++;

	M68K::cpu_regs.ead = M68K::cpu_regs.PC;
	cycles++;

	M68K::cpu_regs.eas = M68K::cpu_regs.PC;		/* allways reads next word (even if it does not use it!) */
	M68K::cpu_regs.PC += 2;
	cycles++;

	if (!BUS_Available(M68K::cpu_regs.eas))
		cycles++;

	M68K::cpu_regs.eas = MEM_getWord(M68K::cpu_regs.eas);
	cycles++;

	if (operands[0] != 0)
	{
		M68K::cpu_regs.eas = operands[0];
		if (operands[0] & 0x80)
		{
			M68K::cpu_regs.eas += 0xFF00;
		}
	}

	M68K::cpu_regs.PC = M68K::cpu_regs.ead + (S16)M68K::cpu_regs.eas;

	if (operands[0] == 0)
	{
		M68K::cpu_regs.ead += 2;		/* account for extra word */
	}

	cycles += PUSH_VALUE(10, 5, stage - 5, M68K::cpu_regs.ead, 4);

	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		return 1;
	case 1:
		M68K::cpu_regs.ead = M68K::cpu_regs.PC;
		return 2;
	case 2:
		M68K::cpu_regs.eas = M68K::cpu_regs.PC;		/* allways reads next word (even if it does not use it!) */
		M68K::cpu_regs.PC += 2;
		return 3;
	case 3:
		if (!BUS_Available(M68K::cpu_regs.eas))
			return 3;
		M68K::cpu_regs.eas = MEM_getWord(M68K::cpu_regs.eas);
		return 4;
	case 4:
		if (operands[0] != 0)
		{
			M68K::cpu_regs.eas = operands[0];
			if (operands[0] & 0x80)
			{
				M68K::cpu_regs.eas += 0xFF00;
			}
		}

		M68K::cpu_regs.PC = M68K::cpu_regs.ead + (S16)M68K::cpu_regs.eas;

		if (operands[0] == 0)
		{
			M68K::cpu_regs.ead += 2;		/* account for extra word */
		}

		stage = 5;	/* Fall through */
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		ret = PUSH_VALUE(10, 5, stage - 5, M68K::cpu_regs.ead, 4);
		if (ret != 10)
			return ret;
		break;
	}

	return 0;
#endif
}

static inline U32 CPU_RTS(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles++;
	cycles += POP_VALUE(6, 1, stage - 1, &M68K::cpu_regs.ead, 4);
	cycles++;
	M68K::cpu_regs.PC = M68K::cpu_regs.ead;
	cycles++;

	return cycles;
#else
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		return POP_VALUE(6, 1, stage - 1, &M68K::cpu_regs.ead, 4);
	case 6:
		return 7;
	case 7:
		M68K::cpu_regs.PC = M68K::cpu_regs.ead;
		break;
	}

	return 0;
#endif
}

static inline U32 CPU_ILLEGAL(U32 stage, U16* operands)
{
	switch (stage)
	{
	case 0:
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
		return PROCESS_EXCEPTION(1, stage - 1, 0x10);
	}
	return 0;
}

static inline U32 CPU_ORd(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	OPCODE_SETUP_LENGTH(operands[1]);
	cycles++;

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[2], M68K::cpu_regs.len, 0);

	M68K::cpu_regs.eas = M68K::cpu_regs.D[operands[0]] & M68K::cpu_regs.zMask;

	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	M68K::cpu_regs.ear = (M68K::cpu_regs.eat | M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	cycles += STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[2], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);

	M68K::cpu_regs.ead = M68K::cpu_regs.eat;

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[1]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[2], M68K::cpu_regs.len, 0);

		if (ret != 6)
			return ret;

		M68K::cpu_regs.eas = M68K::cpu_regs.D[operands[0]] & M68K::cpu_regs.zMask;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		M68K::cpu_regs.ear = (M68K::cpu_regs.eat | M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[2], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);
		if (ret != 16)
			return ret;

		M68K::cpu_regs.ead = M68K::cpu_regs.eat;
		break;
	}
	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_ADDQ(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	OPCODE_SETUP_LENGTH(operands[1]);
	if (operands[0] == 0)
		M68K::cpu_regs.operands[0] = 8;			/* 0 means 8 */
	cycles++;

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[2], M68K::cpu_regs.len, 0);

	M68K::cpu_regs.eas = operands[0] & M68K::cpu_regs.zMask;

	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	M68K::cpu_regs.ear = (M68K::cpu_regs.eat + M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	cycles += STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[2], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);

	M68K::cpu_regs.ead = M68K::cpu_regs.eat;

	if ((operands[2] & 0x38) == 0x00 && M68K::cpu_regs.len == 4)
		cycles++;						/* Data register direct long costs 2 cycles more */

	if ((operands[2] & 0x38) == 0x08)
	{
		/* Address register direct does not affect flags */
		/* also adds 2 cycles (but not worked out reason why yet) */
		cycles++;
	}
	else
	{
		COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
		COMPUTE_ADD_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);
	}

	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[1]);
		if (operands[0] == 0)
			M68K::cpu_regs.operands[0] = 8;			/* 0 means 8 */
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[2], M68K::cpu_regs.len, 0);

		if (ret != 6)
			return ret;

		M68K::cpu_regs.eas = operands[0] & M68K::cpu_regs.zMask;
		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[2], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		M68K::cpu_regs.ear = (M68K::cpu_regs.eat + M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;
		stage = 11;
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[2], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);
		if (ret != 16)
			return ret;

		M68K::cpu_regs.ead = M68K::cpu_regs.eat;
		stage = 16;
	case 16:
		if ((operands[2] & 0x38) == 0x00 && M68K::cpu_regs.len == 4)
			return 19;						/* Data register direct long costs 2 cycles more */
		if ((operands[2] & 0x38) == 0x08)				/* Address register direct does not affect flags */
			return 17;						/* also adds 2 cycles (but not worked out reason why yet) */
		break;
	case 17:
		return 18;
	case 18:
		return 0;
	case 19:
		return 20;
	case 20:
		break;

	}

	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	COMPUTE_ADD_XCV_TESTS(M68K::cpu_regs.eas, M68K::cpu_regs.ead, M68K::cpu_regs.ear);

	return 0;
#endif
}

static inline U32 CPU_CLEAR(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	OPCODE_SETUP_LENGTH(operands[0]);
	cycles++;

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	M68K::cpu_regs.ear = 0;

	cycles += STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);

	M68K::cpu_regs.ead = M68K::cpu_regs.eat;

	if ((operands[1] & 0x38) == 0x00)
	{
		if (M68K::cpu_regs.len == 4)
			cycles++;						/* Data register direct long costs 1 cycles more */
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		M68K::cpu_regs.ear = 0;
		stage = 11;
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = STORE_EFFECTIVE_VALUE(16, 11, stage - 11, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);
		if (ret != 16)
			return ret;

		M68K::cpu_regs.ead = M68K::cpu_regs.eat;
		stage = 16;
	case 16:
		if ((operands[1] & 0x38) == 0x00)
		{
			if (M68K::cpu_regs.len == 4)
				return 17;						/* Data register direct long costs 1 cycles more */
		}
		break;
	case 17:
		break;
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_ANDI(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	OPCODE_SETUP_LENGTH(operands[0]);
	cycles++;

	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, M68K::cpu_regs.len, 1);
	cycles += LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
	cycles += COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);
	cycles += LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);

	M68K::cpu_regs.ear = (M68K::cpu_regs.eat & M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

	if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
		cycles++;							/* Long reg direct takes 1 additional cycle on read and write */

	cycles += STORE_EFFECTIVE_VALUE(26, 21, stage - 21, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);

	M68K::cpu_regs.ead = M68K::cpu_regs.eat;

	if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
		cycles++;							/* Long reg direct takes 1 additional cycle on read and write */

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		OPCODE_SETUP_LENGTH(operands[0]);
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, M68K::cpu_regs.len, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, M68K::cpu_regs.len);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[1], M68K::cpu_regs.len, 0);

		if (ret != 16)
			return ret;

		stage = 16;			/* Fall through */
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
		ret = LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[1], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 21)
			return ret;

		M68K::cpu_regs.ear = (M68K::cpu_regs.eat & M68K::cpu_regs.eas)&M68K::cpu_regs.zMask;

		if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
			return 21;							/* Long reg direct takes 1 additional cycle on read and write */

		stage = 21;			/* Fall through */
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
		ret = STORE_EFFECTIVE_VALUE(26, 21, stage - 21, operands[1], &M68K::cpu_regs.ead, &M68K::cpu_regs.ear);
		if (ret != 26)
			return ret;

		M68K::cpu_regs.ead = M68K::cpu_regs.eat;

		if ((operands[1] & 0x38) == 0 && M68K::cpu_regs.len == 4)
			return 26;							/* Long reg direct takes 1 additional cycle on read and write */
		break;
	case 26:
		break;
	}
	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	COMPUTE_ZN_TESTS(M68K::cpu_regs.ear);
	return 0;
#endif
}

static inline U32 CPU_EXG(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	switch (operands[1])
	{
	case 0x08:	/* Data */
	case 0x11:
		M68K::cpu_regs.tmpL = M68K::cpu_regs.D[operands[0]];
		break;
	case 0x09:	/* Address */
		M68K::cpu_regs.tmpL = M68K::cpu_regs.A[operands[0]];
		break;
	}
	cycles++;

	switch (operands[1])
	{
	case 0x08:	/* Data & Data */
		M68K::cpu_regs.D[operands[0]] = M68K::cpu_regs.D[operands[2]];
		break;
	case 0x09:	/* Address & Address */
		M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.A[operands[2]];
		break;
	case 0x11:	/* Data & Address */
		M68K::cpu_regs.D[operands[0]] = M68K::cpu_regs.A[operands[2]];
		break;
	}
	cycles++;

	switch (operands[1])
	{
	case 0x08:	/* Data */
		M68K::cpu_regs.D[operands[2]] = M68K::cpu_regs.tmpL;
		break;
	case 0x09:	/* Address */
	case 0x11:
		M68K::cpu_regs.A[operands[2]] = M68K::cpu_regs.tmpL;
		break;
	}
	cycles++;

	return cycles;
#else
	switch (stage)
	{
	case 0:
		switch (operands[1])
		{
		case 0x08:	/* Data */
		case 0x11:
			M68K::cpu_regs.tmpL = M68K::cpu_regs.D[operands[0]];
			break;
		case 0x09:	/* Address */
			M68K::cpu_regs.tmpL = M68K::cpu_regs.A[operands[0]];
			break;
		}
		return 1;
	case 1:
		switch (operands[1])
		{
		case 0x08:	/* Data & Data */
			M68K::cpu_regs.D[operands[0]] = M68K::cpu_regs.D[operands[2]];
			break;
		case 0x09:	/* Address & Address */
			M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.A[operands[2]];
			break;
		case 0x11:	/* Data & Address */
			M68K::cpu_regs.D[operands[0]] = M68K::cpu_regs.A[operands[2]];
			break;
		}
		return 2;
	case 2:
		switch (operands[1])
		{
		case 0x08:	/* Data */
			M68K::cpu_regs.D[operands[2]] = M68K::cpu_regs.tmpL;
			break;
		case 0x09:	/* Address */
		case 0x11:
			M68K::cpu_regs.A[operands[2]] = M68K::cpu_regs.tmpL;
			break;
		}
		break;
	}

	return 0;
#endif
}

static inline U32 CPU_JSR(U32 stage, U16* operands)
{
#if CPU_COMBINE_STAGES
	int cycles = 0;

	cycles++;
	cycles += COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[0], 4, 1);
	cycles += FUDGE_EA_CYCLES(9, 6, stage - 6, operands[0]);		/* ODD cycle timings for JSR -this rebalances */
	cycles += PUSH_VALUE(14, 9, stage - 9, M68K::cpu_regs.PC, 4);

	M68K::cpu_regs.PC = M68K::cpu_regs.eas;
	cycles++;
	return cycles;
#else
	U32 ret;

	switch (stage)
	{
	case 0:
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, operands[0], 4, 1);
		if (ret != 6)
			return ret;
		stage = 6;		/* Fall through */
	case 6:
	case 7:
	case 8:
		ret = FUDGE_EA_CYCLES(9, 6, stage - 6, operands[0]);		/* ODD cycle timings for JSR -this rebalances */
		if (ret != 9)
			return ret;

		stage = 9;		/* Fall through */
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
		ret = PUSH_VALUE(14, 9, stage - 9, M68K::cpu_regs.PC, 4);
		if (ret != 14)
			return ret;
		break;
	}

	M68K::cpu_regs.PC = M68K::cpu_regs.eas;
	return 0;
#endif
}

static inline U32 CPU_BCLRI(U32 stage, U16* operands)
{
#if 0
	I AM HERE - NEED TO TIME INSTRUCTION ON REAL AMIGA FOR CASE OF VARIOUS BIT VALUES FOR REGISTER DIRECT MODE
		U32 ret;
	switch (stage)
	{
	case 0:
		if (operands[0]<0x08)
			OPCODE_SETUP_LENGTH(2);			/* Register direct uses long */
		else
			OPCODE_SETUP_LENGTH(0);			/* memory uses byte */
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		ret = COMPUTE_EFFECTIVE_ADDRESS(6, 1, stage - 1, &M68K::cpu_regs.eas, 0x3C, 1, 1);
		if (ret != 6)
			return ret;

		stage = 6;			/* Fall through */
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		ret = LOAD_EFFECTIVE_VALUE(11, 6, stage - 6, 0x3C, &M68K::cpu_regs.eas, &M68K::cpu_regs.eas, 1);
		if (ret != 11)
			return ret;

		stage = 11;			/* Fall through */
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = COMPUTE_EFFECTIVE_ADDRESS(16, 11, stage - 11, &M68K::cpu_regs.ead, operands[0], M68K::cpu_regs.len, 0);

		if (ret != 16)
			return ret;

		stage = 16;			/* Fall through */
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
		ret = LOAD_EFFECTIVE_VALUE(21, 16, stage - 16, operands[0], &M68K::cpu_regs.eat, &M68K::cpu_regs.ead, M68K::cpu_regs.len);
		if (ret != 21)
			return ret;

		stage = 21;			/* Fall through */
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
		ret = STORE_EFFECTIVE_VALUE(26, 21, stage - 21, operands[0], &M68K::cpu_regs.ead, &M68K::cpu_regs.eat);
		if (ret != 26)
			return ret;

		stage = 26;			/* Fall through */
	case 21:
		if (operands[0]<0x08)
			M68K::cpu_regs.eas &= 0x1F;
		else
			M68K::cpu_regs.eas &= 0x07;
		M68K::cpu_regs.eas = 1 << M68K::cpu_regs.eas;
		break;
	}

	COMPUTE_Z_BIT(M68K::cpu_regs.eas, M68K::cpu_regs.ead);
	return 0;
#endif
	int len;
	U32 ead, eas, eat;

	UNUSED_ARGUMENT(stage);

	len = 1;
	eas = MEM_getByte(M68K::cpu_regs.PC + 1);
	M68K::cpu_regs.PC += 2;

	ead = getEffectiveAddress(operands[0], len);
	if (operands[0]<8)
	{
		eas &= 0x1F;
		eas = 1 << eas;

		if (ead & eas)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;

		M68K::cpu_regs.D[operands[0]] &= ~(eas);
	}
	else
	{
		eat = MEM_getByte(ead);
		eas &= 0x07;
		eas = 1 << eas;

		if (eat & eas)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;

		eat &= ~(eas);
		MEM_setByte(ead, eat);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ANDs(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ead = M68K::cpu_regs.D[operands[0]] & zMask;
	eas = getSourceEffectiveAddress(operands[2], len);
	ear = (ead&eas)&zMask;

	M68K::cpu_regs.D[operands[0]] &= ~zMask;
	M68K::cpu_regs.D[operands[0]] |= ear;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;
	eas &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_V);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_SUBd(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear, eat;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ead = M68K::cpu_regs.D[operands[0]] & zMask;
	eat = getEffectiveAddress(operands[2], len);
	switch (len)
	{
	case 1:
		eas = MEM_getByte(eat);
		ear = (eas - M68K::cpu_regs.D[operands[0]])&zMask;
		MEM_setByte(eat, ear);
		break;
	case 2:
		eas = MEM_getWord(eat);
		ear = (eas - M68K::cpu_regs.D[operands[0]])&zMask;
		MEM_setWord(eat, ear);
		break;
	case 4:
		eas = MEM_getLong(eat);
		ear = (eas - M68K::cpu_regs.D[operands[0]])&zMask;
		MEM_setLong(eat, ear);
		break;
	default:
		eas = ear = 0;
		SOFT_BREAK;	/* should not get here */
		break;
	}

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;
	eas &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_BSET(U32 stage, U16* operands)
{
	int len;
	U32 ead, eas, eat;

	UNUSED_ARGUMENT(stage);

	len = 1;

	eas = M68K::cpu_regs.D[operands[0]];
	if (operands[1]<8)
	{
		eas &= 0x1F;
		eas = 1 << eas;
		ead = M68K::cpu_regs.D[operands[1]];

		if (ead & eas)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;

		M68K::cpu_regs.D[operands[1]] |= eas;
	}
	else
	{
		ead = getEffectiveAddress(operands[1], len);
		eat = MEM_getByte(ead);
		eas &= 0x07;
		eas = 1 << eas;

		if (eat & eas)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;

		eat |= eas;
		MEM_setByte(ead, eat);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_BSETI(U32 stage, U16* operands)
{
	int len;
	U32 ead, eas, eat;

	UNUSED_ARGUMENT(stage);

	len = 1;
	eas = MEM_getByte(M68K::cpu_regs.PC + 1);
	M68K::cpu_regs.PC += 2;

	if (operands[0]<8)
	{
		eas &= 0x1F;
		eas = 1 << eas;
		ead = M68K::cpu_regs.D[operands[0]];

		if (ead & eas)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;

		M68K::cpu_regs.D[operands[0]] |= eas;
	}
	else
	{
		ead = getEffectiveAddress(operands[0], len);
		eat = MEM_getByte(ead);
		eas &= 0x07;
		eas = 1 << eas;

		if (eat & eas)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;

		eat |= eas;
		MEM_setByte(ead, eat);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MULU(U32 stage, U16* operands)
{
	int len;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	len = 2;

	ead = M68K::cpu_regs.D[operands[0]] & 0xFFFF;
	eas = getSourceEffectiveAddress(operands[1], len) & 0xFFFF;
	ear = eas * ead;

	M68K::cpu_regs.D[operands[0]] = ear;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= 0x80000000;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_LSL(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (operands[2] == 0)
	{
		if (operands[0] == 0)
			operands[0] = 8;
	}
	else
	{
		operands[0] = M68K::cpu_regs.D[operands[0]] & 0x3F;
	}

	eas = M68K::cpu_regs.D[operands[3]] & zMask;
	if (operands[0]>31)								/* Its like this because the processor is modulo 64 on shifts and >31 with uint32 is not doable in c */
	{
		ead = 0;
		M68K::cpu_regs.D[operands[3]] &= ~zMask;
		M68K::cpu_regs.D[operands[3]] |= ead;
		M68K::cpu_regs.SR &= ~(CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_X);
		M68K::cpu_regs.SR |= CPU_STATUS_Z;
		if ((eas == 1) && (operands[0] == 32) && (operands[1] == 2))			/* the one case where carry can occur */
			M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		ead = (eas << operands[0])&zMask;
		M68K::cpu_regs.D[operands[3]] &= ~zMask;
		M68K::cpu_regs.D[operands[3]] |= ead;

		if (operands[0] == 0)
		{
			M68K::cpu_regs.SR &= ~CPU_STATUS_C;
		}
		else
		{
			if (eas&(nMask >> (operands[0] - 1)))
			{
				M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
			}
			else
			{
				M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
			}
		}
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;
		if (M68K::cpu_regs.D[operands[3]] & nMask)
			M68K::cpu_regs.SR |= CPU_STATUS_N;
		else
			M68K::cpu_regs.SR &= ~CPU_STATUS_N;
		if (M68K::cpu_regs.D[operands[3]] & zMask)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ADDI(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear, eat;

	UNUSED_ARGUMENT(stage);

	switch (operands[0])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		eas = MEM_getByte(M68K::cpu_regs.PC + 1);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		eas = MEM_getWord(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		eas = MEM_getLong(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 4;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		eas = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if ((operands[1] & 0x38) == 0)	/* destination is D register */
	{
		ead = M68K::cpu_regs.D[operands[1]] & zMask;
		ear = (ead + eas)&zMask;
		M68K::cpu_regs.D[operands[1]] &= ~zMask;
		M68K::cpu_regs.D[operands[1]] |= ear;
	}
	else
	{
		ead = getEffectiveAddress(operands[1], len);
		switch (len)
		{
		case 1:
			eat = MEM_getByte(ead);
			ear = (eat + eas)&zMask;
			MEM_setByte(ead, ear&zMask);
			ead = eat;
			break;
		case 2:
			eat = MEM_getWord(ead);
			ear = (eat + eas)&zMask;
			MEM_setWord(ead, ear&zMask);
			ead = eat;
			break;
		case 4:
			eat = MEM_getLong(ead);
			ear = (eat + eas)&zMask;
			MEM_setLong(ead, ear&zMask);
			ead = eat;
			break;
		default:
			ear = 0;
			SOFT_BREAK;		/* should never get here */
			break;
		}
	}

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;
	eas &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_EXT(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	switch (operands[0])
	{
	case 0x02:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		eas = (S8)(M68K::cpu_regs.D[operands[1]] & 0xFF);
		break;
	case 0x03:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		eas = (S16)(M68K::cpu_regs.D[operands[1]] & 0xFFFF);
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		eas = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ead = eas & zMask;
	M68K::cpu_regs.D[operands[1]] &= ~zMask;
	M68K::cpu_regs.D[operands[1]] |= ead;

	if (M68K::cpu_regs.D[operands[1]] & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (M68K::cpu_regs.D[operands[1]] & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;
	M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_V);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MULS(U32 stage, U16* operands)
{
	int len;
	S32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	len = 2;

	ead = (S16)(M68K::cpu_regs.D[operands[0]] & 0xFFFF);
	eas = (S16)(getSourceEffectiveAddress(operands[1], len) & 0xFFFF);
	ear = eas * ead;

	M68K::cpu_regs.D[operands[0]] = ear;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= 0x80000000;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_NEG(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[0])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if ((operands[1] & 0x38) == 0)	/* destination is D register */
	{
		ead = M68K::cpu_regs.D[operands[1]] & zMask;
		ear = (0 - ead)&zMask;
		M68K::cpu_regs.D[operands[1]] &= ~zMask;
		M68K::cpu_regs.D[operands[1]] |= ear;
	}
	else
	{
		eas = getEffectiveAddress(operands[1], len);
		switch (len)
		{
		case 1:
			ead = MEM_getByte(eas);
			ear = (0 - ead)&zMask;
			MEM_setByte(eas, ear);
			break;
		case 2:
			ead = MEM_getWord(eas);
			ear = (0 - ead)&zMask;
			MEM_setWord(eas, ear);
			break;
		case 4:
			ead = MEM_getLong(eas);
			ear = (0 - ead)&zMask;
			MEM_setLong(eas, ear);
			break;
		default:
			ead = ear = 0;
			SOFT_BREAK;		/* should never get here */
			break;
		}
	}

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if (ear & ead)
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

	if (ear | ead)
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MOVEUSP(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_S)
	{
		if (operands[0])
		{
			M68K::cpu_regs.A[operands[1]] = M68K::cpu_regs.USP;
		}
		else
		{
			M68K::cpu_regs.USP = M68K::cpu_regs.A[operands[1]];
		}
	}
	else
	{
		M68K::cpu_regs.PC -= 2;
		CPU_GENERATE_EXCEPTION(0x20);
	}
#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_SCC(U32 stage, U16* operands)
{
	int cc = 0;
	U32 eas, ead;
	U8 value;

	UNUSED_ARGUMENT(stage);

	switch (operands[0])
	{
	case 0x00:
		cc = 1;
		break;
	case 0x01:
		cc = 0;
		break;
	case 0x02:
		cc = ((~M68K::cpu_regs.SR)&CPU_STATUS_C) && ((~M68K::cpu_regs.SR)&CPU_STATUS_Z);
		break;
	case 0x03:
		cc = (M68K::cpu_regs.SR & CPU_STATUS_C) || (M68K::cpu_regs.SR & CPU_STATUS_Z);
		break;
	case 0x04:
		cc = !(M68K::cpu_regs.SR & CPU_STATUS_C);
		break;
	case 0x05:
		cc = (M68K::cpu_regs.SR & CPU_STATUS_C);
		break;
	case 0x06:
		cc = !(M68K::cpu_regs.SR & CPU_STATUS_Z);
		break;
	case 0x07:
		cc = (M68K::cpu_regs.SR & CPU_STATUS_Z);
		break;
	case 0x08:
		cc = !(M68K::cpu_regs.SR & CPU_STATUS_V);
		break;
	case 0x09:
		cc = (M68K::cpu_regs.SR & CPU_STATUS_V);
		break;
	case 0x0A:
		cc = !(M68K::cpu_regs.SR & CPU_STATUS_N);
		break;
	case 0x0B:
		cc = (M68K::cpu_regs.SR & CPU_STATUS_N);
		break;
	case 0x0C:
		cc = ((M68K::cpu_regs.SR & CPU_STATUS_N) && (M68K::cpu_regs.SR & CPU_STATUS_V)) || ((!(M68K::cpu_regs.SR & CPU_STATUS_N)) && (!(M68K::cpu_regs.SR & CPU_STATUS_V)));
		break;
	case 0x0D:
		cc = ((M68K::cpu_regs.SR & CPU_STATUS_N) && (!(M68K::cpu_regs.SR & CPU_STATUS_V))) || ((!(M68K::cpu_regs.SR & CPU_STATUS_N)) && (M68K::cpu_regs.SR & CPU_STATUS_V));
		break;
	case 0x0E:
		cc = ((M68K::cpu_regs.SR & CPU_STATUS_N) && (M68K::cpu_regs.SR & CPU_STATUS_V) && (!(M68K::cpu_regs.SR & CPU_STATUS_Z))) || ((!(M68K::cpu_regs.SR & CPU_STATUS_N)) && (!(M68K::cpu_regs.SR & CPU_STATUS_V)) && (!(M68K::cpu_regs.SR & CPU_STATUS_Z)));
		break;
	case 0x0F:
		cc = (M68K::cpu_regs.SR & CPU_STATUS_Z) || ((M68K::cpu_regs.SR & CPU_STATUS_N) && (!(M68K::cpu_regs.SR & CPU_STATUS_V))) || ((!(M68K::cpu_regs.SR & CPU_STATUS_N)) && (M68K::cpu_regs.SR & CPU_STATUS_V));
		break;
	}

	if (cc)
		value = 0xFF;
	else
		value = 0x00;

	if ((operands[1] & 0x38) == 0)	/* destination is D register */
	{
		M68K::cpu_regs.D[operands[1]] &= ~0xFF;
		M68K::cpu_regs.D[operands[1]] |= value;
	}
	else
	{
		eas = getEffectiveAddress(operands[1], 1);
		ead = MEM_getByte(eas);
		MEM_setByte(eas, value);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ORSR(U32 stage, U16* operands)
{
	U16 oldSR;

	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR = M68K::cpu_regs.SR;
		M68K::cpu_regs.SR |= MEM_getWord(M68K::cpu_regs.PC);
		CPU_CHECK_SP(oldSR, M68K::cpu_regs.SR);
		M68K::cpu_regs.PC += 2;
	}
	else
	{
		M68K::cpu_regs.PC -= 2;
		CPU_GENERATE_EXCEPTION(0x20);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_PEA(U32 stage, U16* operands)
{
	U32 ear;

	UNUSED_ARGUMENT(stage);

	ear = getEffectiveAddress(operands[0], 4);

	M68K::cpu_regs.A[7] -= 4;
	MEM_setLong(M68K::cpu_regs.A[7], ear);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

/* NB: This is oddly not a supervisor instruction on MC68000 */
static inline U32 CPU_MOVEFROMSR(U32 stage, U16* operands)
{
	U32 ear;

	UNUSED_ARGUMENT(stage);

	if ((operands[0] & 0x38) == 0)	/* destination is D register */
	{
		M68K::cpu_regs.D[operands[0]] &= ~0xFFFF;
		M68K::cpu_regs.D[operands[0]] |= M68K::cpu_regs.SR;
	}
	else
	{
		ear = getEffectiveAddress(operands[0], 2);
		MEM_getWord(ear);					/* processor reads address before writing */
		MEM_setWord(ear, M68K::cpu_regs.SR);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_RTE(U32 stage, U16* operands)
{
	U16 oldSR;

	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR = M68K::cpu_regs.SR;

		M68K::cpu_regs.SR = MEM_getWord(M68K::cpu_regs.A[7]);
		M68K::cpu_regs.A[7] += 2;
		M68K::cpu_regs.PC = MEM_getLong(M68K::cpu_regs.A[7]);
		M68K::cpu_regs.A[7] += 4;

		CPU_CHECK_SP(oldSR, M68K::cpu_regs.SR);
	}
	else
	{
		M68K::cpu_regs.PC -= 2;
		CPU_GENERATE_EXCEPTION(0x20);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ANDSR(U32 stage, U16* operands)
{
	U16 oldSR;

	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR = M68K::cpu_regs.SR;
		M68K::cpu_regs.SR &= MEM_getWord(M68K::cpu_regs.PC);
		CPU_CHECK_SP(oldSR, M68K::cpu_regs.SR);
		M68K::cpu_regs.PC += 2;
	}
	else
	{
		M68K::cpu_regs.PC -= 2;
		CPU_GENERATE_EXCEPTION(0x20);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MOVETOSR(U32 stage, U16* operands)
{
	U16 oldSR;
	U32 ear;

	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_S)
	{
		ear = getSourceEffectiveAddress(operands[0], 2);
		oldSR = M68K::cpu_regs.SR;
		M68K::cpu_regs.SR = ear;
		CPU_CHECK_SP(oldSR, M68K::cpu_regs.SR);
	}
	else
	{
		M68K::cpu_regs.PC -= 2;
		CPU_GENERATE_EXCEPTION(0x20);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_LINK(U32 stage, U16* operands)
{
	U32 ear;

	UNUSED_ARGUMENT(stage);

	ear = (S16)MEM_getWord(M68K::cpu_regs.PC);
	M68K::cpu_regs.PC += 2;

	M68K::cpu_regs.A[7] -= 4;
	MEM_setLong(M68K::cpu_regs.A[7], M68K::cpu_regs.A[operands[0]]);

	M68K::cpu_regs.A[operands[0]] = M68K::cpu_regs.A[7];
	M68K::cpu_regs.A[7] += ear;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_CMPM(U32 stage, U16* operands)
{
	U32 zMask, nMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		zMask = 0xFF;
		nMask = 0x80;
		ead = MEM_getByte(M68K::cpu_regs.A[operands[0]]);
		eas = MEM_getByte(M68K::cpu_regs.A[operands[2]]);
		M68K::cpu_regs.A[operands[0]] += 1;
		M68K::cpu_regs.A[operands[2]] += 1;
		break;
	case 0x01:
		zMask = 0xFFFF;
		nMask = 0x8000;
		ead = MEM_getWord(M68K::cpu_regs.A[operands[0]]);
		eas = MEM_getWord(M68K::cpu_regs.A[operands[2]]);
		M68K::cpu_regs.A[operands[0]] += 2;
		M68K::cpu_regs.A[operands[2]] += 2;
		break;
	case 0x02:
		zMask = 0xFFFFFFFF;
		nMask = 0x80000000;
		ead = MEM_getLong(M68K::cpu_regs.A[operands[0]]);
		eas = MEM_getLong(M68K::cpu_regs.A[operands[2]]);
		M68K::cpu_regs.A[operands[0]] += 4;
		M68K::cpu_regs.A[operands[2]] += 4;
		break;
	default:
		nMask = 0;
		zMask = 0;
		ead = eas = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ear = (ead - eas)&zMask;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;
	eas &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_C;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_C;
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ADDd(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear, eat;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ead = M68K::cpu_regs.D[operands[0]] & zMask;
	eat = getEffectiveAddress(operands[2], len);
	switch (len)
	{
	case 1:
		eas = MEM_getByte(eat);
		ear = (M68K::cpu_regs.D[operands[0]] + eas)&zMask;
		MEM_setByte(eat, ear);
		break;
	case 2:
		eas = MEM_getWord(eat);
		ear = (M68K::cpu_regs.D[operands[0]] + eas)&zMask;
		MEM_setWord(eat, ear);
		break;
	case 4:
		eas = MEM_getLong(eat);
		ear = (M68K::cpu_regs.D[operands[0]] + eas)&zMask;
		MEM_setLong(eat, ear);
		break;
	default:
		eas = ear = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;
	eas &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_UNLK(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

	M68K::cpu_regs.A[7] = M68K::cpu_regs.A[operands[0]];
	M68K::cpu_regs.A[operands[0]] = MEM_getLong(M68K::cpu_regs.A[7]);
	M68K::cpu_regs.A[7] += 4;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ORs(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ead = M68K::cpu_regs.D[operands[0]] & zMask;
	eas = getSourceEffectiveAddress(operands[2], len);
	ear = (ead | eas)&zMask;

	M68K::cpu_regs.D[operands[0]] &= ~zMask;
	M68K::cpu_regs.D[operands[0]] |= ear;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;
	eas &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_V);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ANDd(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ead = getEffectiveAddress(operands[2], len);
	switch (len)
	{
	case 1:
		eas = MEM_getByte(ead);
		ear = (M68K::cpu_regs.D[operands[0]] & eas)&zMask;
		MEM_setByte(ead, ear);
		break;
	case 2:
		eas = MEM_getWord(ead);
		ear = (M68K::cpu_regs.D[operands[0]] & eas)&zMask;
		MEM_setWord(ead, ear);
		break;
	case 4:
		eas = MEM_getLong(ead);
		ear = (M68K::cpu_regs.D[operands[0]] & eas)&zMask;
		MEM_setLong(ead, ear);
		break;
	default:
		ear = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	M68K::cpu_regs.SR &= ~CPU_STATUS_C;
	M68K::cpu_regs.SR &= ~CPU_STATUS_V;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ORI(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[0])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		eas = MEM_getByte(M68K::cpu_regs.PC + 1);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		eas = MEM_getWord(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		eas = MEM_getLong(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 4;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		eas = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if ((operands[1] & 0x38) == 0)	/* destination is D register */
	{
		ead = M68K::cpu_regs.D[operands[1]] & zMask;
		ear = (ead | eas)&zMask;
		M68K::cpu_regs.D[operands[1]] &= ~zMask;
		M68K::cpu_regs.D[operands[1]] |= ear;
	}
	else
	{
		ead = getEffectiveAddress(operands[1], len);
		switch (len)
		{
		case 1:
			ear = (MEM_getByte(ead) | eas)&zMask;
			MEM_setByte(ead, ear&zMask);
			break;
		case 2:
			ear = (MEM_getWord(ead) | eas)&zMask;
			MEM_setWord(ead, ear&zMask);
			break;
		case 4:
			ear = (MEM_getLong(ead) | eas)&zMask;
			MEM_setLong(ead, ear&zMask);
			break;
		default:
			ear = 0;
			SOFT_BREAK;		/* should never get here */
			break;
		}
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ASL(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (operands[2] == 0)
	{
		if (operands[0] == 0)
			operands[0] = 8;
	}
	else
	{
		operands[0] = M68K::cpu_regs.D[operands[0]] & 0x3F;
	}

	eas = M68K::cpu_regs.D[operands[3]] & zMask;
	if (operands[0]>31)								/* Its like this because the processor is modulo 64 on shifts and >31 with uint32 is not doable in c */
	{
		ead = 0;
		M68K::cpu_regs.D[operands[3]] &= ~zMask;
		M68K::cpu_regs.D[operands[3]] |= ead;
		M68K::cpu_regs.SR &= ~(CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_X);
		M68K::cpu_regs.SR |= CPU_STATUS_Z;
		if ((eas == 1) && (operands[0] == 32) && (operands[1] == 2))			/* the one case where carry can occur */
			M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
		if (eas != 0)
			M68K::cpu_regs.SR |= CPU_STATUS_V;
	}
	else
	{
		ead = (eas << operands[0])&zMask;

		M68K::cpu_regs.D[operands[3]] &= ~zMask;
		M68K::cpu_regs.D[operands[3]] |= ead;

		if (operands[0] == 0)
		{
			M68K::cpu_regs.SR &= ~CPU_STATUS_C;
		}
		else
		{
			if (eas&(nMask >> (operands[0] - 1)))
			{
				M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
			}
			else
			{
				M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
			}
		}

		M68K::cpu_regs.SR &= ~CPU_STATUS_V;
		while (operands[0])							/* This is rubbish, can't think of a better test at present though */
		{
			if ((eas & nMask) ^ ((eas & (nMask >> operands[0])) << operands[0]))
			{
				M68K::cpu_regs.SR |= CPU_STATUS_V;
				break;
			}

			operands[0]--;
		};

		if (M68K::cpu_regs.D[operands[3]] & nMask)
			M68K::cpu_regs.SR |= CPU_STATUS_N;
		else
			M68K::cpu_regs.SR &= ~CPU_STATUS_N;
		if (M68K::cpu_regs.D[operands[3]] & zMask)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ASR(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (operands[2] == 0)
	{
		if (operands[0] == 0)
			operands[0] = 8;
	}
	else
	{
		operands[0] = M68K::cpu_regs.D[operands[0]] & 0x3F;
	}

	eas = M68K::cpu_regs.D[operands[3]] & zMask;
	if (operands[0]>31)								/* Its like this because the processor is modulo 64 on shifts and >31 with uint32 is not doable in c */
	{
		if (eas&nMask)
		{
			ead = 0xFFFFFFFF & zMask;
			M68K::cpu_regs.D[operands[3]] &= ~zMask;
			M68K::cpu_regs.D[operands[3]] |= ead;
			M68K::cpu_regs.SR &= ~(CPU_STATUS_Z | CPU_STATUS_V);
			M68K::cpu_regs.SR |= CPU_STATUS_N | CPU_STATUS_X | CPU_STATUS_C;
		}
		else
		{
			ead = 0;
			M68K::cpu_regs.D[operands[3]] &= ~zMask;
			M68K::cpu_regs.D[operands[3]] |= ead;
			M68K::cpu_regs.SR &= ~(CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_X | CPU_STATUS_C);
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
		}
	}
	else
	{
		ead = (eas >> operands[0])&zMask;
		if ((eas&nMask) && operands[0])
		{
			eas |= 0xFFFFFFFF & (~zMask);		/* correct eas for later carry test */
			ear = (nMask >> (operands[0] - 1));
			if (ear)
			{
				ead |= (~(ear - 1))&zMask;		/* set sign bits */
			}
			else
			{
				ead |= 0xFFFFFFFF & zMask;		/* set sign bits */
			}
		}
		M68K::cpu_regs.D[operands[3]] &= ~zMask;
		M68K::cpu_regs.D[operands[3]] |= ead;

		if (operands[0] == 0)
		{
			M68K::cpu_regs.SR &= ~CPU_STATUS_C;
		}
		else
		{
			if (eas&(1 << (operands[0] - 1)))
			{
				M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
			}
			else
			{
				M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
			}
		}

		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

		if (M68K::cpu_regs.D[operands[3]] & nMask)
			M68K::cpu_regs.SR |= CPU_STATUS_N;
		else
			M68K::cpu_regs.SR &= ~CPU_STATUS_N;
		if (M68K::cpu_regs.D[operands[3]] & zMask)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_DIVU(U32 stage, U16* operands)
{
	int len;
	U32 ead, eas, eaq, ear;

	UNUSED_ARGUMENT(stage);

	len = 2;

	ead = M68K::cpu_regs.D[operands[0]];
	eas = getSourceEffectiveAddress(operands[1], len) & 0xFFFF;

	M68K::cpu_regs.SR &= ~CPU_STATUS_C;			/* Carry is always cleared */

	if (!eas)
	{
		CPU_GENERATE_EXCEPTION(0x14);
		return 0;
	}

	eaq = ead / eas;
	ear = ead % eas;

	if (eaq>65535)
	{
		M68K::cpu_regs.SR |= CPU_STATUS_V | CPU_STATUS_N;		/* MC68000 docs claim N and Z are undefined. however real amiga seems to set N if V happens, Z is cleared */
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	}
	else
	{
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

		M68K::cpu_regs.D[operands[0]] = (eaq & 0xFFFF) | ((ear << 16) & 0xFFFF0000);

		if (eaq & 0x8000)
			M68K::cpu_regs.SR |= CPU_STATUS_N;
		else
			M68K::cpu_regs.SR &= ~CPU_STATUS_N;

		if (eaq & 0xFFFF)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_BCLR(U32 stage, U16* operands)
{
	int len;
	U32 ead, eas, eat;

	UNUSED_ARGUMENT(stage);

	len = 1;

	eas = M68K::cpu_regs.D[operands[0]];
	if (operands[1]<8)
	{
		ead = M68K::cpu_regs.D[operands[1]];
		eas &= 0x1F;
		eas = 1 << eas;

		if (ead & eas)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;

		M68K::cpu_regs.D[operands[1]] &= ~eas;
	}
	else
	{
		ead = getEffectiveAddress(operands[1], len);
		eat = MEM_getByte(ead);
		eas &= 0x07;
		eas = 1 << eas;

		if (eat & eas)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;

		eat &= ~eas;
		MEM_setByte(ead, eat);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_EORd(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if ((operands[2] & 0x38) == 0)	/* destination is D register */
	{
		ead = M68K::cpu_regs.D[operands[2]] & zMask;
		ear = (ead ^ M68K::cpu_regs.D[operands[0]])&zMask;
		M68K::cpu_regs.D[operands[2]] &= ~zMask;
		M68K::cpu_regs.D[operands[2]] |= ear;
	}
	else
	{
		ead = getEffectiveAddress(operands[2], len);
		switch (len)
		{
		case 1:
			eas = MEM_getByte(ead);
			ear = (M68K::cpu_regs.D[operands[0]] ^ eas)&zMask;
			MEM_setByte(ead, ear);
			break;
		case 2:
			eas = MEM_getWord(ead);
			ear = (M68K::cpu_regs.D[operands[0]] ^ eas)&zMask;
			MEM_setWord(ead, ear);
			break;
		case 4:
			eas = MEM_getLong(ead);
			ear = (M68K::cpu_regs.D[operands[0]] ^ eas)&zMask;
			MEM_setLong(ead, ear);
			break;
		default:
			ear = 0;
			SOFT_BREAK;		/* should never get here */
			break;
		}
	}

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	M68K::cpu_regs.SR &= ~CPU_STATUS_C;
	M68K::cpu_regs.SR &= ~CPU_STATUS_V;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_BTST(U32 stage, U16* operands)
{
	int len;
	U32 ead, eas;

	UNUSED_ARGUMENT(stage);

	len = 1;
	eas = M68K::cpu_regs.D[operands[0]];

	ead = getSourceEffectiveAddress(operands[1], len);
	if (operands[1]<8)
		eas &= 0x1F;
	else
		eas &= 0x07;

	eas = 1 << eas;

	if (ead & eas)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_STOP(U32 stage, U16* operands)
{
	U16 oldSR;

	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR = M68K::cpu_regs.SR;
		M68K::cpu_regs.SR = MEM_getWord(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 2;

		CPU_CHECK_SP(oldSR, M68K::cpu_regs.SR);

		M68K::cpu_regs.stopped = 1;
	}
	else
	{
		M68K::cpu_regs.PC -= 2;
		CPU_GENERATE_EXCEPTION(0x20);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ROL(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (operands[2] == 0)
	{
		if (operands[0] == 0)
			operands[0] = 8;
	}
	else
	{
		operands[0] = M68K::cpu_regs.D[operands[0]] & 0x3F;
	}

	eas = M68K::cpu_regs.D[operands[3]] & zMask;
	ead = eas;
	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	while (operands[0])
	{
		if (ead & nMask)
		{
			ead <<= 1;
			ead |= 1;
			ead &= zMask;
			M68K::cpu_regs.SR |= CPU_STATUS_C;
		}
		else
		{
			ead <<= 1;
			ead &= zMask;
			M68K::cpu_regs.SR &= ~CPU_STATUS_C;
		}
		operands[0]--;
	}
	M68K::cpu_regs.D[operands[3]] &= ~zMask;
	M68K::cpu_regs.D[operands[3]] |= ead;

	if (M68K::cpu_regs.D[operands[3]] & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (M68K::cpu_regs.D[operands[3]] & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ROR(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (operands[2] == 0)
	{
		if (operands[0] == 0)
			operands[0] = 8;
	}
	else
	{
		operands[0] = M68K::cpu_regs.D[operands[0]] & 0x3F;
	}

	eas = M68K::cpu_regs.D[operands[3]] & zMask;
	ead = eas;
	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);
	while (operands[0])
	{
		if (ead & 0x01)
		{
			ead >>= 1;
			ead |= nMask;
			ead &= zMask;
			M68K::cpu_regs.SR |= CPU_STATUS_C;
		}
		else
		{
			ead >>= 1;
			ead &= ~nMask;
			ead &= zMask;
			M68K::cpu_regs.SR &= ~CPU_STATUS_C;
		}
		operands[0]--;
	}
	M68K::cpu_regs.D[operands[3]] &= ~zMask;
	M68K::cpu_regs.D[operands[3]] |= ead;

	if (M68K::cpu_regs.D[operands[3]] & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (M68K::cpu_regs.D[operands[3]] & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_NOP(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_BCHG(U32 stage, U16* operands)
{
	int len;
	U32 ead, eas, eat;

	UNUSED_ARGUMENT(stage);

	len = 1;

	eas = M68K::cpu_regs.D[operands[0]];
	if (operands[1]<8)
	{
		ead = M68K::cpu_regs.D[operands[1]];
		eas &= 0x1F;
		eas = 1 << eas;

		if (ead & eas)
		{
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
			M68K::cpu_regs.D[operands[1]] &= ~eas;
		}
		else
		{
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
			M68K::cpu_regs.D[operands[1]] |= eas;
		}
	}
	else
	{
		ead = getEffectiveAddress(operands[1], len);
		eat = MEM_getByte(ead);
		eas &= 0x07;
		eas = 1 << eas;

		if (eat & eas)
		{
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
			eat &= ~eas;
		}
		else
		{
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
			eat |= eas;
		}

		MEM_setByte(ead, eat);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_BCHGI(U32 stage, U16* operands)
{
	int len;
	U32 ead, eas, eat;

	UNUSED_ARGUMENT(stage);

	len = 1;
	eas = MEM_getByte(M68K::cpu_regs.PC + 1);
	M68K::cpu_regs.PC += 2;

	ead = getEffectiveAddress(operands[0], len);
	if (operands[0]<8)
	{
		eas &= 0x1F;
		eas = 1 << eas;

		if (ead & eas)
		{
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
			M68K::cpu_regs.D[operands[0]] &= ~(eas);
		}
		else
		{
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
			M68K::cpu_regs.D[operands[0]] |= eas;
		}

	}
	else
	{
		eat = MEM_getByte(ead);
		eas &= 0x07;
		eas = 1 << eas;

		if (eat & eas)
		{
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
			eat &= ~(eas);
		}
		else
		{
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
			eat |= eas;
		}

		MEM_setByte(ead, eat);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_LSRm(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, ear;

	UNUSED_ARGUMENT(stage);

	len = 2;
	nMask = 0x8000;
	zMask = 0xFFFF;

	eas = getEffectiveAddress(operands[0], len);

	ead = MEM_getWord(eas);

	ear = (ead >> 1)&zMask;

	MEM_setWord(eas, ear);

	if (ead&(1 << (1 - 1)))
	{
		M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
	}

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;
	if (ear & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ear & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_EORI(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[0])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		eas = MEM_getByte(M68K::cpu_regs.PC + 1);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		eas = MEM_getWord(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 2;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		eas = MEM_getLong(M68K::cpu_regs.PC);
		M68K::cpu_regs.PC += 4;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		eas = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if ((operands[1] & 0x38) == 0)	/* destination is D register */
	{
		ead = M68K::cpu_regs.D[operands[1]] & zMask;
		ear = (ead ^ eas)&zMask;
		M68K::cpu_regs.D[operands[1]] &= ~zMask;
		M68K::cpu_regs.D[operands[1]] |= ear;
	}
	else
	{
		ead = getEffectiveAddress(operands[1], len);
		switch (len)
		{
		case 1:
			ear = (MEM_getByte(ead) ^ eas)&zMask;
			MEM_setByte(ead, ear&zMask);
			break;
		case 2:
			ear = (MEM_getWord(ead) ^ eas)&zMask;
			MEM_setWord(ead, ear&zMask);
			break;
		case 4:
			ear = (MEM_getLong(ead) ^ eas)&zMask;
			MEM_setLong(ead, ear&zMask);
			break;
		default:
			ear = 0;
			SOFT_BREAK;	/* should not get here */
			break;
		}
	}

	M68K::cpu_regs.SR &= ~(CPU_STATUS_V | CPU_STATUS_C);

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	ear &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_EORICCR(U32 stage, U16* operands)
{
	U16 eas, ead;

	UNUSED_ARGUMENT(stage);

	eas = MEM_getByte(M68K::cpu_regs.PC + 1);
	M68K::cpu_regs.PC += 2;

	ead = M68K::cpu_regs.SR;

	eas = (ead^eas)&(CPU_STATUS_X | CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_Z);	/* Only affects lower valid bits in flag */

	ead &= ~(CPU_STATUS_X | CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_Z);

	ead |= eas;

	M68K::cpu_regs.SR = ead;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ROXL(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (operands[2] == 0)
	{
		if (operands[0] == 0)
			operands[0] = 8;
	}
	else
	{
		operands[0] = M68K::cpu_regs.D[operands[0]] & 0x3F;
	}

	eas = M68K::cpu_regs.D[operands[3]] & zMask;
	ead = eas;

	if (!operands[0])
	{
		if (M68K::cpu_regs.SR&CPU_STATUS_X)
			M68K::cpu_regs.SR |= CPU_STATUS_C;
		else
			M68K::cpu_regs.SR &= ~CPU_STATUS_C;
	}

	while (operands[0])
	{
		if (ead & nMask)
		{
			ead <<= 1;
			if (M68K::cpu_regs.SR&CPU_STATUS_X)
				ead |= 1;
			ead &= zMask;
			M68K::cpu_regs.SR |= CPU_STATUS_C | CPU_STATUS_X;
		}
		else
		{
			ead <<= 1;
			if (M68K::cpu_regs.SR&CPU_STATUS_X)
				ead |= 1;
			ead &= zMask;
			M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
		}
		operands[0]--;
	}
	M68K::cpu_regs.D[operands[3]] &= ~zMask;
	M68K::cpu_regs.D[operands[3]] |= ead;

	if (M68K::cpu_regs.D[operands[3]] & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (M68K::cpu_regs.D[operands[3]] & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;					/* V flag always cleared */

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ROXR(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if (operands[2] == 0)
	{
		if (operands[0] == 0)
			operands[0] = 8;
	}
	else
	{
		operands[0] = M68K::cpu_regs.D[operands[0]] & 0x3F;
	}

	eas = M68K::cpu_regs.D[operands[3]] & zMask;
	ead = eas;

	if (!operands[0])
	{
		if (M68K::cpu_regs.SR&CPU_STATUS_X)
			M68K::cpu_regs.SR |= CPU_STATUS_C;
		else
			M68K::cpu_regs.SR &= ~CPU_STATUS_C;
	}

	while (operands[0])
	{
		if (ead & 0x01)
		{
			ead >>= 1;
			ead &= ~nMask;
			if (M68K::cpu_regs.SR&CPU_STATUS_X)
				ead |= nMask;
			ead &= zMask;
			M68K::cpu_regs.SR |= CPU_STATUS_C | CPU_STATUS_X;
		}
		else
		{
			ead >>= 1;
			ead &= ~nMask;
			if (M68K::cpu_regs.SR&CPU_STATUS_X)
				ead |= nMask;
			ead &= zMask;
			M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
		}
		operands[0]--;
	}
	M68K::cpu_regs.D[operands[3]] &= ~zMask;
	M68K::cpu_regs.D[operands[3]] |= ead;

	if (M68K::cpu_regs.D[operands[3]] & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (M68K::cpu_regs.D[operands[3]] & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;					/* V flag always cleared */

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MOVETOCCR(U32 stage, U16* operands)
{
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	eas = getSourceEffectiveAddress(operands[0], 2);

	ead = M68K::cpu_regs.SR;

	eas &= (CPU_STATUS_X | CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_Z);	/* Only affects lower valid bits in flag */

	ead &= ~(CPU_STATUS_X | CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_Z);

	ead |= eas;

	M68K::cpu_regs.SR = ead;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_TRAP(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

	CPU_GENERATE_EXCEPTION(0x80 + (operands[0] * 4));

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ADDX(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ead = M68K::cpu_regs.D[operands[0]] & zMask;
	eas = M68K::cpu_regs.D[operands[2]] & zMask;
	ear = (eas + ead)&zMask;
	if (M68K::cpu_regs.SR & CPU_STATUS_X)
		ear = (ear + 1)&zMask;

	M68K::cpu_regs.D[operands[0]] &= ~zMask;
	M68K::cpu_regs.D[operands[0]] |= ear;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;

	ear &= nMask;
	eas &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_DIVS(U32 stage, U16* operands)
{
	int len;
	S32 ead, eas, eaq, ear;

	UNUSED_ARGUMENT(stage);

	len = 2;

	ead = (S32)M68K::cpu_regs.D[operands[0]];
	eas = (S16)(getSourceEffectiveAddress(operands[1], len) & 0xFFFF);

	M68K::cpu_regs.SR &= ~CPU_STATUS_C;			/* carry is always cleared even if divide by zero */

	if (!eas)
	{
		CPU_GENERATE_EXCEPTION(0x14);
		return 0;
	}

	eaq = ead / eas;
	ear = ead % eas;

	if ((eaq<-32768) || (eaq>32767))
	{
		M68K::cpu_regs.SR |= CPU_STATUS_V | CPU_STATUS_N;		/* MC68000 docs claim N and Z are undefined. however real amiga seems to set N if V happens, Z is cleared */
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	}
	else
	{
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

		M68K::cpu_regs.D[operands[0]] = (eaq & 0xFFFF) | ((ear << 16) & 0xFFFF0000);

		if (eaq & 0x8000)
			M68K::cpu_regs.SR |= CPU_STATUS_N;
		else
			M68K::cpu_regs.SR &= ~CPU_STATUS_N;

		if (eaq & 0xFFFF)
			M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
		else
			M68K::cpu_regs.SR |= CPU_STATUS_Z;
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_SUBX(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[1])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	ead = M68K::cpu_regs.D[operands[0]] & zMask;
	eas = M68K::cpu_regs.D[operands[2]] & zMask;
	ear = (ead - eas)&zMask;
	if (M68K::cpu_regs.SR & CPU_STATUS_X)
		ear = (ear - 1)&zMask;

	M68K::cpu_regs.D[operands[0]] &= ~zMask;
	M68K::cpu_regs.D[operands[0]] |= ear;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;

	ear &= nMask;
	eas &= nMask;
	ead &= nMask;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ASRm(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, eat;

	UNUSED_ARGUMENT(stage);

	len = 2;
	nMask = 0x8000;
	zMask = 0xFFFF;

	eat = getEffectiveAddress(operands[0], len);

	eas = MEM_getWord(eat);
	ead = (eas >> 1)&zMask;
	if (eas&nMask)
	{
		ead |= (~(nMask - 1))&zMask;		/* set sign bits */
	}
	MEM_setWord(eat, ead&zMask);

	if (eas & 1)
	{
		M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
	}

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;

	if (ead & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ead & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ASLm(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, eat;

	UNUSED_ARGUMENT(stage);

	len = 2;
	nMask = 0x8000;
	zMask = 0xFFFF;

	eat = getEffectiveAddress(operands[0], len);

	eas = MEM_getWord(eat);
	ead = (eas << 1)&zMask;
	MEM_setWord(eat, ead&zMask);

	if (eas&nMask)
	{
		M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
	}

	if ((eas&nMask) != (ead&nMask))
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

	if (ead & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ead & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ANDICCR(U32 stage, U16* operands)
{
	U16 eas, ead;

	UNUSED_ARGUMENT(stage);

	eas = MEM_getByte(M68K::cpu_regs.PC + 1);
	M68K::cpu_regs.PC += 2;

	ead = M68K::cpu_regs.SR;

	eas = (ead&eas)&(CPU_STATUS_X | CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_Z);	/* Only affects lower valid bits in flag */

	ead &= ~(CPU_STATUS_X | CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_Z);

	ead |= eas;

	M68K::cpu_regs.SR = ead;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ORICCR(U32 stage, U16* operands)
{
	U16 eas, ead;

	UNUSED_ARGUMENT(stage);

	eas = MEM_getByte(M68K::cpu_regs.PC + 1);
	M68K::cpu_regs.PC += 2;

	ead = M68K::cpu_regs.SR;

	eas = (ead | eas)&(CPU_STATUS_X | CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_Z);	/* Only affects lower valid bits in flag */

	ead &= ~(CPU_STATUS_X | CPU_STATUS_N | CPU_STATUS_V | CPU_STATUS_C | CPU_STATUS_Z);

	ead |= eas;

	M68K::cpu_regs.SR = ead;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_NEGX(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 ead, eas, ear;

	UNUSED_ARGUMENT(stage);

	switch (operands[0])
	{
	case 0x00:
		len = 1;
		nMask = 0x80;
		zMask = 0xFF;
		break;
	case 0x01:
		len = 2;
		nMask = 0x8000;
		zMask = 0xFFFF;
		break;
	case 0x02:
		len = 4;
		nMask = 0x80000000;
		zMask = 0xFFFFFFFF;
		break;
	default:
		len = 0;
		nMask = 0;
		zMask = 0;
		SOFT_BREAK;		/* should never get here */
		break;
	}

	if ((operands[1] & 0x38) == 0)	/* destination is D register */
	{
		ead = M68K::cpu_regs.D[operands[1]] & zMask;
		ear = (0 - ead)&zMask;
		if (M68K::cpu_regs.SR & CPU_STATUS_X)
			ear = (ear - 1)&zMask;
		M68K::cpu_regs.D[operands[1]] &= ~zMask;
		M68K::cpu_regs.D[operands[1]] |= ear;
	}
	else
	{
		eas = getEffectiveAddress(operands[1], len);
		switch (len)
		{
		case 1:
			ead = MEM_getByte(eas);
			ear = (0 - ead)&zMask;
			if (M68K::cpu_regs.SR & CPU_STATUS_X)
				ear = (ear - 1)&zMask;
			MEM_setByte(eas, ear);
			break;
		case 2:
			ead = MEM_getWord(eas);
			ear = (0 - ead)&zMask;
			if (M68K::cpu_regs.SR & CPU_STATUS_X)
				ear = (ear - 1)&zMask;
			MEM_setWord(eas, ear);
			break;
		case 4:
			ead = MEM_getLong(eas);
			ear = (0 - ead)&zMask;
			if (M68K::cpu_regs.SR & CPU_STATUS_X)
				ear = (ear - 1)&zMask;
			MEM_setLong(eas, ear);
			break;
		default:
			ead = ear = 0;
			SOFT_BREAK;	/* should not get here */
			break;
		}
	}

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;

	if (ear)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if (ear & ead)
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_V;

	if (ear | ead)
		M68K::cpu_regs.SR |= (CPU_STATUS_C | CPU_STATUS_X);
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_SBCD(U32 stage, U16* operands)
{
	U32 nMask = 0x80, zMask = 0xFF;
	U32 lMask = 0x0F, hMask = 0xF0;
	U32 ead, eas, ear;
	U32 nonbcd;

	UNUSED_ARGUMENT(stage);

	ead = M68K::cpu_regs.D[operands[0]] & zMask;
	eas = M68K::cpu_regs.D[operands[1]] & zMask;
	ear = (ead&lMask) - (eas&lMask);
	nonbcd = ead - eas;
	if (M68K::cpu_regs.SR & CPU_STATUS_X)
	{
		ear -= 1;
		nonbcd -= 1;
	}
	if (ear>9)
	{
		ear -= 6;
	}
	ear += (ead&hMask) - (eas&hMask);
	if (ear>0x99)
	{
		ear += 0xA0;
		M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
	}
	ear &= 0xFF;
	nonbcd &= 0xFF;
	if (ear&nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if ((nonbcd&nMask) == 1 && (ear&nMask) == 0)
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_V);

	M68K::cpu_regs.D[operands[0]] &= ~zMask;
	M68K::cpu_regs.D[operands[0]] |= ear;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ABCD(U32 stage, U16* operands)
{
	U32 nMask = 0x80, zMask = 0xFF;
	U32 lMask = 0x0F, hMask = 0xF0;
	U32 ead, eas, ear;
	U32 nonbcd;

	UNUSED_ARGUMENT(stage);

	ead = M68K::cpu_regs.D[operands[0]] & zMask;
	eas = M68K::cpu_regs.D[operands[1]] & zMask;

	nonbcd = ead + eas;
	ear = (ead&lMask) + (eas&lMask);
	if (M68K::cpu_regs.SR & CPU_STATUS_X)
	{
		ear += 1;
		nonbcd += 1;
	}
	if (ear>9)
	{
		ear += 6;
	}
	ear += (ead&hMask) + (eas&hMask);
	if (ear>0x99)
	{
		ear -= 0xA0;
		M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
	}

	ear &= 0xFF;
	nonbcd &= 0xFF;
	if (ear&nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;

	if ((nonbcd&nMask) == 0 && (ear&nMask) == 1)
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_V);

	M68K::cpu_regs.D[operands[0]] &= ~zMask;
	M68K::cpu_regs.D[operands[0]] |= ear;

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MOVEP_W_m(U32 stage, U16* operands)
{
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	ead = M68K::cpu_regs.A[operands[1]] + (S16)MEM_getWord(M68K::cpu_regs.PC);
	eas = M68K::cpu_regs.D[operands[0]];

	M68K::cpu_regs.PC += 2;

	MEM_setByte(ead, (eas >> 8) & 0xFF);
	MEM_setByte(ead + 2, eas & 0xFF);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MOVEP_L_m(U32 stage, U16* operands)
{
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	ead = M68K::cpu_regs.A[operands[1]] + (S16)MEM_getWord(M68K::cpu_regs.PC);
	eas = M68K::cpu_regs.D[operands[0]];

	M68K::cpu_regs.PC += 2;

	MEM_setByte(ead, (eas >> 24) & 0xFF);
	MEM_setByte(ead + 2, (eas >> 16) & 0xFF);
	MEM_setByte(ead + 4, (eas >> 8) & 0xFF);
	MEM_setByte(ead + 6, eas & 0xFF);

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MOVEP_m_W(U32 stage, U16* operands)
{
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	eas = M68K::cpu_regs.A[operands[1]] + (S16)MEM_getWord(M68K::cpu_regs.PC);

	M68K::cpu_regs.PC += 2;

	ead = MEM_getByte(eas) << 8;
	ead |= MEM_getByte(eas + 2);

	M68K::cpu_regs.D[operands[0]] &= ~0xFFFF;
	M68K::cpu_regs.D[operands[0]] |= ead;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ABCDm(U32 stage, U16* operands)
{
	int len = 1;
	U32 nMask = 0x80, zMask = 0xFF;
	U32 lMask = 0x0F, hMask = 0xF0;
	U32 ead, eat, eas, ear;
	U32 nonbcd;

	UNUSED_ARGUMENT(stage);

	eat = getEffectiveAddress(0x20 + operands[0], len);
	eas = getEffectiveAddress(0x20 + operands[1], len);

	ead = MEM_getByte(eat)&zMask;
	eas = MEM_getByte(eas)&zMask;
	ear = (ead&lMask) + (eas&lMask);
	nonbcd = ead + eas;
	if (M68K::cpu_regs.SR & CPU_STATUS_X)
	{
		ear += 1;
		nonbcd += 1;
	}
	if (ear>9)
	{
		ear += 6;
	}
	ear += (ead&hMask) + (eas&hMask);
	if (ear>0x99)
	{
		ear -= 0xA0;
		M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
	}
	ear &= 0xFF;
	nonbcd &= 0xFF;
	if (ear&nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if ((nonbcd&nMask) == 0 && (ear&nMask) == 1)
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_V);

	MEM_setByte(eat, ear);

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ROXLm(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, eat;

	UNUSED_ARGUMENT(stage);

	len = 2;
	nMask = 0x8000;
	zMask = 0xFFFF;

	eat = getEffectiveAddress(operands[0], len);

	eas = MEM_getWord(eat);

	ead = eas;

	if (ead & nMask)
	{
		ead <<= 1;
		if (M68K::cpu_regs.SR&CPU_STATUS_X)
			ead |= 1;
		ead &= zMask;
		M68K::cpu_regs.SR |= CPU_STATUS_C | CPU_STATUS_X;
	}
	else
	{
		ead <<= 1;
		if (M68K::cpu_regs.SR&CPU_STATUS_X)
			ead |= 1;
		ead &= zMask;
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	}

	MEM_setWord(eat, ead&zMask);

	if (ead & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ead & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;					/* V flag always cleared */

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_MOVEP_m_L(U32 stage, U16* operands)
{
	U32 eas, ead;

	UNUSED_ARGUMENT(stage);

	eas = M68K::cpu_regs.A[operands[1]] + (S16)MEM_getWord(M68K::cpu_regs.PC);

	M68K::cpu_regs.PC += 2;

	ead = MEM_getByte(eas) << 24;
	ead |= MEM_getByte(eas + 2) << 16;
	ead |= MEM_getByte(eas + 4) << 8;
	ead |= MEM_getByte(eas + 6);

	M68K::cpu_regs.D[operands[0]] = ead;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_SBCDm(U32 stage, U16* operands)
{
	int len = 1;
	U32 nMask = 0x80, zMask = 0xFF;
	U32 lMask = 0x0F, hMask = 0xF0;
	U32 ead, eat, eas, ear;
	U32 nonbcd;

	UNUSED_ARGUMENT(stage);

	eat = getEffectiveAddress(0x20 + operands[0], len);
	eas = getEffectiveAddress(0x20 + operands[1], len);

	ead = MEM_getByte(eat)&zMask;
	eas = MEM_getByte(eas)&zMask;

	ear = (ead&lMask) - (eas&lMask);
	nonbcd = ead - eas;
	if (M68K::cpu_regs.SR & CPU_STATUS_X)
	{
		ear -= 1;
		nonbcd -= 1;
	}
	if (ear>9)
	{
		ear -= 6;
	}
	ear += (ead&hMask) - (eas&hMask);
	if (ear>0x99)
	{
		ear += 0xA0;
		M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
	}
	ear &= 0xFF;
	nonbcd &= 0xFF;
	if (ear&nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if ((nonbcd&nMask) == 1 && (ear&nMask) == 0)
		M68K::cpu_regs.SR |= CPU_STATUS_V;
	else
		M68K::cpu_regs.SR &= ~(CPU_STATUS_V);

	MEM_setByte(eat, ear);

	if (ear)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_RTR(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

	M68K::cpu_regs.SR &= 0xFF00;
	M68K::cpu_regs.SR |= MEM_getWord(M68K::cpu_regs.A[7]) & 0xFF;
	M68K::cpu_regs.A[7] += 2;
	M68K::cpu_regs.PC = 0;
	M68K::cpu_regs.PC |= MEM_getWord(M68K::cpu_regs.A[7]) << 16;
	M68K::cpu_regs.A[7] += 2;
	M68K::cpu_regs.PC |= MEM_getWord(M68K::cpu_regs.A[7]);
	M68K::cpu_regs.A[7] += 2;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_EORISR(U32 stage, U16* operands)
{
	U16 oldSR;

	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR = M68K::cpu_regs.SR;
		M68K::cpu_regs.SR ^= MEM_getWord(M68K::cpu_regs.PC);
		CPU_CHECK_SP(oldSR, M68K::cpu_regs.SR);
		M68K::cpu_regs.PC += 2;
	}
	else
	{
		M68K::cpu_regs.PC -= 2;
		CPU_GENERATE_EXCEPTION(0x20);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_LSLm(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, ear;

	UNUSED_ARGUMENT(stage);

	len = 2;
	nMask = 0x8000;
	zMask = 0xFFFF;

	eas = getEffectiveAddress(operands[0], len);

	ead = MEM_getWord(eas);

	ear = (ead << 1)&zMask;

	MEM_setWord(eas, ear);

	if (ead&(nMask >> (1 - 1)))
	{
		M68K::cpu_regs.SR |= CPU_STATUS_X | CPU_STATUS_C;
	}
	else
	{
		M68K::cpu_regs.SR &= ~(CPU_STATUS_X | CPU_STATUS_C);
	}

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;
	if (ear & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ear & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_TRAPV(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_V)
	{
		CPU_GENERATE_EXCEPTION(0x1C);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ROXRm(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, eat;

	UNUSED_ARGUMENT(stage);

	len = 2;
	nMask = 0x8000;
	zMask = 0xFFFF;

	eat = getEffectiveAddress(operands[0], len);

	eas = MEM_getWord(eat);

	ead = eas;

	if (ead & 1)
	{
		ead >>= 1;
		if (M68K::cpu_regs.SR&CPU_STATUS_X)
			ead |= 0x8000;
		ead &= zMask;
		M68K::cpu_regs.SR |= CPU_STATUS_C | CPU_STATUS_X;
	}
	else
	{
		ead >>= 1;
		if (M68K::cpu_regs.SR&CPU_STATUS_X)
			ead |= 0x8000;
		ead &= zMask;
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C | CPU_STATUS_X);
	}

	MEM_setWord(eat, ead&zMask);

	if (ead & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ead & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;					/* V flag always cleared */

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_ROLm(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, eat;

	UNUSED_ARGUMENT(stage);

	len = 2;
	nMask = 0x8000;
	zMask = 0xFFFF;

	eat = getEffectiveAddress(operands[0], len);

	eas = MEM_getWord(eat);

	ead = eas;

	if (ead & nMask)
	{
		ead <<= 1;
		ead |= 1;
		ead &= zMask;
		M68K::cpu_regs.SR |= CPU_STATUS_C;
	}
	else
	{
		ead <<= 1;
		ead &= 0xFFFE;
		ead &= zMask;
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C);
	}

	MEM_setWord(eat, ead&zMask);

	if (ead & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ead & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;					/* V flag always cleared */

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_RORm(U32 stage, U16* operands)
{
	int len;
	U32 nMask, zMask;
	U32 eas, ead, eat;

	UNUSED_ARGUMENT(stage);

	len = 2;
	nMask = 0x8000;
	zMask = 0xFFFF;

	eat = getEffectiveAddress(operands[0], len);

	eas = MEM_getWord(eat);

	ead = eas;

	if (ead & 1)
	{
		ead >>= 1;
		ead |= 0x8000;
		ead &= zMask;
		M68K::cpu_regs.SR |= CPU_STATUS_C;
	}
	else
	{
		ead >>= 1;
		ead &= 0x7FFF;
		ead &= zMask;
		M68K::cpu_regs.SR &= ~(CPU_STATUS_C);
	}

	MEM_setWord(eat, ead&zMask);

	if (ead & nMask)
		M68K::cpu_regs.SR |= CPU_STATUS_N;
	else
		M68K::cpu_regs.SR &= ~CPU_STATUS_N;
	if (ead & zMask)
		M68K::cpu_regs.SR &= ~CPU_STATUS_Z;
	else
		M68K::cpu_regs.SR |= CPU_STATUS_Z;

	M68K::cpu_regs.SR &= ~CPU_STATUS_V;					/* V flag always cleared */

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_RESET(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

	if (M68K::cpu_regs.SR & CPU_STATUS_S)
	{
		/* should hold for 124 cycles */
		M68K::cpu_regs.PC += 2;
	}
	else
	{
		M68K::cpu_regs.PC -= 2;
		CPU_GENERATE_EXCEPTION(0x20);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}


static inline U32 CPU_LINEF(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

	/* Need to check ILLEGAL instructions.. this only works, if PC -2 */

	switch (stage)
	{
	case 0:
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
		return PROCESS_EXCEPTION(1, stage - 1, 0x2C);
	}
#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}

static inline U32 CPU_LINEA(U32 stage, U16* operands)
{
	UNUSED_ARGUMENT(stage);

	/* Need to check ILLEGAL instructions.. this only works, if PC -2 */

	switch (stage)
	{
	case 0:
		return 1;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
		return PROCESS_EXCEPTION(1, stage - 1, 0x28);
	}

#if CPU_COMBINE_STAGES
	return 1;
#else
	return 0;
#endif
}
