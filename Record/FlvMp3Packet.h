#pragma once

#include "Flv.h"


class CFlvMp3Packet : public CFlv
{
public:
    CFlvMp3Packet(void);
    ~CFlvMp3Packet(void);

public:
    bool WriteHeader(IDataSink *pDataSink, UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, int iAudioCodecID, DWORD dwStartTime);
    bool WriteTailer();

protected:
    void Init();

    int m_iAudioCodecID;

public:
    bool BeginData();
    bool WriteData(const unsigned char *pBuf, int nSize, DWORD lSampleTime);
    bool EndData(DWORD lSampleTime);

private:
    int GetSampleRateIdx(UINT uiSampleRate);

    unsigned char m_cMp3Tag;
};
