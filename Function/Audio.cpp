#include "dr_wav.h"   // Enables WAV decoding.
#include "mini_al.h"


#include "Audio.hpp"

int FAudioPlayer::Initialize(const char * name)
{
	mal_result result = mal_decoder_init_file(name, NULL, &decoder);
	if (result != MAL_SUCCESS)
	{
		return -2;
	}
	mal_device_config config = mal_device_config_init_playback(decoder.outputFormat, decoder.outputChannels, decoder.outputSampleRate, FAudioPlayer::on_send_frames_to_device);
	if (mal_device_init(NULL, mal_device_type_playback, NULL, &config, this, &device) != MAL_SUCCESS)
	{
		printf("Failed to open playback device.\n");
		mal_decoder_uninit(&decoder);
		return -3;
	}
}

int FAudioPlayer::Start()
{
	if (mal_device_start(&device) != MAL_SUCCESS)
	{
		printf("Failed to start playback device.\n");
		mal_device_uninit(&device);
		mal_decoder_uninit(&decoder);
		return -4;
	}
	bIsPlaying = true;
	return 0;
}

int FAudioPlayer::Stop()
{
	if (mal_device_stop(&device) != MAL_SUCCESS)
	{
		printf("Failed to start playback device.\n");
		mal_device_uninit(&device);
		mal_decoder_uninit(&decoder);
		return -4;
	}
	bIsPlaying = false;
	return 0;
}

void FAudioPlayer::Cleanup()
{
	mal_device_uninit(&device);
	mal_decoder_uninit(&decoder);
}

float FAudioPlayer::GetTime()
{
	return ((float)totalFrameCount) / (float)decoder.internalSampleRate;
}

bool FAudioPlayer::IsPlaying()
{
	return bIsPlaying;
}

mal_decoder FAudioPlayer::GetDecoder()
{
	return decoder;
}

void FAudioPlayer::AddFrameCount(mal_uint32 FrameCount)
{
	totalFrameCount += FrameCount;
}

mal_uint32 FAudioPlayer::GetCurrentFrame()
{
	return totalFrameCount;
}

void FAudioPlayer::Seek(mal_int32 framesToSeek)
{
	if (framesToSeek <= 0)
	{
		framesToSeek = 0;
	}
	else
	{
		totalFrameCount = framesToSeek;
	}
	mal_decoder_seek_to_frame(&decoder, framesToSeek);
}