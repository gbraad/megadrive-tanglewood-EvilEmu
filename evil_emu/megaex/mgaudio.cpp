#include "mgaudio.h"
#include <stdio.h>

static ion::audio::Engine* ionAudioEngine = NULL;
static ion::audio::Voice* ionAudioVoice = NULL;
static ion::audio::Buffer* ionAudioBuffer = NULL;
static IonAudioStreamDesc ionAudioStreamDesc;
static IonAudioSource ionAudioSource;

static s16 audioDAC[AUDIO_DAC_COUNT][AUDIO_NUM_CHANNELS] = { 0 };
static double audioClock = 0.0;
static float audioClockFM = 0.0f;
static int audioPSGClock = 0;

LJ_YM2612* YM2612::ym2612_chip = NULL;

void AudioInitialise()
{
	//Create audio engine
	ionAudioEngine = ion::audio::Engine::Create();

	//Create buffer
	ionAudioBuffer = new ion::audio::Buffer(AUDIO_BUFFER_LEN_BYTES);

	//Create voice
	ionAudioVoice = ionAudioEngine->CreateVoice(ionAudioSource, true);

	//XAudio2 voice cannot be starved or it will stop the timer
#if !defined ION_AUDIO_SUPPORTS_SDL2

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

void AudioPausePlayback()
{
	ionAudioVoice->Pause();
}

void AudioResumePlayback()
{
	ionAudioVoice->Resume();
}

void AudioTick()
{
	audioClock += 1.0;

	while (audioClock >= AUDIO_FILL_RATE)
	{
		audioClock -= AUDIO_FILL_RATE;

		//Get current DAC samplesw
		S16 sampleL = (audioDAC[AUDIO_DAC_PSG][0] + audioDAC[AUDIO_DAC_FM][0]);
		S16 sampleR = (audioDAC[AUDIO_DAC_PSG][1] + audioDAC[AUDIO_DAC_FM][1]);

		//Fill buffer
		ionAudioBuffer->Add((const char*)&sampleL, AUDIO_BUFFER_FORMAT_SIZE);
		ionAudioBuffer->Add((const char*)&sampleR, AUDIO_BUFFER_FORMAT_SIZE);

		//If buffer full
		if (ionAudioBuffer->GetDataSize() == AUDIO_BUFFER_LEN_BYTES)
		{
			//Unlock buffer
			ionAudioBuffer->Unlock();

			//Don't over fill
			u32 bufferedBytes = ionAudioVoice->GetBufferedBytes();
			if (bufferedBytes < AUDIO_BUFFER_LEN_BYTES * 8)
			{
				//Submit buffer
				ionAudioVoice->SubmitBuffer(*ionAudioBuffer);
			}

			//Lock and reset buffer
			ionAudioBuffer->Lock();
			ionAudioBuffer->Reset();
		}
	}
}

void AudioSetDAC(int channel, S16 dacValueL, S16 dacValueR)
{
	audioDAC[channel][0] = dacValueL;
	audioDAC[channel][1] = dacValueR;
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

	if (YM2612::ym2612_chip == NULL)
	{
		YM2612::ym2612_chip = LJ_YM2612_create(LJ_YM2612_DEFAULT_CLOCK_RATE, AUDIO_SAMPLE_RATE_HZ);
	}

	LJ_YM_INT16 channels[2];
	LJ_YM_INT16* out[2] = { &channels[0] , &channels[1] };
	LJ_YM2612_generateOutput(YM2612::ym2612_chip, 1, out);

	AudioSetDAC(0, channels[0], channels[1]);
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
		AudioSetDAC(1, PSG_Output(), PSG_Output());
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

