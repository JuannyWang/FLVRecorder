#include "StdAfx.h"
#include "FlvH264Packet.h"
#include <assert.h>
#include <math.h>


CFlvH264Packet::CFlvH264Packet(void) :
    m_pSEI(NULL),
    m_nSEISize(0)
{
    m_nVideoCodecID = -1;
}

CFlvH264Packet::~CFlvH264Packet(void)
{
}

void CFlvH264Packet::Init()
{
    __super::Init();

    m_pSEI = NULL;
    m_nSEISize = 0;
    m_bSPS = false;
    m_bPPS = false;
    m_nPSSize = 0;
    m_nPSOffset = 0;
    m_nVideoCodecID = -1;
}

bool CFlvH264Packet::WriteHeader(IDataSink *pDataSink, UINT nWidth, UINT nHeight, UINT nVideoCodecID, DWORD dwStartTime)
{
    Init();

    if (m_pDataBuffer == NULL)
    {
        return false;
    }

    m_pDataSink = pDataSink;
    m_nVideoCodecID = nVideoCodecID;

#ifdef TEST_H264_FLV_FILE
    m_pFlvFile = fopen(TEST_H264_FLV_FILE, "wb");
#endif

    memset(&m_FrameContext, 0, sizeof(m_FrameContext));
    if (m_pFlvFile)
    {
        int metadata_size_pos, data_size;
        unsigned char pBuf[] = "FLV";
        m_pDataBuffer->PutBuffer(pBuf, 3);

        m_pDataBuffer->PutChar(0x01);
        m_pDataBuffer->PutChar(0x01);
        m_pDataBuffer->PutUI32(0x09);
        m_pDataBuffer->PutUI32(0x00);

        m_pDataBuffer->PutChar(0x12);
        metadata_size_pos = m_pDataBuffer->m_nBufCur;
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

        m_pDataBuffer->PutString("width");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(nWidth);

        m_pDataBuffer->PutString("height");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(nHeight);

        m_pDataBuffer->PutString("videodatarate");
        m_pDataBuffer->PutChar(0x00);
        m_FlvContext.bitrate_offset = m_pDataBuffer->m_nBufCur;
        m_pDataBuffer->PutDouble(0);

        m_pDataBuffer->PutString("framerate");
        m_pDataBuffer->PutChar(0x00);
        m_FlvContext.framerate_offset = m_pDataBuffer->m_nBufCur;
        m_pDataBuffer->PutDouble(0);

        m_pDataBuffer->PutString("videocodecid");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(nVideoCodecID);

        m_pDataBuffer->PutString("avclevel");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(51);

        m_pDataBuffer->PutString("avcprofile");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(88); // 88 main, 66 baseline, 77 extended

        m_pDataBuffer->PutString("videokeyframe_frequency");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(5);

        m_pDataBuffer->PutString("");
        m_pDataBuffer->PutChar(0x09);

        data_size = m_pDataBuffer->m_nBufCur - metadata_size_pos - 10;
        m_pDataBuffer->PutUI32(data_size + 11);
        m_pDataBuffer->RewriteBufUI24(metadata_size_pos, data_size);

        FlushBuf(0x12, GetTickCount());

        m_FrameContext.m_nFrame++;
    }

    m_FlvContext.start = dwStartTime;
    m_FlvContext.finish = m_FlvContext.start;
    m_FlvContext.current = m_FlvContext.start;

    return true;
}

bool CFlvH264Packet::WriteTailer()
{
    if (m_pFlvFile != NULL)
    {
        fclose(m_pFlvFile);
        m_pFlvFile = NULL;
    }

    return true;
}

bool CFlvH264Packet::WriteData(const unsigned char *pBuf, int nSize, DWORD lSampleTime)
{
    if (m_pDataBuffer == NULL)
    {
        return false;
    }

    assert(pBuf && nSize > 5);

    if (m_FlvContext.start < 0)
    {
        m_FlvContext.start = lSampleTime;
    }
    m_FlvContext.current = lSampleTime;
    int nTimeStamp = m_FlvContext.current - m_FlvContext.start;
    if (nTimeStamp < 0)
    {
        nTimeStamp = 0;
    }

    int nCodecTag = m_nVideoCodecID;
    unsigned char type = pBuf[4] & 0x1f;
    switch(type)
    {
    case 0x07:
        if (!m_bPPS && !m_bSPS)
        {
            m_nPSSize = 0;

            m_nPSOffset = m_pDataBuffer->m_nBufCur + 1;
            m_pDataBuffer->PutChar(0x09);
            m_pDataBuffer->PutUI24(0);
            m_pDataBuffer->PutUI24(0);
            m_pDataBuffer->PutChar(0);
            m_pDataBuffer->PutUI24(0);

            int nFrameKey = 0x10;
            nCodecTag |= nFrameKey;
            m_pDataBuffer->PutChar(nCodecTag);
            m_pDataBuffer->PutChar(0);
            m_pDataBuffer->PutUI24(0);

            m_nPSSize = 5;

            m_pDataBuffer->PutChar(1);
            m_pDataBuffer->PutChar(pBuf[5]);
            m_pDataBuffer->PutChar(pBuf[6]);
            m_pDataBuffer->PutChar(pBuf[7]);
            m_pDataBuffer->PutChar(0xff);
            m_pDataBuffer->PutChar(0xe1);

            m_nPSSize += 6;

            m_pDataBuffer->PutUI16(nSize - 4);
            m_pDataBuffer->PutBuffer(pBuf + 4, nSize - 4);
            m_nPSSize += 2 + nSize - 4;

            m_bSPS = true;
        }
        break;
    case 0x08:
        if (!m_bPPS && m_bSPS)
        {
            m_pDataBuffer->PutChar(1);
            m_pDataBuffer->PutUI16(nSize - 4);
            m_pDataBuffer->PutBuffer(pBuf + 4, nSize - 4);

            m_nPSSize += 3 + nSize - 4;
            m_pDataBuffer->PutUI32(m_nPSSize + 11);
            m_pDataBuffer->RewriteBufUI24(m_nPSOffset, m_nPSSize);

            FlushBuf(0x09, nTimeStamp);
            m_FrameContext.m_nFrame++;

            m_bPPS = true;
        }
        break;
    case 0x01:
    case 0x05:
    {
        if (!m_bPPS || !m_bSPS)
        {
            break;
        }

        if (!m_FrameContext.m_bWriteFrame)
        {
            m_FrameContext.m_nFrameSizeOffset = m_pDataBuffer->m_nBufCur + 1;

            assert(nTimeStamp >= 0);
            m_pDataBuffer->PutChar(0x09);
            m_pDataBuffer->PutUI24(0);
            m_pDataBuffer->PutUI24(nTimeStamp & 0x00ffffff);
            m_pDataBuffer->PutChar(nTimeStamp >> 24);
            m_pDataBuffer->PutUI24(0x00);

            int nFrameKey = 0x10;
            int nFrameInter = 0x20;
            nCodecTag |= (type == 0x05) ? nFrameKey : nFrameInter;
            m_pDataBuffer->PutChar(nCodecTag);
            m_pDataBuffer->PutChar(1);
            m_pDataBuffer->PutUI24(0);

            m_FrameContext.m_nFrameSize = 5;
            m_FrameContext.m_bWriteFrame = true;
        }

        if (m_pSEI)
        {
            delete[] m_pSEI;
            m_pSEI = NULL;
        }

        unsigned char pBufSize[5] = "";
        pBufSize[0] = (nSize - 4) >> 24;
        pBufSize[1] = (nSize - 4) >> 16;
        pBufSize[2] = (nSize - 4) >> 8;
        pBufSize[3] = (nSize - 4);
        m_pDataBuffer->PutBuffer(pBufSize, 4);
        m_pDataBuffer->PutBuffer(pBuf + 4, nSize - 4);
        m_FrameContext.m_nFrameSize += nSize;
    }
    break;
    case 0x06:
    {
        m_nSEISize = nSize;
        m_pSEI = new unsigned char[m_nSEISize];
        assert(m_pSEI);
        memcpy(m_pSEI, pBuf, m_nSEISize);
        m_pSEI[0] = (nSize - 4) >> 24;
        m_pSEI[1] = (nSize - 4) >> 16;
        m_pSEI[2] = (nSize - 4) >> 8;
        m_pSEI[3] = (nSize - 4);
    }
    break;
    default:
        return false;
    }

    return true;
}

bool CFlvH264Packet::BeginData()
{
    return true;
}

bool CFlvH264Packet::EndData(DWORD lSampleTime)
{
    if (m_pDataBuffer == NULL)
    {
        return false;
    }

    if (m_pDataBuffer->m_nBufCur > 0)
    {
        m_pDataBuffer->PutUI32(m_FrameContext.m_nFrameSize + 11);
        m_pDataBuffer->RewriteBufUI24(m_FrameContext.m_nFrameSizeOffset, m_FrameContext.m_nFrameSize);

        m_FlvContext.current = lSampleTime;
        int nTimeStamp = m_FlvContext.current - m_FlvContext.start;
        if (nTimeStamp < 0)
        {
            nTimeStamp = 0;
        }
        FlushBuf(0x09, nTimeStamp);

        m_FrameContext.m_nFrame++;
        m_FrameContext.m_bWriteFrame = false;
        m_FrameContext.m_nFrameSize = 0;
        m_FrameContext.m_nFrameSizeOffset = 0;

        m_FlvContext.finish = GetTickCount();
        m_FlvContext.duration = (m_FlvContext.finish - m_FlvContext.start);
        assert(m_FlvContext.duration >= 0);
    }

    return true;
}