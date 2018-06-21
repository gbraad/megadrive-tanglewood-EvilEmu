#include "mgaudio.h"
#include <stdio.h>

LJ_YM2612* ym2612_chip = NULL;

ion::audio::Engine* ionAudioEngine = NULL;
ion::audio::Voice* ionAudioVoice = NULL;
ion::audio::Buffer* ionAudioBuffer = NULL;
IonAudioStreamDesc ionAudioStreamDesc;
IonAudioSource ionAudioSource;

s16 audioDAC[AUDIO_DAC_COUNT] = { 0, 0 };
double audioClock = 0.0;
float audioClockFM = 0.0f;
int audioPSGClock = 0;

void AudioInitialise()
{
	//Create audio engine
	ionAudioEngine = ion::audio::Engine::Create();

	//Create buffer
	ionAudioBuffer = new ion::audio::Buffer(AUDIO_BUFFER_LEN_BYTES);

	//Create voice
	ionAudioVoice = ionAudioEngine->CreateVoice(ionAudioSource, true);

	//XAudio2 voice cannot be starved or it will stop the timer
#if !defined ION_AUDIO_SUPPORTS_SDL

	//Lead with one buffer of silence
	ionAudioBuffer->Lock();

	u16 sample = 0;
	for (int i = 0; i < AUDIO_BUFFER_LEN_SAMPLES; i++)
	{
		ionAudioBuffer->Add((const char*)&sample, AUDIO_BUFFER_FORMAT_SIZE);
	}

	ionAudioBuffer->Unlock();
	ionAudioVoice->SubmitBuffer(*ionAudioBuffer);

#endif

	//Lock and clear buffer ready for filling
	ionAudioBuffer->Lock();
	ionAudioBuffer->Clear();
}

void AudioBeginPlayback()
{
	if (ionAudioVoice->GetState() != ion::audio::Voice::Playing)
	{
		ionAudioVoice->Play();
	}
}

void AudioStopPlayback()
{
	if (ionAudioVoice->GetState() != ion::audio::Voice::Stopped)
	{
		ionAudioVoice->Stop();
	}
}

void AudioTick()
{
	audioClock += 1.0;

	while (audioClock >= AUDIO_FILL_RATE)
	{
		audioClock -= AUDIO_FILL_RATE;

		//Get current DAC samplesw
		S16 sample = (audioDAC[AUDIO_DAC_PSG] + audioDAC[AUDIO_DAC_FM]); // >> 1;

		//Fill buffer
		ionAudioBuffer->Add((const char*)&sample, AUDIO_BUFFER_FORMAT_SIZE);

		//If buffer full
		if (ionAudioBuffer->GetDataSize() == AUDIO_BUFFER_LEN_BYTES)
		{
			//Unlock buffer
			ionAudioBuffer->Unlock();

			//Don't over fill
			u32 bufferedBytes = ionAudioVoice->GetBufferedBytes();
			//printf("Buffered bytes: %i\n", bufferedBytes);
			if (bufferedBytes < AUDIO_BUFFER_LEN_BYTES * 8)
			{
				//Submit buffer
				ionAudioVoice->SubmitBuffer(*ionAudioBuffer);
			}
			else
			{
				bool breakMe = true;
			}

			//Lock and reset buffer
			ionAudioBuffer->Lock();
			ionAudioBuffer->Reset();
		}
	}
}

void AudioSetDAC(int channel, S16 dacValue)
{
	audioDAC[channel] = dacValue;
}

double AudioGetClock()
{
	return ionAudioVoice ? ionAudioVoice->GetPositionSeconds() : 0.0;
}

void AudioFMUpdate()
{
	static U32 divisor = (CYCLES_PER_FRAME_68K*FRAMES_PER_SECOND_NTSC) / AUDIO_SAMPLE_RATE_HZ;
	
	if (divisor>0)
	{
		divisor--;
		return;
	}
	
	divisor = (CYCLES_PER_FRAME_68K*FRAMES_PER_SECOND_NTSC) / AUDIO_SAMPLE_RATE_HZ;

	if (ym2612_chip == NULL)
	{
		ym2612_chip = LJ_YM2612_create(LJ_YM2612_DEFAULT_CLOCK_RATE, AUDIO_SAMPLE_RATE_HZ);
	}

	LJ_YM_INT16 channels[2];
	LJ_YM_INT16* out[2] = { &channels[0] , &channels[1] };
	LJ_YM2612_generateOutput(ym2612_chip, 1, out);

	AudioSetDAC(0, (channels[0] + channels[1]) >> 1);
}

void AudioPSGUpdate()
{
	//Divide clock
	if ((++audioPSGClock) >= 16)
	{
		//Elapsed
		audioPSGClock = 0;

		//Tick channels
		PSG_UpdateTones();
		PSG_UpdateNoise();

		//Output to buffer
		AudioSetDAC(1, PSG_Output());
	}
}

IonAudioSource::IonAudioSource()
	: ion::audio::Source(ion::audio::Source::StreamingFeed)
{
	mStreamDesc = &ionAudioStreamDesc;
}

bool IonAudioSource::OpenStream()
{
	return true;
}

void IonAudioSource::CloseStream()
{
};

