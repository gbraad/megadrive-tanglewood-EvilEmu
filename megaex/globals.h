#pragma once

ION_C_API  U32 lineNo;
ION_C_API  U32 colNo;

ION_C_API unsigned char *SRAM;
ION_C_API U32 SRAM_Size;
ION_C_API U32 SRAM_StartAddress;
ION_C_API U32 SRAM_EndAddress;
ION_C_API U32 SRAM_OddAccess;
ION_C_API U32 SRAM_EvenAccess;

ION_C_API U8 inHBlank;
ION_C_API U8 inVBlank;

ION_C_API int g_newScreenNotify;

ION_C_API U16 keyStatusJoyA;

ION_C_API int g_pause;