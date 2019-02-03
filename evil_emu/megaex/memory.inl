static inline unsigned char* MEM_getDMAAddr(U32 address)
{
	U32	upper24 = (address & 0x00FF0000) >> 16;
	U8	memRegion = upper24;

	return Memory::mem_dma_addr[memRegion](address & 0x00FFFFFF);
}

static inline U8 MEM_getByte(U32 address)
{
	U32	upper24 = (address & 0x00FF0000) >> 16;
	U32	lower16 = address & 0x0000FFFF;
	U8	memRegion = upper24;

	return Memory::mem_read[memRegion](address & 0x00FFFFFF, upper24, lower16);
}

static inline U16 MEM_getWord(U32 address)
{
#ifdef DEBUG
	if (address & 1)
	{
		printf("Mem Get Word (Unaligned Read)\n");
	}
#endif

	U32	memRegion = (address & 0x00FF0000) >> 16;
	return Memory::mem_read_word[memRegion](address & 0x00FFFFFF);
}

static inline U32 MEM_getLong(U32 address)
{
#ifdef DEBUG
	if (address & 1)
	{
		printf("Mem Get Long (Unaligned Read)\n");
	}
#endif

	return (MEM_getWord(address) << 16) | MEM_getWord(address + 2);
}

static inline void MEM_setByte(U32 address, U8 byte)
{
	U32	upper24 = (address & 0x00FF0000) >> 16;
	U32	lower16 = address & 0x0000FFFF;
	U8	memRegion = upper24;

	Memory::mem_write[memRegion](address, upper24, lower16, byte);
}

static inline void MEM_setWord(U32 address, U16 word)
{
#ifdef DEBUG
	if (address & 1)
	{
		printf("Mem Set Word (Unaligned Write)\n");
	}
#endif

	MEM_setByte(address, word >> 8);
	MEM_setByte(address + 1, word & 0xFF);
}

static inline void MEM_setLong(U32 address, U32 dword)
{
#ifdef DEBUG
	if (address & 1)
	{
		printf("Mem Set Long (Unaligned Write)\n");
	}
#endif

	MEM_setWord(address, dword >> 16);
	MEM_setWord(address + 2, dword & 0xFFFF);
}
