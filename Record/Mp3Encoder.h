#pragma once

#include "AudioEncoder.h"
#include "FlvMp3Packet.h"

#define _BLADEDLL
#include "include/BladeMP3EncDLL.h"
#pragma comment(lib, "lib/lame_enc.lib")


class CMp3Encoder : public CAudioEncoder
{
public:
    CMp3Encoder(void);
    ~CMp3Encoder(void);

public:
    void SetOutDataSink(IDataSink *pDataSink);
    bool Begin(IDataSink *pDataSink, UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, DWORD dwStartTime);
    bool End();
    bool Encode(DWORD lSampleTime, BYTE *pBuffer, long lBufferSize);

protected:
    PBYTE m_pMP3Buffer;
    DWORD m_dwSamples;
    HBE_STREAM m_hbeStream;

    FILE *m_pFileOut;

    void WriteFrame(const unsigned char *pBuf, int nSize, DWORD lSampleTime);
    BYTE *memstr(const BYTE *str1, int nLen1, const BYTE *str2);

    BYTE *m_pMp3SampleHeader;
    unsigned char *m_pBuffer;
    int m_nBugLength;
    int m_nBufCur;
    DWORD m_lLastSampleTime;

    CFlvMp3Packet *m_pFlv;
    IDataSink *m_pOutDataSink;
};
