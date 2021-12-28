#include "megaex/vdp/vdp.h"
#include "megaex/memory.h"
#include "megaex/callbacks.h"

#include <string.h>

int VDP::cramCacheDirty;
U8 VDP::VDP_Registers[0x20];
U8* VDP::videoMemory = nullptr;
U8* VDP::videoBuffers[VDP_NUM_BUFFERS];
int VDP::writeBufferIdx = 1;
int VDP::readBufferIdx = 0;
bool VDP::drawPlaneB = true;
bool VDP::transparentBG = false;
ion::thread::Event VDP::readBufferReady;

struct Sprite
{
	U16 ypos;
	U8  size;
	U8  next;
	U8  flags;
	U8  tileid;
	U16 xpos;
};

void VDP_Init()
{
#if VDP_SCALE_2X
	int bufferSize = DRAW_BUFFER_WIDTH*DRAW_BUFFER_HEIGHT * sizeof(U32) * 4;
#else
	int bufferSize = DRAW_BUFFER_WIDTH * DRAW_BUFFER_HEIGHT * sizeof(U32);
#endif

	for (int i = 0; i < VDP_NUM_BUFFERS; i++)
	{
		VDP::videoBuffers[i] = new U8[bufferSize];
		memset(VDP::videoBuffers[i], 0, bufferSize);
	}

	VDP::videoMemory = VDP::videoBuffers[0];
	VDP::readBufferIdx = 0;
	VDP::writeBufferIdx = 0;
}

void VDP_Shutdown()
{
	for (int i = 0; i < VDP_NUM_BUFFERS; i++)
	{
		delete[] VDP::videoBuffers[i];
	}
}

void VDP_WriteLock()
{

}

void VDP_WriteUnlock()
{
	VDP::readBufferIdx = VDP::writeBufferIdx;
	VDP::writeBufferIdx = (VDP::writeBufferIdx + 1) % VDP_NUM_BUFFERS;
	VDP::videoMemory = VDP::videoBuffers[VDP::writeBufferIdx];
	VDP::readBufferReady.Signal();
}

U8* VDP_ReadLock()
{
	VDP::readBufferReady.Wait();
	return VDP::videoBuffers[VDP::readBufferIdx];
}

void VDP_ReadUnlock()
{

}

void VDP_GetRegisterContents(U16 offset, char *buffer)
{
#if SMS_MODE
	offset &= 0x0F;
	sprintf(buffer, "%s : %02X", SMS_VDP_DumpRegisterName(offset), VDP::VDP_Registers[offset]);
#else
	offset &= 0x1F;
	sprintf(buffer, "%s : %02X", VDP_DumpRegisterName(offset), VDP::VDP_Registers[offset]);
#endif
}

const char *VDP_DumpRegisterName(U16 byte)
{
	switch (byte)
	{
	case 0x00:
		return "Mode Set Register 01  ";
	case 0x01:
		return "Mode Set Register 02  ";
	case 0x02:
		return "Table Address Scroll A";
	case 0x03:
		return "Table Address Window  ";
	case 0x04:
		return "Table Address Scroll B";
	case 0x05:
		return "Table Address Sprite  ";
	case 0x06:
		return "Unknown (0x06)        ";
	case 0x07:
		return "Backdrop Colour       ";
	case 0x08:
		return "Unknown (0x08)        ";
	case 0x09:
		return "Unknown (0x09)        ";
	case 0x0A:
		return "H Interrupt Register  ";
	case 0x0B:
		return "Mode Set Register 03  ";
	case 0x0C:
		return "Mode Set Register 04  ";
	case 0x0D:
		return "H Scroll Table Address";
	case 0x0E:
		return "Unknown (0x0E)        ";
	case 0x0F:
		return "Auto Increment Data   ";
	case 0x10:
		return "Scroll Size           ";
	case 0x11:
		return "Window H Position     ";
	case 0x12:
		return "Window V Position     ";
	case 0x13:
		return "DMA Length Counter Lo ";
	case 0x14:
		return "DMA Length Counter Hi ";
	case 0x15:
		return "DMA Src Address Low   ";
	case 0x16:
		return "DMA Src Address Mid   ";
	case 0x17:
		return "DMA Src Address Hgh   ";
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		return "Unknown               ";
	}

	return "";
}

const char *SMS_VDP_DumpRegisterName(U16 byte)
{
	switch (byte)
	{
	case 0x00:
		return "Mode Control 01       ";
	case 0x01:
		return "Mode Control 02       ";
	case 0x02:
		return "Name Table Address    ";
	case 0x03:
		return "Color Table Address   ";
	case 0x04:
		return "BKG Pattern Address   ";
	case 0x05:
		return "Sprite Attr Table     ";
	case 0x06:
		return "Sprite Pattern Table  ";
	case 0x07:
		return "Backdrop Colour       ";
	case 0x08:
		return "Scroll X              ";
	case 0x09:
		return "Scroll Y              ";
	case 0x0A:
		return "Line Counter          ";
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
		return "Unknown               ";
	}

	return "";
}

enum DrawPlane
{
	PLANE_A,
	PLANE_B,
	PLANE_S,
	PLANE_W
};

inline unsigned int GetVRAMBankTiles(int plane)
{
#if VRAM_128KB_MODE
	// Reg 1 bit 7 = 128KB VRAM enable
	if ((VDP::VDP_Registers[1] & (1 << 7)) == 0)
	{
		// 64KB VRAM mode
		return 0;
	}

	switch (plane)
	{
	case PLANE_A:
		// Reg E bit 0 = plane A tile data in upper 64kb VRAM
		return (VDP::VDP_Registers[14] & 1) ? 0x10000 : 0;
	case PLANE_B:
		// Reg E bits 0 and 4 = plane B tile data in upper 64kb VRAM
		return ((VDP::VDP_Registers[14] & 1) && (VDP::VDP_Registers[14] & (1 << 4))) ? 0x10000 : 0;
	case PLANE_S:
		// Reg 6 bit 5 = sprite tile data in upper 64kb VRAM
		return (VDP::VDP_Registers[6] & (1 << 5)) ? 0x10000 : 0;
	default:
		return 0;
	}
#else
	return 0;
#endif
}

void DisplayFillBGColourHiLoLine(U8 colHi, U8 colLo, U32 *pixelPos, int w)
{
	int a;
	U32 colour;
	U8 r = (colHi & 0x0F) << 4;
	U8 g = (colLo & 0xF0);
	U8 b = (colLo & 0x0F) << 4;

	colour = (0xFF << 24) | (r << 16) | (g << 8) | (b << 0);

	for (a = 0; a<w; a++)
	{
		*pixelPos++ = colour;
#if VDP_SCALE_2X
		*pixelPos++ = colour;
#endif
	}
}

U32 VDP_GetBackgroundColourRGBA()
{
	U8 palIdx = (VDP::VDP_Registers[7] & 0x30) >> 4;
	U8 colourIdx = VDP::VDP_Registers[7] & 0x0F;

	U8 colLo = (Memory::cRam[palIdx * 2 * 16 + colourIdx * 2 + 0] & 0x0E) | (Memory::cRam[palIdx * 2 * 16 + colourIdx * 2 + 1] & 0xE0);
	U8 colHi = Memory::cRam[palIdx * 2 * 16 + colourIdx * 2 + 1] & 0x0E;

	U8 r = (colHi & 0x0F) << 4;
	U8 g = (colLo & 0xF0);
	U8 b = (colLo & 0x0F) << 4;

	U32 colour = (r << 24) | (g << 16) | (b << 8);

	return colour;
}

void DisplayFillTransparent(U32* pixelPos, int w)
{
	memset(pixelPos, 0, w * sizeof(U32));

#if VDP_SCALE_2X
	memset(pixelPos + w, 0, w * sizeof(U32));
#endif
}

void DisplayFillBGColourLine(U32 *pixelPos, int w)
{
	U8 pal = (VDP::VDP_Registers[7] & 0x30) >> 4;
	U8 colour = VDP::VDP_Registers[7] & 0x0F;

	U8 r = colour;
	U8 gb = colour | (colour << 4);

	gb = Memory::cRam[pal * 2 * 16 + colour * 2 + 0] & 0x0E;
	r = Memory::cRam[pal * 2 * 16 + colour * 2 + 1] & 0x0E;
	gb |= Memory::cRam[pal * 2 * 16 + colour * 2 + 1] & 0xE0;

	DisplayFillBGColourHiLoLine(r, gb, pixelPos, w);
}

void SMS_DisplayFillBGColourLine(U32 *pixelPos, int w)
{
	U8 pal = 1;
	U8 colour = VDP::VDP_Registers[7] & 0x0F;

	U8 r = 0;
	U8 gb = 0;

	r = (Memory::cRam[pal * 16 + colour] & 0x03) << 2;
	gb |= (Memory::cRam[pal * 16 + colour] & 0x0C) << 4;
	gb |= (Memory::cRam[pal * 16 + colour] & 0x30) >> 2;

	DisplayFillBGColourHiLoLine(r, gb, pixelPos, w);
}

static inline int ComputeTilePixelColourXY(int tx, int ty, U32 address, U32 flipH, U32 flipV)
{
	int colour;
	int odd;

	//If flipped Y
	if (flipV)
	{
		//Invert Y, + offset in longwords
		address += (7 - ty) << 2;
	}
	else
	{
		//+ offset in longwords
		address += ty << 2;
	}

	//If flipped X
	if (flipH)
	{
		//Invert X, + offset in nibbles
		address += (7 - tx) >> 1;
		odd = (7 - tx) & 1;
	}
	else
	{
		//+ offset in nibbles
		address += tx >> 1;
		odd = tx & 1;
	}
	address &= 0xFFFF;

	if (odd == 0)
	{
		//Even nibble
		colour = (Memory::vRam[address] & 0xF0) >> 4;
	}
	else
	{
		//Odd nibble
		colour = Memory::vRam[address] & 0x0F;
	}

	return colour;
}

U8	zBuffer[VDP_SCREEN_WIDTH_TILES * 8];

void doPixelSprite(int x, int y, U8 colHi, U8 colLo, U8 zValue)
{
	if (zBuffer[x]<zValue)
	{
		zBuffer[x] = zValue;
		doPixel(x, y - 128, colHi, colLo);
	}
}

U32 paletteCache[16 * 4];

void UpdatePaletteCache()
{
	for (int pal = 0; pal < 4; pal++)
	{
		for (int colour = 0; colour < 16; colour++)
		{
			// 0BBB 0RRR 0000 0GGG
			U16 cramWord = *(U16*)&Memory::cRam[pal * 2 * 16 + colour * 2];

			// 0x0000BR0G
			// to
			// 0xAARRBBGG;
			paletteCache[(pal * 16) + colour] = (0xFF << 24) | ((cramWord & 0x0E00) << 12) | (cramWord & 0xE000) | ((cramWord & 0x000E) << 4);
		}
	}
}

U32 VDP_GetPlaneWidthPx()
{
	return ((VDP::VDP_Registers[12] & 0x01) ? VDP_SCREEN_WIDTH_TILES : 32) * 8;
}

S32 VDP_GetScrollX_PlaneB(int y)
{
	U32 scrollHTable = (VDP::VDP_Registers[13] & 0x3F) << 10;

	S32 b_scrollAmountX;

	if ((VDP::VDP_Registers[11] & 0x03) == 0x03)
	{
		b_scrollAmountX = ((Memory::vRam[scrollHTable + 2 + y * 4] & 0x07) << 8) | Memory::vRam[scrollHTable + 3 + y * 4];
	}
	else
	{
		if ((VDP::VDP_Registers[11] & 0x03) == 0x02)
		{
			b_scrollAmountX = ((Memory::vRam[scrollHTable + 2 + (y / 8) * 32] & 0x07) << 8) | Memory::vRam[scrollHTable + 3 + (y / 8) * 32];
		}
		else
		{
			b_scrollAmountX = ((Memory::vRam[scrollHTable + 2] & 0x07) << 8) | Memory::vRam[scrollHTable + 3];
		}
	}

	return b_scrollAmountX;
}

S32 VDP_GetScrollY_PlaneB()
{
	return ((Memory::vsRam[2] & 0x0F) << 8) | (Memory::vsRam[3]);
}

void DrawScreenRow(int ty, U32* pixelPos, U8* zPos)
{
#if VDP_SCALE_2X
	U32* pixelPos2 = pixelPos + 1;
#endif

	//Update CRAM cache first
	if (VDP::cramCacheDirty)
	{
		UpdatePaletteCache();
		VDP::cramCacheDirty = 0;
	}

	//Get plane width (H40 == H56 if VDP_H54_MODE defined)
	int planeWidthX = (VDP::VDP_Registers[12] & 0x01) ? VDP_SCREEN_WIDTH_TILES : 32;

	//If display enabled
	if (VDP::VDP_Registers[1] & 0x40)
	{
		//Get h-scroll table address
		U32 scrollHTable = (VDP::VDP_Registers[13] & 0x3F) << 10;

		//Get plane A scroll mode
		S32 a_scrollAmountX;

		if ((VDP::VDP_Registers[11] & 0x03) == 0x03)
		{
			//Pixel scroll
			a_scrollAmountX = ((Memory::vRam[scrollHTable + 0 + ty * 4] & 0x07) << 8) | Memory::vRam[scrollHTable + 1 + ty * 4];
		}
		else
		{
			if ((VDP::VDP_Registers[11] & 0x03) == 0x02)
			{
				//Cell scroll
				a_scrollAmountX = ((Memory::vRam[scrollHTable + 0 + (ty / 8) * 32] & 0x07) << 8) | Memory::vRam[scrollHTable + 1 + (ty / 8) * 32];
			}
			else
			{
				//Plane scroll
				a_scrollAmountX = ((Memory::vRam[scrollHTable + 0] & 0x07) << 8) | Memory::vRam[scrollHTable + 1];
			}
		}

		//Get plane B scroll mode
		S32 b_scrollAmountX;

		if ((VDP::VDP_Registers[11] & 0x03) == 0x03)			/* do pixel scroll */
		{
			b_scrollAmountX = ((Memory::vRam[scrollHTable + 2 + ty * 4] & 0x07) << 8) | Memory::vRam[scrollHTable + 3 + ty * 4];
		}
		else
		{
			if ((VDP::VDP_Registers[11] & 0x03) == 0x02)			/* do cell scroll */
			{
				b_scrollAmountX = ((Memory::vRam[scrollHTable + 2 + (ty / 8) * 32] & 0x07) << 8) | Memory::vRam[scrollHTable + 3 + (ty / 8) * 32];
			}
			else
			{
				b_scrollAmountX = ((Memory::vRam[scrollHTable + 2] & 0x07) << 8) | Memory::vRam[scrollHTable + 3];
			}
		}

		//Get plane A/B v-scroll values
		S32 a_scrollAmountY = ((Memory::vsRam[0] & 0x0F) << 8) | (Memory::vsRam[1]);
		S32 b_scrollAmountY = ((Memory::vsRam[2] & 0x0F) << 8) | (Memory::vsRam[3]);

		//Get plane A/B map data addresses
		U32 a_map_data_addr = ((VDP::VDP_Registers[0x02]) & 0x38) << 10;
		U32 b_map_data_addr = ((VDP::VDP_Registers[0x04]) & 0x07) << 13;

#if EMU_SUPPORT_WINDOW_PLANE
		// Window plane
		if (VDP::VDP_Registers[0x12] & 0x9F)
		{
			U8 windowLine = (VDP::VDP_Registers[0x12] & 0x1F) << 3;
			U8 useWindow = 0;

			if (VDP::VDP_Registers[0x12] & 0x80)
			{
				if (ty >= windowLine)
				{
					useWindow = 1;
				}
			}
			else
			{
				if (ty < windowLine)
				{
					useWindow = 1;
				}
			}

			if (useWindow)
			{
				a_scrollAmountY = 0;
				a_scrollAmountX = 0;
				a_map_data_addr = ((VDP::VDP_Registers[0x03]) & 0x3E) << 10;		/* todo 40 cell clamp to 3C */
			}
		}
#endif

		//Get plane size
		int planeSizeX;
		int planeSizeY;

		switch (VDP::VDP_Registers[16] & 0x30)
		{
		default:
		case 0x20:			/* Marked Prohibited */
		case 0x00:
			planeSizeY = 32;
			break;
		case 0x10:
			planeSizeY = 64;
			break;
		case 0x30:
			planeSizeY = 128;
			break;
		}
		switch (VDP::VDP_Registers[16] & 0x03)
		{
		default:
		case 2:			/* Marked Prohibited */
		case 0:
			planeSizeX = 32;
			break;
		case 1:
			planeSizeX = 64;
			break;
		case 3:
			planeSizeX = 128;
			break;
		}

		if (a_scrollAmountY & 0x0800)
			a_scrollAmountY |= 0xFFFFF000;
		if (a_scrollAmountX & 0x0400)
			a_scrollAmountX |= 0xFFFFF800;
		if (b_scrollAmountY & 0x0800)
			b_scrollAmountY |= 0xFFFFF000;
		if (b_scrollAmountX & 0x0400)
			b_scrollAmountX |= 0xFFFFF800;

		//Get map tile Y coord and wrap around plane height
		int a_ty = (ty + a_scrollAmountY) & (planeSizeY * 8 - 1);
		int b_ty = (ty + b_scrollAmountY) & (planeSizeY * 8 - 1);

		int widthPixels = planeWidthX * 8;

		for (int pixelX = 0; pixelX < widthPixels; pixelX++)			/* base address is vertical adjusted..  */
		{
			//Get map tile X coord and wrap around plane width
			int a_tx = (pixelX - a_scrollAmountX) & (planeSizeX * 8 - 1);
			int b_tx = (pixelX - b_scrollAmountX) & (planeSizeX * 8 - 1);

			//Get VRAM address of tile ID
#if VRAM_128KB_MODE
			U16 a_tile_addr = (a_map_data_addr + (a_tx / 8) * 2 + (a_ty / 8)*planeSizeX * 2) & 0x1FFFF;
			U16 b_tile_addr = (b_map_data_addr + (b_tx / 8) * 2 + (b_ty / 8)*planeSizeX * 2) & 0x1FFFF;
#else
			U16 a_tile_addr = (a_map_data_addr + (a_tx / 8) * 2 + (a_ty / 8)*planeSizeX * 2) & 0xFFFF;
			U16 b_tile_addr = (b_map_data_addr + (b_tx / 8) * 2 + (b_ty / 8)*planeSizeX * 2) & 0xFFFF;
#endif

			//Get tile ID and flags
			U16 a_tile = (Memory::vRam[a_tile_addr] << 8) | Memory::vRam[a_tile_addr + 1];
			U16 b_tile = (Memory::vRam[b_tile_addr] << 8) | Memory::vRam[b_tile_addr + 1];

			//Get priority
			int a_prio = (a_tile & 0x8000) ? 2 << 4 : 2;
			int b_prio = (b_tile & 0x8000) ? 1 << 4 : 1;

			// Draw pixel plane A
			if (*zPos < a_prio)
			{
				//Get tile VRAM address
				U16 a_tileAddress = (a_tile & 0x07FF) << 5;	// pccv hnnn nnnn nnnn << 5 = tileId to bytes

															//Get palette
				int a_pal = (a_tile & 0x6000) >> 13;

				//Compute pixel colour
				int colour = ComputeTilePixelColourXY((a_tx & 7), (a_ty & 7), a_tileAddress, a_tile & 0x0800, a_tile & 0x1000);

				if (colour != 0)
				{
					*zPos = a_prio;
					*pixelPos = paletteCache[(a_pal * 16) + colour];
#if VDP_SCALE_2X
					*pixelPos2 = paletteCache[(a_pal * 16) + colour];
#endif
				}
			}

			// Draw pixel plane B
			if (VDP::drawPlaneB && (*zPos < b_prio))
			{
				//Get tile VRAM address
				U16 b_tileAddress = (b_tile & 0x07FF) << 5;	// pccv hnnn nnnn nnnn << 5 = tileId to bytes

															//Get palette
				int b_pal = (b_tile & 0x6000) >> 13;

				//Compute pixel colour
				int colour = ComputeTilePixelColourXY((b_tx & 7), (b_ty & 7), b_tileAddress, b_tile & 0x0800, b_tile & 0x1000);

				if (colour != 0)
				{
					*zPos = b_prio;
					*pixelPos = paletteCache[(b_pal * 16) + colour];
#if VDP_SCALE_2X
					*pixelPos2 = paletteCache[(b_pal * 16) + colour];
#endif
				}
			}

#if VDP_SCALE_2X
			pixelPos += 2;
			pixelPos2 += 2;
#else
			pixelPos++;
#endif

			zPos++;
		}
	}
	else
	{
		//Display disabled, draw BG colour
		int widthPixels = planeWidthX * 8;

		U16 bgColourPal = (VDP::VDP_Registers[7] & 0xF0) >> 4;
		U16 bgColourIdx = VDP::VDP_Registers[7] & 0x0F;
		U32 bgColour = paletteCache[(bgColourPal * 16) + bgColourIdx];

		for (int pixelX = 0; pixelX < widthPixels; pixelX++)
		{
			*pixelPos++ = bgColour;
		}
	}
}

/* we need at least : pcc0XXXX			(p priority - cc colour palette - XXXX colour from tile) */
const int spriteTempBufferSize = VDP_SCREEN_WIDTH_TILES * 8;
U8	spriteTempBuffer[spriteTempBufferSize];		/* 1 Full width line		(VDP must compute this or similar buffer a line ahead?) */

void ProcessSpritePixel(int x, int y, U8 attributes, int ScreenY)
{
	if (y == ScreenY)
	{
		if (x >= VDP_SPRITE_BORDER_X && x < VDP_SPRITE_BORDER_Y + VDP_SCREEN_WIDTH_TILES * 8)
		{
			if (spriteTempBuffer[x - VDP_SPRITE_BORDER_X] == 0)
			{
				spriteTempBuffer[x - VDP_SPRITE_BORDER_X] = attributes;
			}
		}
	}
}

/* Slow - but easy to test quickly */
void ProcessSpriteTile(int xx, int yy, U32 address, int pal, U32 flipH, U32 flipV, int ScreenY, U8 highNibbleOfSpriteInfo)
{
	int x, y;
	int colour;

	UNUSED_ARGUMENT(pal);

#if VRAM_128KB_MODE
	address &= 0x1FFFF;
#else
	address &= 0xFFFF;
#endif

	for (y = 0; y<8; y++)
	{
		for (x = 0; x<8; x++)
		{
			if ((x & 1) == 0)
			{
				colour = (Memory::vRam[address + (y * 4) + (x / 2)] & 0xF0) >> 4;
			}
			else
			{
				colour = Memory::vRam[address + (y * 4) + (x / 2)] & 0x0F;
			}

			if (colour != 0)
			{
				colour |= highNibbleOfSpriteInfo;
				if (flipH && flipV)
				{
					ProcessSpritePixel((7 - x) + xx, (7 - y) + yy, colour, ScreenY);
				}
				if (!flipH && flipV)
				{
					ProcessSpritePixel(x + xx, (7 - y) + yy, colour, ScreenY);
				}
				if (flipH && !flipV)
				{
					ProcessSpritePixel((7 - x) + xx, y + yy, colour, ScreenY);
				}
				if (!flipH && !flipV)
				{
					ProcessSpritePixel(x + xx, y + yy, colour, ScreenY);
				}
			}
		}
	}
}

void ProcessSpriteTileLine(int drawX, U32 tileAddress, U32 tileLine, U32 flipH, U8 highNibbleOfSpriteInfo)
{
	int colour;

#if VRAM_128KB_MODE
	tileAddress &= 0x1FFFF;
#else
	tileAddress &= 0xFFFF;
#endif

	for (int x = 0; x<8; x++)
	{
		if ((x & 1) == 0)
		{
			colour = (Memory::vRam[tileAddress + (tileLine * 4) + (x / 2)] & 0xF0) >> 4;
		}
		else
		{
			colour = Memory::vRam[tileAddress + (tileLine * 4) + (x / 2)] & 0x0F;
		}

		if (colour != 0)
		{
			U32 pixelX = 0;
			colour |= highNibbleOfSpriteInfo;

			if (flipH)
			{
				pixelX = (7 - x) + drawX;
			}
			else
			{
				pixelX = x + drawX;
			}

			if (pixelX >= VDP_SPRITE_BORDER_X && pixelX < VDP_SPRITE_BORDER_X + VDP_SCREEN_WIDTH_TILES * 8)
			{
				if (spriteTempBuffer[pixelX - VDP_SPRITE_BORDER_X] == 0)
				{
					spriteTempBuffer[pixelX - VDP_SPRITE_BORDER_X] = colour;
				}
			}
		}
	}
}

void ComputeSpritesForNextLine(int nextLine)
{
	//If display enabled
	if (VDP::VDP_Registers[1] & 0x40)
	{
		int xTile, yTile;
		U16 baseSpriteAddress = (VDP::VDP_Registers[5] & 0x7F) << 9;
		U32 baseTilesAddress = GetVRAMBankTiles(PLANE_S);
		int displaySizeX = (VDP::VDP_Registers[12] & 0x01) ? VDP_SCREEN_WIDTH_TILES : 32;
		U16 spriteAddress;
		U16 totSpritesScreen = (VDP::VDP_Registers[12] & 0x01) ? 80 : 64;
		U16 totSpritesScan = (VDP::VDP_Registers[12] & 0x01) ? 20 : 16;
		U16 curLink = 0;

		int	bailOut = 80;			/* bug in sprite stuff - infinite loop this will clear after 80  */

		memset(spriteTempBuffer, 0, displaySizeX * 8);			/* clear processing buffer down */

																/* Sprite Data


																Index + 0  :   -------y yyyyyyyy	: 9 bits (10 bits in interlaced mode only)
																Index + 2  :   ----hhvv			; 2x2 bits
																Index + 3  :   -lllllll			; 7 bits
																Index + 4  :   pccvhnnn nnnnnnnn	; 1 bit (prio), 2 bits (pal), 1 bit (vflip), 1 bit (hflip), 11 bits (tile id)
																Index + 6  :   -------x xxxxxxxx	; 9 bits

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


																NB : sprite with x coord=0 masks off lower sprites for scanlines this sprite would occupy

																*/

		spriteAddress = baseSpriteAddress;
		while (totSpritesScan && bailOut)
		{
#if VDP_H50_MODE || VDP_H54_MODE|| VDP_H106_MODE
			U16 xPosPixels = ((Memory::vRam[spriteAddress + 6] << 8) | (Memory::vRam[spriteAddress + 7])) & 0x03FF;
#else
			U16 xPosPixels = ((Memory::vRam[spriteAddress + 6] << 8) | (Memory::vRam[spriteAddress + 7])) & 0x01FF;
#endif
			U16 yPosPixels = ((Memory::vRam[spriteAddress + 0] << 8) | (Memory::vRam[spriteAddress + 1])) & 0x01FF;
			U16 size = Memory::vRam[spriteAddress + 2] & 0x0F;
			U16 link = Memory::vRam[spriteAddress + 3] & 0x7F;
			U16 ctrl = ((Memory::vRam[spriteAddress + 4] << 8) | (Memory::vRam[spriteAddress + 5])) & 0xFFFF;
			int vSizeTiles = (size & 0x03) + 1;
			int hSizeTiles = ((size & 0x0C) >> 2) + 1;
			U8 attributeHighNibble = (ctrl & 0xE000) >> 8;
			U32 firstTileAddress = ((ctrl & 0x07FF) << 5) + baseTilesAddress;

			bailOut--;

			if ((nextLine >= yPosPixels) && (nextLine <= (yPosPixels + ((8 * vSizeTiles) - 1))))			/* not interested if not current line */
			{
				if (xPosPixels == 0 && curLink != 0)			/* Special masking mode.. check if curLine is within sprite Y range, if it is */
				{						/*stop processing list - sprite 0 does not count */
					break;
				}
				/* Need to draw correct number of sprites for one line etc. */

				totSpritesScan--;

				bool flipX = (ctrl & 0x0800) != 0;
				bool flipY = (ctrl & 0x1000) != 0;

				const int bytesPerTile = 32;
				const int tileWidth = 8;
				const int tileHeight = 8;

				for (xTile = 0; xTile < hSizeTiles; xTile++)
				{
					U32 tileStartAddress = 0;
					U32 lineDrawPosX = 0;
					U32 lineDrawPosY = 0;
					U32 tileLine = 0;

					if (!flipX && !flipY)	/* no flip */
					{
						tileLine = nextLine - yPosPixels;
						yTile = (tileLine / 8);
						lineDrawPosX = xPosPixels + xTile * tileWidth;
						lineDrawPosY = yPosPixels + tileLine;
						tileStartAddress = firstTileAddress + (yTile * bytesPerTile) + (xTile * vSizeTiles * bytesPerTile);
					}
					else if (flipX && !flipY)	/* H flip */
					{
						tileLine = nextLine - yPosPixels;
						yTile = (tileLine / 8);
						lineDrawPosX = xPosPixels + (hSizeTiles - xTile - 1) * tileWidth;
						lineDrawPosY = yPosPixels + tileLine;
						tileStartAddress = firstTileAddress + (yTile * bytesPerTile) + (xTile * vSizeTiles * bytesPerTile);
					}
					else
					{
						#if defined _DEBUG
						static bool warned = false;
						if (!warned)
						{
							EMU_PRINTF("megaEx: Sprite Y flip not implemented!\n");
							warned = true;
						}
						#endif
					}

					ProcessSpriteTileLine(lineDrawPosX, tileStartAddress, (tileLine % tileHeight), flipX, attributeHighNibble);
				}

#if 0
				for (x = 0; x <= hSize; x++)
				{
					for (y = 0; y <= vSize; y++)
					{
						if ((ctrl & 0x0800) && (ctrl & 0x1000))		/* H & V flip */
						{
							ProcessSpriteTile(xPos + ((hSize)-x) * 8, yPos + ((vSize)-y) * 8, ((ctrl & 0x07FF) << 5) + y * 32 + x * (vSize + 1) * 32, (ctrl & 0x6000) >> 13, ctrl & 0x0800, ctrl & 0x1000, nextLine, attributeHighNibble);
						}
						if (((ctrl & 0x0800) == 0) && (ctrl & 0x1000))	/* V flip */
						{
							ProcessSpriteTile(xPos + x * 8, yPos + ((vSize)-y) * 8, ((ctrl & 0x07FF) << 5) + y * 32 + x * (vSize + 1) * 32, (ctrl & 0x6000) >> 13, ctrl & 0x0800, ctrl & 0x1000, nextLine, attributeHighNibble);
						}
						if ((ctrl & 0x0800) && ((ctrl & 0x1000) == 0))	/* H flip */
						{
							ProcessSpriteTile(xPos + ((hSize)-x) * 8, yPos + y * 8, ((ctrl & 0x07FF) << 5) + y * 32 + x * (vSize + 1) * 32, (ctrl & 0x6000) >> 13, ctrl & 0x0800, ctrl & 0x1000, nextLine, attributeHighNibble);
						}
						if (((ctrl & 0x0800) == 0) && ((ctrl & 0x1000) == 0))	/* no flip */
						{
							ProcessSpriteTile(xPos + x * 8, yPos + y * 8, ((ctrl & 0x07FF) << 5) + y * 32 + x * (vSize + 1) * 32, (ctrl & 0x6000) >> 13, ctrl & 0x0800, ctrl & 0x1000, nextLine, attributeHighNibble);
						}
					}
					totPixelsScan -= 8;
					if (!totPixelsScan)
					{
						break;
					}
					if (x == 2)
					{
						if (!totSpritesScan)
						{
							break;
						}
						totSpritesScan--;
					}
				}
#endif
			}

			if (link == 0)
				break;

			curLink = link;
			if (curLink >= totSpritesScreen)
				break;

			spriteAddress = baseSpriteAddress + link * 8;

#if VRAM_128KB_MODE
			spriteAddress &= 0x1FFFF;
#else
			spriteAddress &= 0xFFFF;
#endif
		}
	}
	else
	{
		//Display disabled
		memset(spriteTempBuffer, 0, spriteTempBufferSize);
	}
}

void DrawSpritesForLine(int curLine, U8 zValue)
{
	int x;
	int displaySizeX = (VDP::VDP_Registers[12] & 0x01) ? VDP_SCREEN_WIDTH_TILES : 32;

	for (x = 0; x<displaySizeX * 8; x++)
	{
		U8 spriteInfo = spriteTempBuffer[x];

		if (spriteInfo != 0)
		{
			int pal = (spriteInfo & 0x60) >> 5;
			int colour = spriteInfo & 0x0F;
			U8 r = colour;
			U8 gb = colour | (colour << 4);

			gb = Memory::cRam[pal * 2 * 16 + colour * 2 + 0] & 0x0E;
			r = Memory::cRam[pal * 2 * 16 + colour * 2 + 1] & 0x0E;
			gb |= Memory::cRam[pal * 2 * 16 + colour * 2 + 1] & 0xE0;

			if (spriteInfo & 0x80)
			{
				doPixelSprite(x, curLine, r, gb, zValue << 4);
			}
			else
			{
				doPixelSprite(x, curLine, r, gb, zValue);
			}
		}
	}
}

#if ENABLE_32X_MODE

extern U8 FRAMEBUFFER[0x00040000];

extern U32 ActiveFrameBuffer;
extern U16 BitmapModeRegister;

#define USE_PALETTE		1

void Draw32XScreenRowPalette(U32* out, U8* displayPtr, int displaySizeX)
{
	int x;
#if USE_PALETTE
	U8 gr, bg;
#endif
	U32 colour;
	U8 pixel;

	for (x = 0; x<displaySizeX * 8; x++)
	{
		pixel = *displayPtr++;

#if !USE_PALETTE
		colour = (pixel << 16) + (pixel << 8) + (pixel);
#else
		bg = CRAM[pixel * 2 + 0];
		gr = CRAM[pixel * 2 + 1];

		colour = 0;
		colour |= (gr & 0x1F) << (3 + 8 + 8);
		colour |= (bg & 0x7C) << (1);
		colour |= (bg & 0x03) << (6 + 8);
		colour |= (gr & 0xE0) << (8 - 2);
#endif
		*out++ = colour;
	}
}

void Draw32XScreenRowDirect(U32* out, U8* displayPtr, int displaySizeX)
{
	int x;
	U8 gr, bg;
	U32 colour;
	/*	U8 pixel;*/

	for (x = 0; x<displaySizeX * 8; x++)
	{
		bg = *displayPtr++;
		gr = *displayPtr++;

		colour = 0;
		colour |= (gr & 0x1F) << (3 + 8 + 8);
		colour |= (bg & 0x7C) << (1);
		colour |= (bg & 0x03) << (6 + 8);
		colour |= (gr & 0xE0) << (8 - 2);
		*out++ = colour;
	}
}

void Draw32XScreenRowRLE(U32* out, U8* displayPtr, int displaySizeX)
{
	int x = 0;
	U8 gr, bg;
	U32 colour = 0;
	U8 pixel;
	U16 rle = 0;

	while (x<displaySizeX * 8)
	{
		if (rle == 0)
		{
			rle = *displayPtr++;
			pixel = *displayPtr++;

			bg = CRAM[pixel * 2 + 0];
			gr = CRAM[pixel * 2 + 1];

			colour = 0;
			colour |= (gr & 0x1F) << (3 + 8 + 8);
			colour |= (bg & 0x7C) << (1);
			colour |= (bg & 0x03) << (6 + 8);
			colour |= (gr & 0xE0) << (8 - 2);

			rle++;
			continue;
		}
		else
		{
			rle--;
		}

		*out++ = colour;
		x++;
	}
}


void Draw32XScreenRow(int y, U32* out, U8* zPos)
{
	int displaySizeX = (VDP::VDP_Registers[12] & 0x01) ? 40 : 32;
	U16 lineOffset;
	U8* displayPtr;

	UNUSED_ARGUMENT(zPos);

	if (!ActiveFrameBuffer)
		displayPtr = &FRAMEBUFFER[0x20000];
	else
		displayPtr = &FRAMEBUFFER[0];

#if 0
	lineOffset = FRAMEBUFFER[y * 2 + 0] << 8;
	lineOffset |= FRAMEBUFFER[y * 2 + 1];
#else
	lineOffset = displayPtr[y * 2 + 0] << 8;
	lineOffset |= displayPtr[y * 2 + 1];
#endif

	displayPtr += lineOffset * 2;

	switch (BitmapModeRegister & 0x0003)
	{
	case 0:
		return;
	case 1:
		Draw32XScreenRowPalette(out, displayPtr, displaySizeX);
		return;
	case 2:
		Draw32XScreenRowDirect(out, displayPtr, displaySizeX);
		return;
	case 3:
		Draw32XScreenRowRLE(out, displayPtr, displaySizeX);
		return;
	}
}

#endif

void VDP_SetDrawPlaneB(bool drawPlaneB)
{
	VDP::drawPlaneB = drawPlaneB;
}

void VDP_SetTransparentBG(bool transparentBG)
{
	VDP::transparentBG = transparentBG;
}

void VID_BeginFrame()
{
	//Clear sprite line cache
	memset(spriteTempBuffer, 0, spriteTempBufferSize);
}

#if !SMS_MODE
void VID_DrawScreenRow(int y)
{
	//Pixel output buffer
	U32* lineAddr = pixelPosition(0, 0 + y);

	//Z output buffer
	U8* pixelDepth = zBuffer;

	//Clear with BG colour
	if (VDP::transparentBG)
		DisplayFillTransparent(lineAddr, VDP_SCREEN_WIDTH);
	else
		DisplayFillBGColourLine(lineAddr, VDP_SCREEN_WIDTH);

	//Clear Z buffer
	memset(zBuffer, 0, VDP_SCREEN_WIDTH_TILES * 8);

#if ENABLE_32X_MODE
	Draw32XScreenRow(y, pixel, zPos);
#endif

	//Draw plane A/B row
	DrawScreenRow(y, lineAddr, pixelDepth);

	//Draw sprite row (at Y pos + sprite border)
	DrawSpritesForLine(y + VDP_SPRITE_BORDER_Y, 0x04);
	ComputeSpritesForNextLine(y + VDP_SPRITE_BORDER_Y + 1);

#if VDP_SCALE_2X
	//If scaling x2, duplicate line
	U32* lineB = lineAddr + (DRAW_BUFFER_WIDTH * 2);
	memcpy(lineB, lineAddr, DRAW_BUFFER_WIDTH * sizeof(U32) * 2);
#endif
}
#else

int SMS_ComputeColourXY(int tx, int ty, U32 address, U32 flipH, U32 flipV)
{
	int colour;
	U8 mask = 0x80;
	int planeShift = 7, p;
	U8 plane[4];

	if (flipV)
	{
		address += (7 - ty) * 4;
	}
	else
	{
		address += ty * 4;
	}
	if (flipH)
	{
		mask >>= 7 - tx;
		planeShift -= 7 - tx;
	}
	else
	{
		mask >>= tx;
		planeShift -= tx;
	}
	address &= 0x3FFF;

	for (p = 0; p<4; p++)
	{
		plane[p] = Memory::vRam[address + p];
	}

	colour = ((plane[3] & mask) >> planeShift) << 3;
	colour |= ((plane[2] & mask) >> planeShift) << 2;
	colour |= ((plane[1] & mask) >> planeShift) << 1;
	colour |= ((plane[0] & mask) >> planeShift) << 0;

	return colour;
}

void SMS_DrawTileXY(int tx, int ty, U32 address, int pal, U32 flipH, U32 flipV, U8 zValue, U32 *pixelPos, U8 *zPos)
{
	if (*zPos<zValue)
	{
		int colour = SMS_ComputeColourXY(tx, ty, address, flipH, flipV);

		/*if (colour!=0)			// seems the sms draws colour 0 - so we skip setting the zpos (which keeps the transparent value for sprite priority) */
		{
			U8 r = colour;
			U8 g = colour;
			U8 b = colour;
			U32 realColour;

			if (colour != 0)
			{
				*zPos = zValue;
			}

			r = (Memory::cRam[pal * 16 + colour] & 0x03) << 6;
			b = (Memory::cRam[pal * 16 + colour] & 0x0C) << 4;
			g = (Memory::cRam[pal * 16 + colour] & 0x30) << 2;

			realColour = (r << 16) | (b << 8) | g;

			*pixelPos = realColour;
		}
	}
}

S32 latchedScrollAmountY;

void SMS_DrawScreenRow(int ty, U8 zValue, U32 *pixelPos, U8 *zPos)
{
	int tx, ox, oty = ty, uty;
	int displaySizeX = 32;
	S32 scrollAmountY = latchedScrollAmountY;
	S32 scrollAmountX = VDP::VDP_Registers[0x08];
	U32 address = (VDP::VDP_Registers[0x02]) & 0x0E;
	U32 baseAddress;
	address <<= 10;
	baseAddress = address;

	ty += scrollAmountY;
	if (ty>223)
	{
		ty -= 224;
	}

	ox = 0;
	if (VDP::VDP_Registers[0x00] & 0x20)			/* left column disabled */
	{
		ox = 8;
		pixelPos += 8;
		zPos += 8;
	}

	for (; ox<displaySizeX * 8; ox++)			/* base address is vertical adjusted..  */
	{
		U16 tile;
		U16 tileAddress;
		int pal;

		if ((oty<16) && (VDP::VDP_Registers[0x00] & 0x40))
		{
			tx = ox;
		}
		else
		{
			tx = ox - scrollAmountX;
			tx &= (32 * 8 - 1);
		}
		if ((ox >= 24 * 8) && (VDP::VDP_Registers[0x00] & 0x80))
		{
			uty = oty;
		}
		else
		{
			uty = ty;
		}
		address = baseAddress + (tx / 8) * 2 + (uty / 8) * 32 * 2;
		address &= 0x3FFF;

		tile = (Memory::vRam[address + 1] << 8) | Memory::vRam[address + 0];

		tileAddress = tile & 0x01FF;			/* ---p cvhn nnnn nnnn */

		pal = (tile & 0x0800) >> 11;

		tileAddress <<= 5;

		if (tile & 0x1000)
		{
			SMS_DrawTileXY((tx & 7), (uty & 7), tileAddress, pal, tile & 0x0200, tile & 0x0400, zValue << 4, pixelPos, zPos);
		}
		else
		{
			SMS_DrawTileXY((tx & 7), (uty & 7), tileAddress, pal, tile & 0x0200, tile & 0x0400, zValue, pixelPos, zPos);
		}
		pixelPos++;
		zPos++;
	}
}

void SMS_ProcessSpriteTile(int xx, int yy, U32 address, int ScreenY)
{
	int x, y;
	int colour;

	address &= 0x3FFF;

	for (y = 0; y<8; y++)
	{
		U8 mask = 0x80;
		int planeShift = 7, p;
		U8 plane[4];

		for (p = 0; p<4; p++)
		{
			plane[p] = Memory::vRam[address + (y * 4) + p];
		}

		for (x = 0; x<8; x++)
		{
			colour = ((plane[3] & mask) >> planeShift) << 3;
			colour |= ((plane[2] & mask) >> planeShift) << 2;
			colour |= ((plane[1] & mask) >> planeShift) << 1;
			colour |= ((plane[0] & mask) >> planeShift) << 0;

			mask >>= 1;
			planeShift--;

			if (colour != 0)
			{
				ProcessSpritePixel(x + xx, y + yy, colour, ScreenY);
			}
		}
	}
}

void SMS_ComputeSpritesForNextLine(int nextLine)
{
	int x, y;
	U16 baseSpriteAddress = ((VDP::VDP_Registers[5] & 0x3E) << 8) | 0x100;
	int displaySizeX = 32;
	U16 spriteAddress;
	int curSNum = 0;
	int totSpritesScan = 8;

	memset(spriteTempBuffer, 0, displaySizeX * 8);			/* clear processing buffer down */

	spriteAddress = baseSpriteAddress & 0x3FFF;

	while (1)
	{
		U16 yPos = Memory::vRam[spriteAddress + curSNum];
		U16 xPos = Memory::vRam[spriteAddress + 0x80 + curSNum * 2];
		U16 tile = Memory::vRam[spriteAddress + 0x80 + curSNum * 2 + 1] << 5;
		U16 vSize = 0;
		U16 hSize = 0;
		U16 tileAddress = (VDP::VDP_Registers[0x06] & 0x04) << 11;

		if (VDP::VDP_Registers[0x01] & 0x02)
		{
			tile &= ~0x20;		/* sprite tiles for 16 high are always even first */
			vSize += 1;
		}
		if (yPos == 0xD0)
			break;
		if (yPos>0xD0)
			yPos -= 256;

		yPos += 128;
		xPos += 128;
		if ((nextLine >= yPos) && (nextLine <= (yPos + ((8 * (1 + vSize)) - 1))))			/* not interested if not current line */
		{
			/* Need to draw correct number of sprites for one line etc. */

			totSpritesScan--;
			if (!totSpritesScan)
				break;

			for (x = 0; x <= hSize; x++)
			{
				for (y = 0; y <= vSize; y++)
				{
					SMS_ProcessSpriteTile(xPos + x * 8, yPos + y * 8, tileAddress + tile + y * 32, nextLine);
				}
			}
		}

		curSNum++;
		if (curSNum>63)
			break;
	}
}

void SMS_DrawSpritesForLine(int curLine, U8 zValue)
{
	int x;
	int displaySizeX = 32;

	for (x = 0; x<displaySizeX * 8; x++)
	{
		U8 spriteInfo = spriteTempBuffer[x];

		if (spriteInfo != 0)
		{
			int colour = spriteInfo & 0x0F;
			U8 r = colour;
			U8 gb = colour | (colour << 4);

			r = (Memory::cRam[1 * 16 + colour] & 0x03) << 2;
			gb = (Memory::cRam[1 * 16 + colour] & 0x0C) << 4;
			gb |= (Memory::cRam[1 * 16 + colour] & 0x30) >> 2;

			doPixelZ(128 + x, curLine, r, gb, zValue);
		}
	}
}



void VID_DrawScreenRow(int y)
{
	/*
	if (y==0)		is this correct.. should the y scroll latch?
	*/
	{
		latchedScrollAmountY = VDP::VDP_Registers[0x09];
	}

	if (y<192)
	{
		U32 *pixel = pixelPosition(128, 128 + y);
		U8 *zPos = zBuffer;

		SMS_DisplayFillBGColourLine(pixel, 320);

		memset(zBuffer, 0, 40 * 8);

		SMS_DrawScreenRow(y, 0x02, pixel, zPos);

		SMS_DrawSpritesForLine(128 + y, 0x04);
		SMS_ComputeSpritesForNextLine(128 + y + 1);

	}
}
#endif
