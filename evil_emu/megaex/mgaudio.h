#ifndef _MG_AUDIO_H
#define _MG_AUDIO_H

#include <ion/audio/Engine.h>
#include <ion/audio/Buffer.h>
#include <ion/audio/Source.h>
#include <ion/audio/StreamDesc.h>
#include <ion/audio/Voice.h>
#include <ion/core/containers/Queue.h>

#include "config.h"
#include "platform.h"
#include "audio/lj_ym2612.h"
#include "audio/psg.h"

#define AUDIO_BUFFER_LEN_SAMPLES	2048

#define AUDIO_NUM_CHANNELS			2
#define AUDIO_BUFFER_FORMAT			s16
#define AUDIO_BUFFER_FORMAT_SIZE	sizeof(AUDIO_BUFFER_FORMAT)
#define AUDIO_SAMPLE_RATE_HZ		44100
#define AUDIO_NUM_BUFFERS			4
#define AUDIO_NUM_INITIAL_BUFFERS	2
#define AUDIO_BUFFER_LEN_BYTES		(AUDIO_BUFFER_LEN_SAMPLES*AUDIO_BUFFER_FORMAT_SIZE*AUDIO_NUM_CHANNELS)
#define AUDIO_FILL_RATE				((double)CYCLES_PER_SECOND_68K / (double)AUDIO_SAMPLE_RATE_HZ)

enum AudioDACChannel
{
	AUDIO_DAC_PSG,
	AUDIO_DAC_FM,

	AUDIO_DAC_COUNT
};

class IonAudioStreamDesc : public ion::audio::StreamDesc
{
public:
	virtual ion::audio::DataFormat GetEncodedFormat() const { return ion::audio::DataFormat::PCM16; }
	virtual ion::audio::DataFormat GetDecodedFormat() const { return ion::audio::DataFormat::PCM16; }

	virtual u32 GetNumChannels() const { return AUDIO_NUM_CHANNELS; }
	virtual u32 GetSampleRate() const { return AUDIO_SAMPLE_RATE_HZ; }
	virtual u32 GetBitsPerSample() const { return AUDIO_BUFFER_FORMAT_SIZE * 8; }
	virtual u32 GetBlockSize() const { return (GetNumChannels() * GetBitsPerSample()) / 8; }
	virtual u32 GetEncodedSizeBytes() const { return AUDIO_BUFFER_LEN_BYTES; }
	virtual u32 GetDecodedSizeBytes() const { return AUDIO_BUFFER_LEN_BYTES; }
	virtual u32 GetSizeSamples() const { return AUDIO_BUFFER_LEN_SAMPLES; }
};

class IonAudioSource : public ion::audio::Source
{
public:
	IonAudioSource();
	virtual bool OpenStream(OnStreamOpened const& onOpened);
	virtual void CloseStream(OnStreamClosed const& onClosed);
	virtual void RequestBuffer(ion::audio::SourceCallback& callback);
};

void AudioInitialise();
void AudioBeginPlayback();
void AudioStopPlayback();
void AudioPausePlayback();
void AudioResumePlayback();
void AudioDACUpdate();
void AudioFMUpdate();
void AudioPSGUpdate();
void AudioSetDAC(int channel, S16 dacValueL, S16 dacValueR);
double AudioGetClock();

//Debugging
int AudioGetBuffersQueued();
u64 AudioGetSamplesWritten();
u64 AudioGetSamplesPlayed();

class YM2612
{
public:
	static LJ_YM2612* ym2612_chip;
};

#endif
