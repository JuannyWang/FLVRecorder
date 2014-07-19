#pragma once

#include "Sink.h"

class CVideoEncoder
{
public:
    CVideoEncoder(void) {};
    virtual ~CVideoEncoder(void) {};
    virtual void SetOutDataSink(IDataSink *pDataSink) = 0;
    virtual bool Begin(IDataSink *pDataSink, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, DWORD dwStartTime) = 0;
    virtual bool End() = 0;
    virtual bool Encode(DWORD lSampleTime, unsigned char *pData, int nSize) = 0;
};
