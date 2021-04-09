#pragma once

#include <iostream>
#include <exception>
#include <time.h>
#include <sstream>
#include <memory>
#include "Logger.h"
#include "Utils.h"

namespace _____LOGGER
{

	OutputInterface::NewLine OutputInterface::nl;

	OutputInterface::OutputInterface()
		: _type(Type::None)
		, _file(nullptr)
		, _strPtr(nullptr)
		, _nl("\r\n")
		, _mode(FileMode::CreateAlways)
		, _directWrite(true)
	{
	}

	OutputInterface::OutputInterface(Type type, NewLineType nlType, std::string& fileNameOrString, FileMode mode)
		: _type(type)
		, _file(nullptr)
		, _strPtr(nullptr)
		, _mode(mode)
		, _directWrite(true)
	{
		UINT umode = 0;
		switch (mode)
		{
		case FileMode::CreateAlways:
			umode = CREATE_ALWAYS;
			break;
		case FileMode::CreateIfNotExist:
		case FileMode::OpenExisting:
			umode = OPEN_EXISTING;
			break;
		}

		HANDLE hFile = NULL;
		switch (type)
		{
		case Type::String:
			_strPtr = &fileNameOrString;
			break;

		case Type::FileAndConsole:
		case Type::File:
			if (mode == FileMode::OpenExisting && !Utils::fileExists(fileNameOrString))
				throw std::runtime_error("OutputInterface::OutputInterface: File does not exists");
			if (mode == FileMode::CreateIfNotExist)
			{
				if (Utils::fileExists(fileNameOrString))
					umode = OPEN_EXISTING;
				else
					umode = CREATE_ALWAYS;
			}
			hFile = CreateFileA(fileNameOrString.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
				umode, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
			if (hFile == 0 || hFile == HANDLE(~0))
				throw std::runtime_error("OutputInterface::OutputInterface: Cannot open file " + fileNameOrString + " for writing.");
			_file = std::shared_ptr<WinHandle>(new WinHandle(hFile));
			break;
		}

		switch (nlType)
		{
		case NewLineType::CR:
			_nl = "\r";
			break;
		case NewLineType::LF:
			_nl = "\n";
			break;

		case NewLineType::CRLF:
			_nl = "\r\n";
			break;
		}
	}

	OutputInterface::OutputInterface(Type type, NewLineType nlType, FileMode mode)
		: _type(type)
		, _file(nullptr)
		, _strPtr(nullptr)
		, _mode(mode)
		, _directWrite(true)
	{
		switch (nlType)
		{
		case NewLineType::CR:
			_nl = "\r";
			break;
		case NewLineType::LF:
			_nl = "\n";
			break;

		case NewLineType::CRLF:
			_nl = "\r\n";
			break;
		}
	}

	void OutputInterface::_toOutput(const std::string& str)
	{
		DWORD dw = 0;

		switch (_type)
		{
		case Type::None:
			break;

		case Type::Console:
			std::cout << str;
			break;

		case Type::String:
			(*_strPtr) += str;
			break;

		case Type::File:
			WriteFile(*_file, &str[0], DWORD(str.length()), &dw, NULL);
			break;

		case Type::FileAndConsole:
			std::cout << str;
			WriteFile(*_file, &str[0], DWORD(str.length()), &dw, NULL);
			break;
		}
	}

	OutputInterface& OutputInterface::operator<<(const std::string& str)
	{
		_toOutput(str);
		return *this;
	}

	OutputInterface& OutputInterface::operator<<(const NewLine&)
	{
		_toOutput(_nl);
		return *this;
	}

	bool OutputInterface::IsReady() const
	{
		return (_type != Type::None);
	}

	void OutputInterface::SetOutputFile(const std::string& fname)
	{
		Close();

		UINT umode = 0;
		switch (_mode)
		{
		case FileMode::CreateAlways:
			umode = CREATE_ALWAYS;
			break;
		case FileMode::CreateIfNotExist:
		case FileMode::OpenExisting:
			umode = OPEN_EXISTING;
			break;
		}

		if (_mode == FileMode::OpenExisting && !Utils::fileExists(fname.c_str()))
			throw std::runtime_error("OutputInterface::OutputInterface: File does not exists");
		if (_mode == FileMode::CreateIfNotExist)
		{
			if (Utils::fileExists(fname.c_str()))
				umode = OPEN_EXISTING;
			else
				umode = CREATE_ALWAYS;
		}

		UINT ff = FILE_ATTRIBUTE_NORMAL;
		if (_directWrite)
			ff |= FILE_FLAG_WRITE_THROUGH;

		HANDLE hFile = CreateFileA(fname.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
			umode, ff, NULL);
		if (hFile == 0 || hFile == HANDLE(~0))
			throw std::runtime_error("OutputInterface::OutputInterface: Cannot open file " + fname + " for writing.");
		_file = std::shared_ptr<WinHandle>(new WinHandle(hFile));
	}

	void OutputInterface::SetDirectWrite(bool enable)
	{
		_directWrite = enable;
	}

	std::string OutputInterface::genNl() const
	{
		return _nl;
	}

	void OutputInterface::Close()
	{
		_file.reset();
	}

	std::recursive_mutex Logger::Writer::_writeMutex;

	std::string _generateTimeStamp()
	{
		char date[64];
		time_t t = time(0);
		struct tm tm;

		gmtime_s(&tm, &t);
		strftime(date, sizeof(date), "[%d.%m.%Y|%H:%M:%S]", &tm);
		return date;
	}

	/****************
	 *	  LOGGER	*
	 ****************/

	Logger::Logger(OutputInterface::Type outType, std::string fileName) :
		OutputInterface
		(outType
			, NewLineType::CRLF
			, fileName
		)
		, _level(Level::Info)
		, _minLevel(Level::Debug)
		, _file("None")
		, _line(0)
		, _func("?")
		, _buffer("")
		, _logFileName(fileName)
		, _doNotAppendSpaces(false)
	{
	}

	Logger::~Logger()
	{
	}

	std::string Logger::_levelToString()
	{
		switch (_level)
		{
		case Level::Debug:
			return "Debug";

		case Level::Info:
			return "Info";

		case Level::Warning:
			return "Warning";

		case Level::Critical:
			return "Critical";

		case Level::Fatal:
			return "Fatal";

		default:
			return "Unknown";
		}
	}

	void Logger::_toOutput(const std::string& str)
	{
		_buffer += str;
		if (!_doNotAppendSpaces)
			_buffer += " ";
	}

	void Logger::_write()
	{
		if (_buffer != "")
		{
			if (_level >= _minLevel)
			{
				std::ostringstream ss;
				ss << _generateTimeStamp() << "[" << _file << ":" << _line << " (" << _func << ")] [" << _levelToString() << "]:\t" << _buffer << _nl;
				OutputInterface::_toOutput(ss.str());
			}
			_buffer = "";
		}
	}

	void Logger::level(Level lvl)
	{
		_minLevel = lvl;
	}

	/****************
	 *	  WRITER	*
	 ****************/

	Logger::Writer::Writer(std::string file, const std::string& func, int line, Level level, Logger& logger) : _logger(&logger)
	{
		_writeMutex.lock();

		if (level == Logger::Level::Clear)
		{
			logger.SetOutputFile(__logFileName);
			return;
		}

		if (file.find("/") != std::string::npos)
			logger._file = file.substr(file.find_last_of("/") + 1, file.length() - 1);
		else if (file.find("\\") != std::string::npos)
			logger._file = file.substr(file.find_last_of("\\") + 1, file.length() - 1);
		else
			logger._file = file;
		logger._line = line;
		logger._func = func;
		logger._level = level;
	}

	Logger::Writer::~Writer()
	{
		_logger->_write();
		_writeMutex.unlock();
	}

	Logger::Writer& Logger::Writer::operator<<(const std::string& str)
	{
		_logger->_toOutput(str);
		return *this;
	}

	Logger::Writer& Logger::Writer::operator<<(const NewLine&)
	{
		_logger->_toOutput(_logger->_nl);
		return *this;
	}

	Logger::Writer& Logger::Writer::operator<<(const std::vector<std::string>& strings)
	{
		_logger->_doNotAppendSpaces = true;
		_logger->_toOutput("{ ");
		for (size_t i = 0; i < strings.size(); i++)
		{
			_logger->_toOutput("\"" + strings[i] + "\"");
			if (i != strings.size() - 1)
				_logger->_toOutput(", ");
		}
		_logger->_toOutput(" }");
		_logger->_doNotAppendSpaces = false;
		return *this;
	}

}