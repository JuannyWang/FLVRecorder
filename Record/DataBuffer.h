#pragma once


class CDataBuffer
{
public:
    CDataBuffer(void);
    ~CDataBuffer(void);

public:
    void PutChar(DWORD dwVal);
    void PutUI16(DWORD dwVal);
    void PutUI24(DWORD dwVal);
    void PutUI32(DWORD dwVal);
    void PutUI64(unsigned __int64 nVal);
    unsigned __int64 dbl2int(double d);
    void PutDouble(double dVal);
    void PutBuffer(const unsigned char *pBuf, int nSize);
    void PutString(const char *sStr);
    void PutString(const unsigned char *sStr);
    void PutLongString(const unsigned char *sStr);
    void RewriteBufUI24(__int64 pos, int val);
    void RewriteBufUI32(__int64 pos, int val);

    unsigned char *m_pBuffer;
    int m_nBufLength;
    int m_nBufCur;
};
