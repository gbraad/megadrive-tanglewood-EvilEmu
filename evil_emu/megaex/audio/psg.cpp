/*
 *  psg.c

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
#include "psg.h"

// Registers
U8	PSG_RegsAttenuation[4]={0xF,0xF,0xF,0xF};
U16 PSG_RegsCounters[4] = { 0,0,0,0 };

// Current counters
U16	PSG_ToneCounter[4] = { 0,0,0,0 };

// Current tone output edges
U16 PSG_ToneOut[4] = { 0,0,0,0 };

U16 PSG_MasterClock=0;
U8 PSG_AddressLatch = 0;
U16 PSG_NoiseShiftRegister = 0x8000;
U16 PSG_NoiseCounter = 0;

void PSG_UpdateTones()
{
	for (int a = 0; a < 3; a++)
	{
		if (PSG_ToneCounter[a] == 0)	/* special case, always set high */
		{
			PSG_ToneOut[a] = 1;
			PSG_ToneCounter[a] = PSG_RegsCounters[a];
		}
		else
		{
			//Decrement counter
			if ((--PSG_ToneCounter[a]) == 0)
			{
				//Counter elapsed, flip waveform edge
				PSG_ToneOut[a] ^= 1;

				//Reset counter
				PSG_ToneCounter[a] = PSG_RegsCounters[a];
			}
		}
	}
}

void PSG_UpdateNoiseShiftRegister()
{
	U16 newBit=0;
	PSG_ToneOut[3]^=PSG_NoiseShiftRegister&0x01;
	if (PSG_RegsCounters[3]&0x4)
	{
		newBit = ((PSG_NoiseShiftRegister&0x08)>>3) ^ (PSG_NoiseShiftRegister&0x01);
	}
	else
	{
		newBit = (PSG_NoiseShiftRegister&0x01);
	}
	PSG_NoiseShiftRegister>>=1;
	PSG_NoiseShiftRegister|=newBit<<15;
}

void PSG_UpdateNoise()
{
	if (PSG_ToneCounter[3]==0)		/* special case, use tone 2 value as input - but tone2 was off, so re-load */
	{
		PSG_ToneCounter[3]=PSG_ToneCounter[2];
	}
	else
	{
		PSG_ToneCounter[3]--;
		if (PSG_ToneCounter[3]==0)
		{
			if (PSG_NoiseCounter==0)
			{
				PSG_UpdateNoiseShiftRegister();
			}
			PSG_NoiseCounter^=1;
			PSG_ToneCounter[3]=(PSG_RegsCounters[3]+1)&0x3;
			if (PSG_ToneCounter[3]==0)
			{
				PSG_ToneCounter[3]=PSG_ToneCounter[2];
			}
			else
			{
				PSG_ToneCounter[3]<<=4;	
			}
		}
	}
}

#define VOL_TYPE		U16

// From smspower.org
VOL_TYPE logScale[16] =
{
	0,
	1304 / 14,
	1642 / 14,
	2067 / 14,
	2603 / 14,
	3277 / 14,
	4125 / 14,
	5193 / 14,
	6568 / 14,
	8231 / 14,
	10362 / 14,
	13045 / 14,
	16422 / 14,
	20675 / 14,
	26028 / 14,
	32767 / 14
};

U16 PSG_Output()
{
	U16 sample = 0;

	for (int a = 0; a < 4; a++)
	{
		//If waveform edge high
		if (PSG_ToneOut[a])
		{
			//Output at current volume
			sample += logScale[15 - PSG_RegsAttenuation[a]];
		}
	}

	return sample;
}

void PSG_UpdateRegLo(U8 data)
{
	//Latch bits 6-5 = channel
	int channel = (PSG_AddressLatch&0x60)>>5;

	//Latch bit 4 = write attenuation reg
	if (PSG_AddressLatch&0x10)
	{
		PSG_RegsAttenuation[channel]=data&0x0F;
	}
	else
	{
		//Write lower 4 bits
		PSG_RegsCounters[channel] = (data & 0x0F) | (PSG_RegsCounters[channel] & ~0x000F);

		//If noise channel
		if (channel == 3)
		{
			PSG_NoiseShiftRegister=0x8000;
			PSG_ToneCounter[3]=(PSG_RegsCounters[3]+1)&0x3;
			if (PSG_ToneCounter[3]==0)
			{
				PSG_ToneCounter[3]=PSG_ToneCounter[2];
			}
			else
			{
				PSG_ToneCounter[3]<<=4;	
			}
		}
	}
}

void PSG_UpdateRegHi(U8 data)
{
	//Latch bits 6-5 = channel
	int channel = (PSG_AddressLatch&0x60)>>5;

	//Latch bit 4 = write attenuation reg
	if (PSG_AddressLatch&0x10)
	{
		PSG_RegsAttenuation[channel]=data&0x0F;
	}
	else
	{
		//If noise channel
		if (channel == 3)
		{
			PSG_RegsCounters[channel]&=~0x0F;
			PSG_RegsCounters[channel]|=data&0x0F;
			PSG_NoiseShiftRegister=0x8000;
			PSG_ToneCounter[3]=(PSG_RegsCounters[3]+1)&0x3;
			if (PSG_ToneCounter[3]==0)
			{
				PSG_ToneCounter[3]=PSG_ToneCounter[2];
			}
			else
			{
				PSG_ToneCounter[3]<<=4;
			}
		}
		else
		{
			//Write upper 6 bits
			PSG_RegsCounters[channel] = (data << 4) | (PSG_RegsCounters[channel] & ~0xFFF0);
		}
	}
}

void PSG_Write(U8 data)
{
	if (data&0x80)
	{
		//Latch on, write low byte
		PSG_AddressLatch=data;
		PSG_UpdateRegLo(data);
	}
	else
	{
		//Latch off, write high byte
		PSG_UpdateRegHi(data);
	}
}
