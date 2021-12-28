#pragma once

#include "constants.h"

#if EVIL_EMU_CD_AUDIO

#include <ion/audio/FileReaderWAV.h>
#include <ion/audio/FileSource.h>
#include <ion/audio/Voice.h>
#include <ion/audio/EffectFader.h>

class AudioStream
{
public:
	AudioStream();

	void PlayTrack(int trackId);
	void StopTrack();

private:
	static const float s_fadeSpeed;

	ion::audio::FileReaderWAV* m_fileReader;
	ion::audio::FileSource* m_fileSource;
	ion::audio::Voice* m_voice;
	ion::audio::EffectFader* m_fader;
	int m_currentTrackId;
};

#endif