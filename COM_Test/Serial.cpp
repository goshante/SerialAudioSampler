#include "Serial.h"
#include <stdexcept>

using uint64_t = unsigned long long int;

SerialMgr::SerialMgr()
    : _hSerial(NULL)
    , _timeouts({0,0,0,0,0})
    , _baudRate(0)
{
    _currentStateRTS = true;
    _currentStateDTR = true;
}

SerialMgr::~SerialMgr()
{
    closeDevice();
}

SerialMgr::errCode SerialMgr::openDevice(const char* Device, const unsigned int Bauds)
{
    _hSerial = CreateFileA(Device, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (_hSerial == INVALID_HANDLE_VALUE) 
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return errCode::FileNotFound;
        return errCode::InvalidHandleValue;
    }
    DCB dcbSerialParams;
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(_hSerial, &dcbSerialParams)) 
        return errCode::GetCommStateFailed;

    switch (Bauds)
    {
        case 110:     dcbSerialParams.BaudRate = CBR_110; break;
        case 300:     dcbSerialParams.BaudRate = CBR_300; break;
        case 600:     dcbSerialParams.BaudRate = CBR_600; break;
        case 1200:     dcbSerialParams.BaudRate = CBR_1200; break;
        case 2400:     dcbSerialParams.BaudRate = CBR_2400; break;
        case 4800:     dcbSerialParams.BaudRate = CBR_4800; break;
        case 9600:     dcbSerialParams.BaudRate = CBR_9600; break;
        case 14400:    dcbSerialParams.BaudRate = CBR_14400; break;
        case 19200:    dcbSerialParams.BaudRate = CBR_19200; break;
        case 38400:    dcbSerialParams.BaudRate = CBR_38400; break;
        case 56000:    dcbSerialParams.BaudRate = CBR_56000; break;
        case 57600:    dcbSerialParams.BaudRate = CBR_57600; break;
        case 115200:   dcbSerialParams.BaudRate = CBR_115200; break;
        case 128000:   dcbSerialParams.BaudRate = CBR_128000; break;
        case 256000:   dcbSerialParams.BaudRate = CBR_256000; break;
        default: return errCode::InvalidBaudRate;
    }
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(_hSerial, &dcbSerialParams)) 
        return errCode::SetCommStateFailed;

    _timeouts.ReadIntervalTimeout = 0;
    _timeouts.ReadTotalTimeoutConstant = MAXDWORD;
    _timeouts.ReadTotalTimeoutMultiplier = 0;
    _timeouts.WriteTotalTimeoutConstant = MAXDWORD;
    _timeouts.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(_hSerial, &_timeouts)) 
        return errCode::SetCommTimeoutFailed;

    _baudRate = Bauds;
    return errCode::Success;
}

void SerialMgr::closeDevice()
{
    _baudRate = 0;
    CloseHandle(_hSerial);
}

bool SerialMgr::writeChar(const char Byte)
{
    DWORD dwBytesWritten;
    if (!WriteFile(_hSerial, &Byte, 1, &dwBytesWritten, NULL))
        return false;
    return true;
}

bool SerialMgr::writeByte(const byte Byte)
{
    DWORD dwBytesWritten;
    if (!WriteFile(_hSerial, &Byte, 1, &dwBytesWritten, NULL)) 
        return false;
    return true;
}

bool SerialMgr::writeString(const char* receivedString)
{
    DWORD dwBytesWritten;
    if (!WriteFile(_hSerial, receivedString, strlen(receivedString), &dwBytesWritten, NULL))
        return false;
    return true;
}

bool SerialMgr::writeBytes(const void* Buffer, const unsigned int NbBytes)
{
    DWORD dwBytesWritten;
    if (!WriteFile(_hSerial, Buffer, NbBytes, &dwBytesWritten, NULL))
        return false;
    return true;
}

SerialMgr::errCode SerialMgr::readChar(char* pByte, unsigned int timeOut_ms)
{
    DWORD dwBytesRead = 0;
    _timeouts.ReadTotalTimeoutConstant = timeOut_ms;

    if (!SetCommTimeouts(_hSerial, &_timeouts))
        return errCode::SetCommTimeoutFailed;

    if (!ReadFile(_hSerial, pByte, 1, &dwBytesRead, NULL))
        return errCode::ReadFileFailed;

    if (dwBytesRead == 0)
        return errCode::Failed;
    return errCode::Success;
}

SerialMgr::errCode SerialMgr::readByte(byte* pByte, unsigned int timeOut_ms)
{
    DWORD dwBytesRead = 0;
    _timeouts.ReadTotalTimeoutConstant = timeOut_ms;

    if (!SetCommTimeouts(_hSerial, &_timeouts))
        return errCode::SetCommTimeoutFailed;

    if (!ReadFile(_hSerial, pByte, 1, &dwBytesRead, NULL))
        return errCode::ReadFileFailed;

    if (dwBytesRead == 0)
        return errCode::Failed;
    return errCode::Success;
}

int SerialMgr::readStringNoTimeOut(char* receivedString, char finalChar, unsigned int maxNbBytes)
{
    unsigned int    NbBytes = 0;
    errCode          charRead;
    while (NbBytes < maxNbBytes)
    {
        charRead = readChar(&receivedString[NbBytes]);
        if (charRead == errCode::Success)
        {
            if (receivedString[NbBytes] == finalChar)
            {
                receivedString[++NbBytes] = 0;
                return NbBytes;
            }

            NbBytes++;
        }

        if (charRead > errCode::Success)
            throw std::runtime_error("Failed to read.");
    }

    return -1;
}

int SerialMgr::readString(char* receivedString, char finalChar, unsigned int maxNbBytes, unsigned int timeOut_ms)
{
    if (timeOut_ms == 0) return readStringNoTimeOut(receivedString, finalChar, maxNbBytes);

    unsigned int    nbBytes = 0;
    errCode         charRead;
    timeOut         timer;
    long int        timeOutParam;

    timer.initTimer();

    while (nbBytes < maxNbBytes)
    {
        timeOutParam = timeOut_ms - timer.elapsedTime_ms();

        if (timeOutParam > 0)
        {
            charRead = readChar(&receivedString[nbBytes], timeOutParam);
            if (charRead == errCode::Success)
            {
                if (receivedString[nbBytes] == finalChar)
                {
                    receivedString[++nbBytes] = 0;
                    return nbBytes;
                }
                nbBytes++;
            }

            if (charRead > errCode::Success)
                throw std::runtime_error("Failed to read.");
        }

        if (timer.elapsedTime_ms() > timeOut_ms)
        {
            receivedString[nbBytes] = 0;
            return 0;
        }
    }
    return -1;
}

int SerialMgr::readBytes(void* buffer, unsigned int maxNbBytes, unsigned int timeOut_ms)
{
    DWORD dwBytesRead = 0;
    _timeouts.ReadTotalTimeoutConstant = (DWORD)timeOut_ms;
    if (!SetCommTimeouts(_hSerial, &_timeouts)) return -1;
    if (!ReadFile(_hSerial, buffer, (DWORD)maxNbBytes, &dwBytesRead, NULL))  return -2;
    return dwBytesRead;
}

bool SerialMgr::flushReceiver()
{
    if (PurgeComm(_hSerial, PURGE_RXCLEAR)) return true; else return false;
}

int SerialMgr::available()
{
    DWORD commErrors;
    COMSTAT commStatus;
    ClearCommError(_hSerial, &commErrors, &commStatus);
    return commStatus.cbInQue;
}

bool SerialMgr::DTR(bool status)
{
    if (status)
        return this->setDTR();
    else
        return this->clearDTR();
}

bool SerialMgr::setDTR()
{
    _currentStateDTR = true;
    return EscapeCommFunction(_hSerial, SETDTR);
}

bool SerialMgr::clearDTR()
{
    _currentStateDTR = true;
    return EscapeCommFunction(_hSerial, CLRDTR);
}

bool SerialMgr::RTS(bool status)
{
    if (status)
        return this->setRTS();
    else
        return this->clearRTS();
}

bool SerialMgr::setRTS()
{
    _currentStateRTS = false;
    return EscapeCommFunction(_hSerial, SETRTS);
}

bool SerialMgr::clearRTS()
{
    _currentStateRTS = false;
    return EscapeCommFunction(_hSerial, CLRRTS);
}

bool SerialMgr::isCTS()
{
    DWORD modemStat;
    GetCommModemStatus(_hSerial, &modemStat);
    return modemStat & MS_CTS_ON;
}

bool SerialMgr::isDSR()
{
    DWORD modemStat;
    GetCommModemStatus(_hSerial, &modemStat);
    return modemStat & MS_DSR_ON;
}

bool SerialMgr::isDCD()
{
    DWORD modemStat;
    GetCommModemStatus(_hSerial, &modemStat);
    return modemStat & MS_RLSD_ON;
}

bool SerialMgr::isRI()
{
    DWORD modemStat;
    GetCommModemStatus(_hSerial, &modemStat);
    return modemStat & MS_RING_ON;
}

bool SerialMgr::isDTR()
{
    return _currentStateDTR;
}

bool SerialMgr::isRTS()
{
    return _currentStateRTS;
}

SerialMgr::timeOut::timeOut()
    : _prevTime({ 0,0 })
{}

int gettimeofday(timeval_t * tp, struct timezone * tzp)
{
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}

void SerialMgr::timeOut::initTimer()
{
    gettimeofday(&_prevTime, NULL);
}

UINT32 SerialMgr::timeOut::elapsedTime_ms()
{
    timeval_t CurrentTime;
    int sec, usec;
    gettimeofday(&CurrentTime, NULL);
    sec = CurrentTime.tv_sec - _prevTime.tv_sec;
    usec = CurrentTime.tv_usec - _prevTime.tv_usec;
    if (usec < 0)
    {
        usec = 1000000 - _prevTime.tv_usec + CurrentTime.tv_usec;
        sec--;
    }
    return sec * 1000 + usec / 1000;
}
