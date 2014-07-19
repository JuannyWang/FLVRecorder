#include "StdAfx.h"
#include "FlvMp3Packet.h"
#include <assert.h>
#include <math.h>


CFlvMp3Packet::CFlvMp3Packet(void)
{
    m_iAudioCodecID = -1;
}

CFlvMp3Packet::~CFlvMp3Packet(void)
{
}

void CFlvMp3Packet::Init()
{
    __super::Init();

    m_iAudioCodecID = -1;
}

bool CFlvMp3Packet::WriteHeader(IDataSink *pDataSink, UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, int iAudioCodecID, DWORD dwStartTime)
{
    if (m_pDataBuffer == NULL)
    {
        return false;
    }

    Init();

    m_pDataSink = pDataSink;

    m_iAudioCodecID = iAudioCodecID;
    bool bStereo = (uiNumChannels > 1);

    int iSampleRateIdx = GetSampleRateIdx(uiSampleRate);
    int iSoundSizeIdx = (uiBitsPerSample == 16 ? 1 : 0);
    m_cMp3Tag = (m_iAudioCodecID << 4) | (iSampleRateIdx << 2) | (iSoundSizeIdx << 1) | (bStereo ? 1 : 0);

#ifdef TEST_MP3_FLV_FILE
    m_pFlvFile = fopen(TEST_MP3_FLV_FILE, "wb");
#endif

    memset(&m_FrameContext, 0, sizeof(m_FrameContext));
    if (m_pFlvFile)
    {
        int metdata_size_pos, data_size;
        m_pDataBuffer->PutChar('F');
        m_pDataBuffer->PutChar('L');
        m_pDataBuffer->PutChar('V');

        m_pDataBuffer->PutChar(0x01);
        m_pDataBuffer->PutChar(0x04);
        m_pDataBuffer->PutUI32(0x09);
        m_pDataBuffer->PutUI32(0x00);

        // meta tag
        m_pDataBuffer->PutChar(0x12);
        metdata_size_pos = m_pDataBuffer->m_nBufCur;
        m_pDataBuffer->PutUI24(0);
        m_pDataBuffer->PutUI24(0);
        m_pDataBuffer->PutUI32(0);

        m_pDataBuffer->PutChar(0x02);
        m_pDataBuffer->PutString("@setDataFrame");

        m_pDataBuffer->PutChar(0x02);
        m_pDataBuffer->PutString("onMetaData");

        m_pDataBuffer->PutChar(0x03);
        m_pDataBuffer->PutString("duration");
        m_pDataBuffer->PutChar(0x00);
        m_FlvContext.duration_offset = m_pDataBuffer->m_nBufCur;
        m_pDataBuffer->PutDouble(0);

        m_pDataBuffer->PutString("filesize");
        m_pDataBuffer->PutChar(0x00);
        m_FlvContext.filesize_offset = m_pDataBuffer->m_nBufCur;
        m_pDataBuffer->PutDouble(0);

        m_pDataBuffer->PutString("videodatarate");
        m_pDataBuffer->PutChar(0x00);
        m_FlvContext.bitrate_offset = m_pDataBuffer->m_nBufCur;
        m_pDataBuffer->PutDouble(0);

        m_pDataBuffer->PutString("framerate");
        m_pDataBuffer->PutChar(0x00);
        m_FlvContext.framerate_offset = m_pDataBuffer->m_nBufCur;
        m_pDataBuffer->PutDouble(0);

        m_pDataBuffer->PutString("audiosamplerate");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(uiSampleRate);

        m_pDataBuffer->PutString("audiosamplesize");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(uiBitsPerSample);

        m_pDataBuffer->PutString("stereo");
        m_pDataBuffer->PutChar(0x01);
        m_pDataBuffer->PutChar(bStereo ? 1 : 0);

        m_pDataBuffer->PutString("audiocodecid");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(iAudioCodecID);

        m_pDataBuffer->PutString("");
        m_pDataBuffer->PutChar(0x09);

        data_size = m_pDataBuffer->m_nBufCur - metdata_size_pos - 10;
        m_pDataBuffer->PutUI32(data_size + 11);
        m_pDataBuffer->RewriteBufUI24(metdata_size_pos, data_size);

        FlushBuf(0x12, GetTickCount());
        m_FrameContext.m_nFrame++;
    }

    m_FlvContext.start = dwStartTime;
    m_FlvContext.finish = m_FlvContext.start;
    m_FlvContext.current = m_FlvContext.start;

    return true;
}

bool CFlvMp3Packet::WriteTailer()
{
    if (m_pFlvFile != NULL)
    {
        fclose(m_pFlvFile);
        m_pFlvFile = NULL;
    }

    return true;
}

bool CFlvMp3Packet::WriteData(const unsigned char *pBuf, int nSize, DWORD lSampleTime)
{
    if (m_pDataBuffer == NULL)
    {
        return false;
    }

    assert(pBuf);
    assert(nSize > 0);

    if (m_FlvContext.start < 0)
    {
        m_FlvContext.start = lSampleTime;
    }

    if (!m_FrameContext.m_bWriteFrame)
    {
        m_FlvContext.current = lSampleTime;
        long nTimeStamp = m_FlvContext.current - m_FlvContext.start;
        if (nTimeStamp < 0)
        {
            nTimeStamp = 0;
        }
        m_FrameContext.m_nFrameSizeOffset = m_pDataBuffer->m_nBufCur + 1;
        assert(nTimeStamp >= 0);
        m_pDataBuffer->PutChar(0x08);
        m_pDataBuffer->PutUI24(0);
        m_pDataBuffer->PutUI24(nTimeStamp & 0x00ffffff);
        m_pDataBuffer->PutChar(nTimeStamp >> 24);
        m_pDataBuffer->PutUI24(0x00);

        m_pDataBuffer->PutChar(m_cMp3Tag);
        m_FrameContext.m_nFrameSize = 1;
        m_FrameContext.m_bWriteFrame = true;
    }
    else
    {

    }

    m_pDataBuffer->PutBuffer(pBuf, nSize);
    m_FrameContext.m_nFrameSize += nSize;

    return true;
}

bool CFlvMp3Packet::BeginData()
{
    return true;
}

bool CFlvMp3Packet::EndData(DWORD lSampleTime)
{
    if (m_pDataBuffer == NULL)
    {
        return false;
    }

    m_pDataBuffer->PutUI32(m_FrameContext.m_nFrameSize + 11);
    m_pDataBuffer->RewriteBufUI24(m_FrameContext.m_nFrameSizeOffset, m_FrameContext.m_nFrameSize);

    m_FlvContext.current = lSampleTime;
    long nTimeStamp = m_FlvContext.current - m_FlvContext.start;
    if (nTimeStamp < 0)
    {
        nTimeStamp = 0;
    }
    FlushBuf(0x08, nTimeStamp);

    m_FrameContext.m_bWriteFrame = false;
    m_FrameContext.m_nFrameSize = 0;
    m_FrameContext.m_nFrameSizeOffset = 0;

    m_FlvContext.finish = GetTickCount();
    m_FlvContext.duration = (m_FlvContext.finish - m_FlvContext.start) / CLOCKS_PER_SEC;

    return true;
}

int CFlvMp3Packet::GetSampleRateIdx(UINT uiSampleRate)
{
    int iIdx = 0;
    if (uiSampleRate >= 44100)
    {
        iIdx = 3;
    }
    else if (uiSampleRate >= 22050)
    {
        iIdx = 2;
    }
    else if (uiSampleRate >= 11025)
    {
        iIdx = 1;
    }
    else if (uiSampleRate >= 8000)
    {
        iIdx = 0;
    }

    return iIdx;
}