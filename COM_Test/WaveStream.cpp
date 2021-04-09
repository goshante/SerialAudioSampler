#include "WaveStream.h"
#include "Logger.h"

void WaveBuffer_t::append(const void* data, size_t size)
{
	const byte* bData = reinterpret_cast<const byte*>(data);
	for (size_t i = 0; i < size; i++)
		push_back(bData[i]);
}

void WaveBuffer_t::makeWaveFile(WORD channels, SamplingRate_t samplingRate)
{
	WaveBuffer_t wave;
	wave.append((const byte*)"RIFF", 4);
	wave.append<int>(36 + (size()));
	wave.append((const byte*)"WAVEfmt ", 8);
	wave.append<int>(16);
	wave.append<WORD>(WAVE_FORMAT_PCM);
	wave.append<WORD>(channels);
	wave.append<SamplingRate_t>(samplingRate);
	wave.append<int>(samplingRate * channels * sizeof(WaveSample16_t));
	wave.append<WORD>(channels * sizeof(WaveSample16_t));
	wave.append<WORD>(sizeof(WaveSample16_t) * 8);
	wave.append((const byte*)"data", 4);
	wave.append<size_t>(size());
	wave.append(&at(0), size());
	clear();
	*this = wave;
}

bool WaveBuffer_t::saveToFile(const std::string& path)
{
	HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == HANDLE(0xFFFFFFFF))
		return false;
	DWORD dwWritten = 0;
	WriteFile(hFile, &at(0), size(), &dwWritten, NULL);
	CloseHandle(hFile);
	return true;
}

void CALLBACK callback(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR inst, DWORD_PTR param1, DWORD_PTR param2)
{
	StreamQueue* queue = (StreamQueue*)inst;

	if (uMsg == WOM_DONE)
		queue->pop();
}

WaveStream::WaveStream()
	: _hWaveOut(NULL)
	, _wfx({ 0,0,0,0,0,0,0 })
	, _device(0)
{
}

WaveStream::WaveStream(UINT device, WORD bps, SamplingRate_t samplingRate, int channels)
	: _hWaveOut(NULL)
	, _wfx({0,0,0,0,0,0,0})
	, _device(0)
{
	if (!Initialize(device, bps, samplingRate, channels))
		throw std::runtime_error("Failed to initialize WaveStream");
}

bool WaveStream::Initialize(UINT device, WORD bps, SamplingRate_t samplingRate, int channels)
{
	if (_hWaveOut)
	{
		if (waveOutClose(_hWaveOut) != MMSYSERR_NOERROR)
			return false;
	}

	_device = device;
	_wfx.wFormatTag = WAVE_FORMAT_PCM;
	_wfx.wBitsPerSample = bps;
	_wfx.nChannels = channels;
	_wfx.nSamplesPerSec = samplingRate;
	_wfx.nBlockAlign = (_wfx.wBitsPerSample * _wfx.nChannels) / 8;
	_wfx.nAvgBytesPerSec = _wfx.nSamplesPerSec * _wfx.nBlockAlign;
	_wfx.cbSize = 0;

	if (waveOutOpen(&_hWaveOut, device, &_wfx, DWORD(&callback), DWORD_PTR(&_queue), CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		return false;

	appLog(Info) << "WaveStream initialized.";
	return true;
}

WaveStream::~WaveStream()
{
	waveOutClose(_hWaveOut);
	appLog(Info) << "WaveStream destroyed. ";
}


void WaveStream::PushSegment(WaveBufferPtr buffer)
{
	std::shared_ptr<WAVEHDR> header(new WAVEHDR);
	*header = { LPSTR(&(buffer->operator[](0))), buffer->size(), 0, 0, 0, 0, 0, 0 };
	WaveQueueUnit sound = { buffer, header };
	_queue.push(sound);
	waveOutPrepareHeader(_hWaveOut, header.get(), sizeof(WAVEHDR));
	waveOutWrite(_hWaveOut, header.get(), sizeof(WAVEHDR));
	waveOutUnprepareHeader(_hWaveOut, header.get(), sizeof(WAVEHDR));
}

SamplingRate_t WaveStream::GetSamplingRate() const
{
	return _wfx.nSamplesPerSec;
}

int WaveStream::GetChannels() const
{
	return _wfx.nChannels;
}

WORD WaveStream::GetBPS() const
{
	return _wfx.wBitsPerSample;
}

UINT WaveStream::GetDevice() const
{
	return _device;
}