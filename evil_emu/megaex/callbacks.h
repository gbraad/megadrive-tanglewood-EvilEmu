#pragma once

void _AudioAddData(int channel, S16 dacValue);
void doPixel(int x, int y, U8 colHi, U8 colLo);
void doPixelNoScale(int x, int y, U8 colHi, U8 colLo);
void doPixel32(int x, int y, U32 colour);
void doPixel32NoScale(int x, int y, U32 colour);
void doPixelClipped(int x, int y, U8 colHi, U8 colLo);
void doPixelClippedNoScale(int x, int y, U8 colHi, U8 colLo);
int CheckKey(int key);
void ClearKey(int key);
U32* pixelPosition(int x, int y);