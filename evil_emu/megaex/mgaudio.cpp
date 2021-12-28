#include "mgaudio.h"
#include <stdio.h>
#include <ion/engine/Engine.h>
#include <ion/core/time/Time.h>

static ion::audio::Voice* ionAudioVoice = NULL;
static IonAudioStreamDesc ionAudioStreamDesc;
static IonAudioSource ionAudioSource;

#if EMU_ENABLE_AUDIO
static ion::audio::Buffer* ionAudioBuffers[AUDIO_NUM_BUFFERS] = { nullptr };
static u32 audioProducerBufferIdx = 0;
#endif

static s16 audioDAC[AUDIO_DAC_COUNT][AUDIO_NUM_CHANNELS] = { { 0 } };
static int audioPSGClock = 0;
static int audioSamplesWritten = 0;

#if !EMU_ENABLE_AUDIO
static float audioStartTime = 0.0f;
#endif

LJ_YM2612* YM2612::ym2612_chip = NULL;

void AudioInitialise()
{
	//Create FM chip
	YM2612::ym2612_chip = LJ_YM2612_create(LJ_YM2612_DEFAULT_CLOCK_RATE, AUDIO_SAMPLE_RATE_HZ);

#if defined ION_BUILD_DEBUG
	LJ_YM2612_setFlags(YM2612::ym2612_chip, LJ_YM2612_DEBUG);
#endif

#if EMU_ENABLE_AUDIO
	//Create buffers
	for (int i = 0; i < AUDIO_NUM_BUFFERS; i++)
	{
		ionAudioBuffers[i] = ion::audio::Buffer::Create(AUDIO_BUFFER_LEN_BYTES);
	}
#endif
}

void AudioBeginPlayback()
{
#if EMU_ENABLE_AUDIO
	if (!ionAudioVoice)
	{
		//Create voice
		ionAudioVoice = ion::engine.audio.engine->CreateVoice(ionAudioSource, false);

		//Fill and submit initial (silent) buffers
		for (int i = 0; i < AUDIO_NUM_INITIAL_BUFFERS; i++)
		{
			int bufferIdx = audioProducerBufferIdx % AUDIO_NUM_BUFFERS;

			ionAudioBuffers[bufferIdx]->WriteLock();
			ionAudioBuffers[bufferIdx]->Reserve(AUDIO_BUFFER_LEN_BYTES);
			ionAudioBuffers[bufferIdx]->WriteUnlock();

			ionAudioVoice->SubmitBuffer(*ionAudioBuffers[bufferIdx]);
			ion::thread::atomic::Increment(audioProducerBufferIdx);
		}

		//Lock and clear next buffer ready for filling
		ionAudioBuffers[audioProducerBufferIdx % AUDIO_NUM_BUFFERS]->WriteLock();
		ionAudioBuffers[audioProducerBufferIdx % AUDIO_NUM_BUFFERS]->Clear();
	}

	if (ionAudioVoice->GetState() == ion::audio::Voice::State::Stopped)
	{
		//Begin playback
		ionAudioVoice->Play();
	}
#else
	audioStartTime = ion::time::TicksToSeconds(ion::time::GetSystemTicks());
#endif
}

void AudioStopPlayback()
{
#if EMU_ENABLE_AUDIO
	if (ionAudioVoice->GetState() != ion::audio::Voice::State::Stopped)
	{
		ionAudioVoice->Stop();
	}
#endif
}

void AudioPausePlayback()
{
#if EMU_ENABLE_AUDIO
	if(ionAudioVoice)
		ionAudioVoice->Pause();
#endif
}

void AudioResumePlayback()
{
#if EMU_ENABLE_AUDIO
	if(ionAudioVoice)
		ionAudioVoice->Resume();
#endif
}

void AudioDACUpdate()
{
#if EMU_ENABLE_AUDIO
	//Get current DAC samples
	S16 sampleL = (audioDAC[AUDIO_DAC_PSG][0] + audioDAC[AUDIO_DAC_FM][0]);
	S16 sampleR = (audioDAC[AUDIO_DAC_PSG][1] + audioDAC[AUDIO_DAC_FM][1]);

	//Get current buffer
	ion::audio::Buffer* buffer = ionAudioBuffers[audioProducerBufferIdx % AUDIO_NUM_BUFFERS];

	//Fill buffer
	buffer->Add((const char*)&sampleL, AUDIO_BUFFER_FORMAT_SIZE);
	buffer->Add((const char*)&sampleR, AUDIO_BUFFER_FORMAT_SIZE);

	audioSamplesWritten++;

	//If buffer full
	if (buffer->GetDataSize() == AUDIO_BUFFER_LEN_BYTES)
	{
		//Unlock buffer
		buffer->WriteUnlock();

		//Submit
		ionAudioVoice->SubmitBuffer(*buffer);

		//Next buffer
		ion::thread::atomic::Increment(audioProducerBufferIdx);

		//Lock and reset next buffer
		buffer = ionAudioBuffers[audioProducerBufferIdx % AUDIO_NUM_BUFFERS];
		buffer->WriteLock();
		buffer->Reset();
	}
#endif
}

void AudioSetDAC(int channel, S16 dacValueL, S16 dacValueR)
{
	audioDAC[channel][0] = dacValueL;
	audioDAC[channel][1] = dacValueR;
}

double AudioGetClock()
{
#if EMU_ENABLE_AUDIO
	if (ionAudioVoice && (ionAudioVoice->GetState() != ion::audio::Voice::State::Stopped))
	{
		return ionAudioVoice->GetPositionSeconds();
	}
	else
	{
		return 0.0f;
	}
#else
	return ion::time::TicksToSeconds(ion::time::GetSystemTicks()) - audioStartTime;
#endif
}

int AudioGetBuffersQueued()
{
	return ionAudioVoice ? ionAudioVoice->GetQueuedBuffers() : 0;
}

u64 AudioGetSamplesWritten()
{
	return audioSamplesWritten;
}

u64 AudioGetSamplesPlayed()
{
	return ionAudioVoice ? ionAudioVoice->GetPositionSamples() : 0;
}

void AudioFMUpdate()
{
	static U32 divisor = CYCLES_PER_SECOND_68K / AUDIO_SAMPLE_RATE_HZ;
	
	if (divisor>0)
	{
		divisor--;
		return;
	}
	
	divisor = CYCLES_PER_SECOND_68K / AUDIO_SAMPLE_RATE_HZ;

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
	: ion::audio::Source(ion::audio::Source::FeedType::Streaming)
{
	m_streamDesc = &ionAudioStreamDesc;
}

bool IonAudioSource::OpenStream(OnStreamOpened const& onOpened)
{
	return true;
}

void IonAudioSource::CloseStream(OnStreamClosed const& onClosed)
{
};

void IonAudioSource::RequestBuffer(ion::audio::SourceCallback& callback)
{

}