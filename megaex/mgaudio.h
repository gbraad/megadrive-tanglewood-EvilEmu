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
#include "lj_ym2612.h"
#include "psg.h"

#define AUDIO_BUFFER_FORMAT			s16
#define AUDIO_BUFFER_FORMAT_SIZE	sizeof(AUDIO_BUFFER_FORMAT)

#define AUDIO_SAMPLE_RATE_HZ		44100
#define AUDIO_BUFFER_LEN_SAMPLES	256
#define AUDIO_BUFFER_LEN_BYTES		(AUDIO_BUFFER_LEN_SAMPLES*AUDIO_BUFFER_FORMAT_SIZE)
#define AUDIO_FILL_RATE				((double)(CYCLES_PER_FRAME_68K * FRAMES_PER_SECOND_NTSC) / (double)AUDIO_SAMPLE_RATE_HZ)

enum AudioDACChannel
{
	AUDIO_DAC_PSG,
	AUDIO_DAC_FM,

	AUDIO_DAC_COUNT
};

class IonAudioStreamDesc : public ion::audio::StreamDesc
{
public:
	virtual ion::audio::DataFormat GetEncodedFormat() const { return ion::audio::PCM16; }
	virtual ion::audio::DataFormat GetDecodedFormat() const { return ion::audio::PCM16; }

	virtual u32 GetNumChannels() const { return 1; }
	virtual u32 GetSampleRate() const { return AUDIO_SAMPLE_RATE_HZ; }
	virtual u32 GetBitsPerSample() const { return AUDIO_BUFFER_FORMAT_SIZE * 8; }
	virtual u32 GetBlockSize() const { return (GetNumChannels() * GetBitsPerSample()) / 8; }
	virtual u32 GetEncodedSizeBytes() const { return AUDIO_BUFFER_LEN_BYTES; }
	virtual u32 GetDecodedSizeBytes() const { return AUDIO_BUFFER_LEN_BYTES; }
	virtual u32 GetSizeSamples() const { return AUDIO_BUFFER_LEN_BYTES / AUDIO_BUFFER_FORMAT_SIZE; }
};

class IonAudioSource : public ion::audio::Source
{
public:
	IonAudioSource();
	virtual bool OpenStream();
	virtual void CloseStream();
	virtual void RequestBuffer(ion::audio::SourceCallback& callback) {}
};

void AudioInitialise();
void AudioBeginPlayback();
void AudioStopPlayback();
void AudioTick();
void AudioFMUpdate();
void AudioPSGUpdate();
void AudioSetDAC(int channel, S16 dacValue);
double AudioGetClock();

extern LJ_YM2612* ym2612_chip;

extern ion::audio::Engine* ionAudioEngine;
extern ion::audio::Voice* ionAudioVoice;
extern ion::audio::Buffer* ionAudioBuffer;
extern IonAudioStreamDesc ionAudioStreamDesc;
extern IonAudioSource ionAudioSource;

extern s16 audioDAC[AUDIO_DAC_COUNT];

extern double audioClock;
extern int audioPSGClock;

#endif
