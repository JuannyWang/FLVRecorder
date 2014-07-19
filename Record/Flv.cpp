#include "StdAfx.h"
#include "Flv.h"
#include <assert.h>
#include <math.h>


CFlv::CFlv(void) :
    m_pFlvFile(NULL)
{
    m_pDataBuffer = NULL;
    m_pDataBuffer = new CDataBuffer;
}

CFlv::~CFlv(void)
{
    if (m_pDataBuffer)
    {
        delete m_pDataBuffer;
        m_pDataBuffer = NULL;
    }
}

void CFlv::Init()
{
    assert(m_pFlvFile == NULL);

    m_pFlvFile = NULL;
    memset(&m_FrameContext, 0, sizeof(FrameContext));
    memset(&m_FlvContext, 0, sizeof(FlvContext));
    m_FrameContext.m_nFrame = -1;
}

bool CFlv::FlushBuf(int nPacketType, DWORD lSampleTime)
{
    if (m_pDataBuffer == NULL)
    {
        return false;
    }

    if (m_pDataBuffer->m_pBuffer == NULL || m_pDataBuffer->m_nBufCur <= 0)
    {
        return false;
    }

    bool bRet = true;
    int nTimeStamp = m_FlvContext.current - m_FlvContext.start;

    if (m_pFlvFile)
    {
        int len = fwrite(m_pDataBuffer->m_pBuffer, sizeof(char), m_pDataBuffer->m_nBufCur, m_pFlvFile);
    }

    if (m_pDataSink)
    {
        bRet = m_pDataSink->OnSample(nPacketType, m_pDataBuffer->m_pBuffer, m_pDataBuffer->m_nBufCur, lSampleTime);
    }

    m_pDataBuffer->m_nBufCur = 0;

    return bRet;
}

void CFlv::RewriteFileDouble(__int64 pos, double val)
{
    if (m_pFlvFile)
    {
        m_pDataBuffer->PutDouble(val);
        fseek(m_pFlvFile, pos, SEEK_SET);
        FlushBuf(0, 0);
        fseek(m_pFlvFile, 0, SEEK_END);
    }
    else
    {

    }
}