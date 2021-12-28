#include "cdaudio.h"

#if EVIL_EMU_CD_AUDIO

#include "cdtracks.h"
#include <ion/engine/Engine.h>

const float AudioStream::s_fadeSpeed = 4.0f;

AudioStream::AudioStream()
{
	m_fileReader = nullptr;
	m_fileSource = nullptr;
	m_voice = nullptr;
	m_currentTrackId = 0;
}

void AudioStream::PlayTrack(int trackId)
{
	if (trackId > 0 && trackId <= s_numCDTracks)
	{
		if (trackId != m_currentTrackId)
		{
			if (m_fileSource)
			{
				StopTrack();
			}

			const char* filename = s_CDTrackFilenames[trackId - 1];
			m_currentTrackId = trackId;

			//Create reader and streamer
			m_fileReader = new ion::audio::FileReaderWAV(filename);
			m_fileSource = new ion::audio::FileSource(ion::audio::Source::FeedType::Streaming, *m_fileReader, true);

			//Open stream, then create and play voice
			m_fileSource->OpenStream(
				[&](ion::audio::Source& source, bool success)
				{
					if (success)
					{
						m_voice = ion::engine.audio.engine->CreateVoice(source, true);
						m_fader = m_voice->CreateEffect<ion::audio::EffectFader>();
						m_fader->FadeIn(s_fadeSpeed);
						m_voice->Play();
					}
					else
					{
						StopTrack();
					}
				});
		}
	}
	else
	{
		StopTrack();
	}
}

void AudioStream::StopTrack()
{
	if (m_fileSource)
	{
		if (m_voice)
		{
			//Fade out, then stop voice and close stream
			m_fader->FadeOut(s_fadeSpeed,
				[fileReader = m_fileReader, fileSource = m_fileSource, voice = m_voice](ion::audio::EffectFader& fader)
			{
				//Stop voice
				voice->Stop();

				//Close stream, then release voice
				fileSource->CloseStream(
					[fileReader, fileSource, voice](ion::audio::Source& source, bool success)
				{
					ion::engine.audio.engine->ReleaseVoice(*voice);
					delete fileSource;
					delete fileReader;
				});
			});
		}
		else
		{
			//Close stream
			m_fileSource->CloseStream(
				[fileReader = m_fileReader, fileSource = m_fileSource](ion::audio::Source& source, bool success)
			{
				delete fileSource;
				delete fileReader;
			});
		}
	}

	m_fileReader = nullptr;
	m_fileSource = nullptr;
	m_voice = nullptr;
	m_fader = nullptr;
	m_currentTrackId = 0;
}

#endif