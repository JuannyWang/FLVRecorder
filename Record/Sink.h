#pragma once

class IDataSink
{
public:
    virtual bool OnSample(int nPacketType, const unsigned char *pBuffer, int nBufLength, DWORD lSampleTime) = 0;
};
