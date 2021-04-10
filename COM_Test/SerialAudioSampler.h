#pragma once
#include <string>
#include <atomic>
#include <thread>
#include "Serial.h"
#include "Utils.h"
#include "WaveStream.h"


class SerialAudioSampler
{
private:
	SerialMgr						_serial;
	std::unique_ptr<WaveStream>		_wave;
	std::atomic<bool>				_isSampling;
	std::atomic<bool>				_stopFlag;
	std::thread						_worker;

	SamplingRate_t _calculateSamplingRate(UINT dur);
	void _sampleToFile(std::string fileName);
	void _sampleToStream(int msBuffer);

	static constexpr float SAMPLE_REDUCE_FACTOR = 0.33f;

public:
	SerialAudioSampler(const std::string& port, int baudRate, UINT SamplingRateCalculationDurSec);
	SerialAudioSampler(const SerialAudioSampler&) = delete;
	~SerialAudioSampler();

	void StartSamplingToFile(const std::string& fileName);
	void StartSamplingToWaveStream(int msBuffer, UINT device);

	void Stop();
	void Sync();
};