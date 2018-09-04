#pragma once

#include <stdio.h>

class FAudioPlayer
{
public:
	static mal_uint32 on_send_frames_to_device(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
	{
		FAudioPlayer* audioPlayer = (FAudioPlayer*)pDevice->pUserData;

		mal_decoder* pDecoder = &audioPlayer->GetDecoder();

		if (pDecoder == NULL)
		{
			return 0;
		}

		audioPlayer->AddFrameCount(frameCount);

		return (mal_uint32)mal_decoder_read(pDecoder, frameCount, pSamples);
	}

	int Initialize(const char* name);
	int Start();
	int Stop();
	void Cleanup();
	float GetTime();
	bool IsPlaying();
	mal_decoder GetDecoder();
	void AddFrameCount(mal_uint32 FrameCount);
	mal_uint32 GetCurrentFrame();
	void Seek(mal_int32 framesToSeek);

private:
	bool bIsPlaying = false;
	mal_decoder decoder;
	mal_device device;
	mal_uint32 totalFrameCount = 0;
};