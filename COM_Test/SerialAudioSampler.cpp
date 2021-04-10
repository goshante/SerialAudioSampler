#include <stdexcept>
#include <thread>

#include "SerialAudioSampler.h"
#include "Logger.h"

SerialAudioSampler::SerialAudioSampler(const std::string& port, int baudRate, UINT SamplingRateCalculationDurSec)
	: _isSampling(false)
	, _stopFlag(false)
{
	if (_serial.openDevice(port.c_str(), baudRate) != SerialMgr::errCode::Success)
		throw std::runtime_error("Failed to open serial port");
	appLog(Info) << "Connected to " << port << " with baud rate " << baudRate;
	appLog(Info) << "Calculating sampling rate... " << "Measure time (sec): " << SamplingRateCalculationDurSec;
	auto freq = _calculateSamplingRate(SamplingRateCalculationDurSec);

	constexpr auto bps = sizeof(WaveSample16_t) * 8;
	constexpr auto channels = 1;

	_wave .reset(new WaveStream(bps, freq, channels));
}

SerialAudioSampler::~SerialAudioSampler()
{
	_serial.closeDevice();
	appLog(Debug) << "SerialAudioSampler destroyed.";
}

SamplingRate_t SerialAudioSampler::_calculateSamplingRate(UINT dur)
{
	SamplingRate_t frequency = 0;
	std::vector<SamplingRate_t> measurements;
	auto time = Utils::getTimeMs();
	auto last = time;
	constexpr int measureInterval = 100;
	SamplingRate_t samplingRateHz;

	while (measurements.size() < (dur * (1000 / measureInterval)))
	{
		time = Utils::getTimeMs();
		if ((time - last) >= measureInterval)
		{
			measurements.push_back(frequency);
			last = time;
			frequency = 0;
		}
		byte dummy[2];
		_serial.readBytes(dummy, sizeof(dummy), 0);
		frequency++;
	}

	auto rate = Utils::findMostFrequentlyElement<SamplingRate_t>(measurements);
	samplingRateHz = rate * (1000 / measureInterval);
	appLog(Info) << "Sampling rate: " << samplingRateHz << " Hz";
	return samplingRateHz;
}

void SerialAudioSampler::StartSamplingToFile(const std::string& fileName)
{
	if (_isSampling.load())
		throw std::runtime_error("Cannot do StartSamplingToFile(). Already working.");

	_isSampling = true;
	_stopFlag = false;

	_worker = std::thread(&SerialAudioSampler::_sampleToFile, this, fileName);
}

void SerialAudioSampler::StartSamplingToWaveStream(int msBuffer, UINT device)
{
	if (_isSampling.load())
		throw std::runtime_error("Cannot do StartSamplingToWaveStream(). Already working.");

	_isSampling = true;
	_stopFlag = false;

	_wave->Initialize(device, _wave->GetBPS(), _wave->GetSamplingRate(), _wave->GetChannels());
	_worker = std::thread(&SerialAudioSampler::_sampleToStream, this, msBuffer);
}

void SerialAudioSampler::_sampleToFile(std::string fileName)
{
	WaveBuffer_t buffer;
	while (_stopFlag.load() == false)
	{
		WaveSample16_t sample;
		_serial.readBytes(&sample, sizeof(sample), 0);
		sample = WaveSample16_t(sample * SAMPLE_REDUCE_FACTOR);
		buffer.append<WaveSample16_t>(sample);
	}

	buffer.makeWave(_wave->GetChannels(), _wave->GetSamplingRate(), _wave->GetBPS());
	buffer.saveToFile(fileName);
	_isSampling = false;
}

void SerialAudioSampler::_sampleToStream(int msBuffer)
{
	WaveBufferPtr buffer = WaveBufferPtr(new WaveBuffer_t);
	auto time = Utils::getTimeMs();
	auto lastTime = time;
	auto devices = Utils::getAudioDeviceList();
	auto devId = _wave->GetDevice();

	appLog(Info) << "Streaming to " << devices[devId] << " with sampling rate " << _wave->GetSamplingRate() <<  " Hz";

	while (_stopFlag.load() == false)
	{
		WaveSample16_t sample;
		_serial.readBytes(&sample, sizeof(sample), 0);
		sample = WaveSample16_t(sample * SAMPLE_REDUCE_FACTOR);
		buffer->append<WaveSample16_t>(sample);

		time = Utils::getTimeMs();
		if (time - lastTime >= msBuffer)
		{
			_wave->PushSegment(buffer);
			buffer.reset(new WaveBuffer_t);
			lastTime = time;
		}
	}

	_isSampling = false;
}

void SerialAudioSampler::Stop()
{
	_stopFlag = true;
}

void SerialAudioSampler::Sync()
{
	if (!_isSampling)
		return;

	_worker.join();
}