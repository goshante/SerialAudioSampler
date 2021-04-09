#pragma once

#include <queue>

#include <Windows.h>
#include <mmeapi.h>
#include <mmsystem.h>

#include "Utils.h"


class WaveBuffer_t;
struct WaveQueueUnit
{
	std::shared_ptr<WaveBuffer_t> buffer;
	std::shared_ptr<WAVEHDR>	  header;
};

using WaveSample16_t = unsigned short;
using SamplingRate_t = DWORD;
using StreamQueue = std::queue<WaveQueueUnit>;
using WaveBufferPtr = std::shared_ptr<WaveBuffer_t>;

class WaveBuffer_t : public std::vector<byte>
{
public:
	void append(const void* data, size_t size);

	template <typename T>
	void append(T data)
	{
		const byte* bData = reinterpret_cast<const byte*>(&data);
		size_t size = sizeof(T);
		for (size_t i = 0; i < size; i++)
			push_back(bData[i]);
	}

	void makeWaveFile(WORD channels, SamplingRate_t samplingRate);
	bool saveToFile(const std::string& path);
};

class WaveStream
{
private:
	HWAVEOUT		_hWaveOut;
	StreamQueue	    _queue;
	WAVEFORMATEX	_wfx;
	UINT			_device;

public:
	WaveStream();
	WaveStream(UINT device, WORD bps, SamplingRate_t samplingRate, int channels);
	WaveStream(const WaveStream&) = delete;
	~WaveStream();

	bool Initialize(UINT device, WORD bps, SamplingRate_t samplingRate, int channels);
	void PushSegment(WaveBufferPtr buffer); //Add to queue and play

	SamplingRate_t GetSamplingRate() const;
	int GetChannels() const;
	WORD GetBPS() const;
	UINT GetDevice() const;
};