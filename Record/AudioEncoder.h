#pragma once

#include "Sink.h"

class CAudioEncoder
{
public:
    CAudioEncoder(void) {};
    virtual ~CAudioEncoder(void) {};
    virtual void SetOutDataSink(IDataSink *pDataSink) = 0;
    virtual bool Begin(IDataSink *pDataSink, UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, DWORD dwStartTime) = 0;
    virtual bool End() = 0;
    virtual bool Encode(DWORD lSampleTime, BYTE *pBuffer, long lBufferSize) = 0;
};
