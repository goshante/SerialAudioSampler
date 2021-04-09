#pragma once

#include <windows.h>

struct timeval_t
{
    long tv_sec;
    long tv_usec;
};

class SerialMgr
{
public:
    enum class errCode
    {
        Success,
        Failed,
        InvalidHandleValue,
        FileNotFound,
        GetCommStateFailed,
        InvalidBaudRate,
        SetCommStateFailed,
        SetCommTimeoutFailed,
        ReadFileFailed
    };

    class timeOut
    {
        timeval_t   _prevTime;

    public:
        timeOut();
        void initTimer();
        UINT32 elapsedTime_ms();
    };

private:
    bool            _currentStateRTS;
    bool            _currentStateDTR;
    HANDLE          _hSerial;
    COMMTIMEOUTS    _timeouts;
    unsigned int    _baudRate;

    int readStringNoTimeOut(char* String, char FinalChar, unsigned int MaxNbBytes);

public:
    SerialMgr();
    ~SerialMgr();

    errCode openDevice(const char* Device, const unsigned int Bauds);
    void closeDevice();

    int getCurrentBaudRate();

    errCode readChar(char* pByte, const unsigned int timeOut_ms = 0);
    errCode readByte(byte* pByte, const unsigned int timeOut_ms = 0);
    int readString(char* receivedString, char finalChar, unsigned int maxNbBytes, const unsigned int timeOut_ms = 0);
    int readBytes(void* buffer, unsigned int maxNbBytes, const unsigned int timeOut_ms = 0);

    bool writeChar(char c);
    bool writeByte(byte b);
    bool writeString(const char* String);
    bool writeBytes(const void* Buffer, const unsigned int NbBytes);

    bool flushReceiver();
    int available();

    bool DTR(bool status);
    bool setDTR();
    bool clearDTR();

    bool RTS(bool status);
    bool setRTS();
    bool clearRTS();


    bool isRI();
    bool isDCD();
    bool isCTS();
    bool isDSR();
    bool isRTS();
    bool isDTR();
};