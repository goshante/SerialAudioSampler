#include <iostream>

#include "ConfigMgr.h"
#include "SerialAudioSampler.h"
#include "Utils.h"

#define _LOGGER_MAIN_CPP
#include "Logger.h"

#pragma comment(lib, "Winmm.lib")

constexpr auto CONFIG_FILE_NAME = "config.cfg";

struct ConfigValues
{
	std::string			SerialPort;
	int					BaudRate;

	UINT				Device;
	int					SampleCalcDurationSec;
	int					StreamBufferMs;
	std::string			FileName;
};

void ConfigResetDefaults(CConfigMgr& cmgr)
{
	cmgr.SetValue_Str("SerialPort",		"Name",						"COM1");
	cmgr.SetValue_Num("SerialPort",		"BaudRate",					115200);

	cmgr.SetValue_Num("Audio",			"Device",					0);
	cmgr.SetValue_Num("Audio",			"SampleCalcDurationSec",	5);
	cmgr.SetValue_Num("Audio",			"StreamBufferMs",			50);
	cmgr.SetValue_Str("Audio",			"FileName",					"result.wav");

	cmgr.Save();
	cmgr.SaveAs(CONFIG_FILE_NAME);
}

ConfigValues GetSettings(CConfigMgr& cmgr)
{
	ConfigValues cvals;
	cvals.SerialPort = cmgr.GetValue_Str("SerialPort", "Name", "COM1");
	cvals.BaudRate = cmgr.GetValue_Num("SerialPort", "BaudRate", 115200);

	cvals.Device = cmgr.GetValue_Num("Audio", "Device", 0);
	cvals.SampleCalcDurationSec = cmgr.GetValue_Num("Audio", "SampleCalcDurationSec", 5);
	cvals.StreamBufferMs = cmgr.GetValue_Num("Audio", "StreamBufferMs", 50);
	cvals.FileName = cmgr.GetValue_Str("Audio", "FileName", "result.wav");

	cmgr.Save();
	cmgr.SaveAs(CONFIG_FILE_NAME);	// This will fix broken config file if it's broken

	return cvals;
}

int main()
{
	APP_LOG_LEVEL(LOGLVL(Debug));

	CConfigMgr cmgr;
	if (!Utils::fileExists(CONFIG_FILE_NAME))
		ConfigResetDefaults(cmgr);
	else
		Utils::RemoveBOMFromFile(CONFIG_FILE_NAME);
	cmgr.Load(CONFIG_FILE_NAME);
	auto settings = GetSettings(cmgr);
	appLog(Info) << "Config loaded.";

	try
	{
		int mode = -1;
		std::cout << "Enter 0 for recording to file or 1 for audio stream: ";
		while (mode != 0 && mode != 1)
			std::cin >> mode;
		appLog(Debug) << "Mode " << mode;

		SerialAudioSampler sampler(settings.SerialPort, settings.BaudRate, settings.SampleCalcDurationSec, settings.Device);

		if (mode == 0)
			sampler.StartSamplingToFile(settings.FileName);
		else
			sampler.StartSamplingToWaveStream(settings.StreamBufferMs);

		std::cout << "Press F12 to stop..." << std::endl;
		while (!(GetKeyState(VK_F12) & 0x8000))
			Sleep(500);
		
		sampler.Stop();
		sampler.Sync();
	}
	catch (const std::exception& ex)
	{
		appLog(Critical) << "Error occured: " << ex.what();
		return -1;
	}

	return 0;
}


