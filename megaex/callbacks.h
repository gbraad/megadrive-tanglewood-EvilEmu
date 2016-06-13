#pragma once

ION_C_API void _AudioAddData(int channel, S16 dacValue);
ION_C_API void doPixel(int x, int y, U8 colHi, U8 colLo);
ION_C_API void doPixel32(int x, int y, U32 colour);
ION_C_API void doPixelClipped(int x, int y, U8 colHi, U8 colLo);
ION_C_API int CheckKey(int key);
ION_C_API void ClearKey(int key);
ION_C_API U32* pixelPosition(int x, int y);