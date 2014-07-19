#include "StdAfx.h"
#include "DataBuffer.h"


CDataBuffer::CDataBuffer(void)
{
    m_nBufCur = 0;
    m_nBufLength = 256;
    m_pBuffer = (unsigned char *)malloc(m_nBufLength);
}

CDataBuffer::~CDataBuffer(void)
{
    if (m_pBuffer)
    {
        free(m_pBuffer);
        m_pBuffer = NULL;
    }
    m_nBufCur = 0;
    m_nBufLength = 0;
}

void CDataBuffer::PutChar( DWORD dwVal )
{
    unsigned char pCur[2];
    pCur[0] = (unsigned char)(dwVal & 0xff);
    pCur[1] = '\0';
    PutBuffer(pCur, 1);
}

void CDataBuffer::PutUI16( DWORD dwVal )
{
    PutChar(dwVal >> 8);
    PutChar(dwVal);
}

void CDataBuffer::PutUI24( DWORD dwVal )
{
    PutChar(dwVal >> 16);
    PutUI16(dwVal);
}

void CDataBuffer::PutUI32( DWORD dwVal )
{
    PutChar(dwVal >> 24);
    PutUI24(dwVal);
}

void CDataBuffer::PutUI64( unsigned __int64 nVal )
{
    PutUI32((DWORD)(nVal >> 32));
    PutUI32((DWORD)(nVal & 0xffffffff));
}

unsigned __int64 CDataBuffer::dbl2int( double d )
{
    union X
    {
        double a;
        __int64 b;
    } x;
    x.a = d;
    return x.b;
}

void CDataBuffer::PutDouble( double dVal )
{
    PutUI64(dbl2int(dVal));
}

void CDataBuffer::PutBuffer( const unsigned char *pBuf, int nSize )
{
    assert(pBuf);

    int nTotal = m_nBufCur + nSize;
    if (nTotal >= m_nBufLength)
    {
        unsigned char *pNew = (unsigned char *)realloc(m_pBuffer, nTotal + 1);
        assert(pNew);
        if (pNew == NULL)
        {
            return;
        }

        m_pBuffer = pNew;
        m_nBufLength = nTotal + 1;
    }

    unsigned char *pCur = m_pBuffer + m_nBufCur;
    memcpy(pCur, pBuf, nSize);
    m_nBufCur += nSize;
}

void CDataBuffer::PutString( const char *sStr )
{
    assert(sStr);
    int nLen = strlen(sStr);
    PutUI16(nLen);
    PutBuffer((unsigned char *)sStr, nLen);
}

void CDataBuffer::PutString( const unsigned char *sStr )
{
    assert(sStr);
    int nLen = _mbslen(sStr);
    PutUI16(nLen);
    PutBuffer(sStr, nLen);
}

void CDataBuffer::PutLongString( const unsigned char *sStr )
{
    assert(sStr);
    int nLen = _mbslen(sStr);
    PutUI32(nLen);
    PutBuffer(sStr, nLen);
}

void CDataBuffer::RewriteBufUI24( __int64 pos, int val )
{
    unsigned char pBuf[4];
    pBuf[0] = (unsigned char)((val >> 16) & 0xff);
    pBuf[1] = (unsigned char)((val >> 8) & 0xff);
    pBuf[2] = (unsigned char)(val & 0xff);
    pBuf[3] = '\0';
    unsigned char *pCur = m_pBuffer + pos;
    memcpy(pCur, pBuf, 3 * sizeof(unsigned char));
}

void CDataBuffer::RewriteBufUI32( __int64 pos, int val )
{
    unsigned char pBuf[5];
    pBuf[0] = (unsigned char)((val >> 24) & 0xff);
    pBuf[1] = (unsigned char)((val >> 16) & 0xff);
    pBuf[2] = (unsigned char)((val >> 8) & 0xff);
    pBuf[3] = (unsigned char)(val & 0xff);
    pBuf[4] = '\0';
    unsigned char *pCur = m_pBuffer + pos;
    memcpy(pCur, pBuf, 4 * sizeof(unsigned char));
}
