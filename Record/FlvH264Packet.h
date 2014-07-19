#pragma once

#include "Flv.h"


class CFlvH264Packet : public CFlv
{
public:
    CFlvH264Packet(void);
    ~CFlvH264Packet(void);

public:
    bool WriteHeader(IDataSink *pDataSink, UINT nWidth, UINT nHeight, UINT nVideoCodecID, DWORD dwStartTime);
    bool WriteTailer();

protected:
    void Init();

    int m_nVideoCodecID;

public:
    bool BeginData();
    bool WriteData(const unsigned char *pBuf, int nSize, DWORD lSampleTime);
    bool EndData(DWORD lSampleTime);

private:
    bool m_bSPS, m_bPPS;
    int m_nPSSize;
    __int64 m_nPSOffset;
    unsigned char *m_pSEI;
    int m_nSEISize;
};
