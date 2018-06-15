#include "mgaudio.h"
#include <stdio.h>

LJ_YM2612* ym2612_chip = NULL;

ion::audio::Engine* ionAudioEngine = NULL;
ion::audio::Buffer* ionAudioBuffers[AUDIO_NUM_BUFFERS];
ion::audio::Voice* ionAudioVoice = NULL;
ion::Queue<ion::audio::Buffer*, AUDIO_NUM_BUFFERS> ionAudioBufferQueue;
ion::audio::SourceCallback* ionAudioStarvedSource = NULL;
IonAudioStreamDesc ionAudioStreamDesc;
IonAudioSource ionAudioSource;

s16 audioDAC[AUDIO_DAC_COUNT] = { 0, 0 };
int audioBufferIdx = 0;
int audioWritePtr = 0;
u64 audioSamplesWritten = 0;
float audioClock = 0.0f;
int audioPSGClock = 0;

void AudioInitialise()
{
	ionAudioEngine = ion::audio::Engine::Create();

	for (int i = 0; i < AUDIO_NUM_BUFFERS; i++)
	{
		ionAudioBuffers[i] = new ion::audio::Buffer(AUDIO_BUFFER_LEN_BYTES);
		ionAudioBuffers[i]->Lock();
	}
}

void AudioTick(float deltaTime)
{
	audioClock += 1.0f;

	while (audioClock >= AUDIO_FILL_RATE)
	{
		audioClock -= AUDIO_FILL_RATE;

		if ((audioSamplesWritten % (44100 / 4)) == 0)
		{
			u64 samplesPlayed = ionAudioVoice ? ionAudioVoice->GetPositionSamples() : 0;
			printf("Samples ahead: %i\n", audioSamplesWritten - samplesPlayed);
		}

		//Get current DAC sample
		S16 sample = (audioDAC[AUDIO_DAC_PSG] + audioDAC[AUDIO_DAC_FM]); // >> 1;

		//Fill buffer
		ionAudioBuffers[audioBufferIdx]->Put((const char*)&sample, AUDIO_BUFFER_FORMAT_SIZE, audioWritePtr);

		//Advance buffer
		audioWritePtr += AUDIO_BUFFER_FORMAT_SIZE;
		audioSamplesWritten++;

		//If buffer full
		if (audioWritePtr == AUDIO_BUFFER_LEN_BYTES)
		{
			//If voice is starved
			if (ionAudioStarvedSource)
			{
				//Submit it immediately
				ionAudioStarvedSource->SubmitBuffer(*ionAudioBuffers[audioBufferIdx]);
				ionAudioStarvedSource = NULL;
			}
			else
			{
				//Push to queue
				ionAudioBufferQueue.Push(ionAudioBuffers[audioBufferIdx]);
			}

			//Start filling next buffer
			audioWritePtr = 0;
			audioBufferIdx = (audioBufferIdx + 1) % AUDIO_NUM_BUFFERS;
		}

		//Create and play voice after at least first buffer filled
		if (!ionAudioVoice && !ionAudioBufferQueue.IsEmpty())
		{
			ionAudioVoice = ionAudioEngine->CreateVoice(ionAudioSource, true);
			ionAudioVoice->Play();
		}
	}
}

void AudioSetDAC(int channel, S16 dacValue)
{
	audioDAC[channel] = dacValue;
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

void IonAudioSource::RequestBuffer(ion::audio::SourceCallback& callback)
{
	if (ionAudioBufferQueue.IsEmpty())
	{
		//printf("Voice starved, waiting for next buffer\n");
		ionAudioStarvedSource = &callback;
	}
	else
	{
		callback.SubmitBuffer(*ionAudioBufferQueue.Pop());
		ionAudioStarvedSource = NULL;
	}
}
