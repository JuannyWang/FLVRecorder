#include "StdAfx.h"
#include "RecordImp.h"
#include "H264Encoder.h"
#include "Mp3Encoder.h"

CFlvVideoDataSink::CFlvVideoDataSink(CRecordImp *pRecordImp)
{
    m_pRecordImp = pRecordImp;
}

CFlvVideoDataSink::~CFlvVideoDataSink()
{
}

bool CFlvVideoDataSink::OnSample(int nPacketType, const unsigned char *pBuffer, int nBufLength, DWORD lSampleTime)
{
    if (m_pRecordImp)
    {
        m_pRecordImp->WriteData(nPacketType, pBuffer, nBufLength, lSampleTime);
    }
    return true;
}

CFlvAudioDataSink::CFlvAudioDataSink(CRecordImp *pRecordImp)
{
    m_pRecordImp = pRecordImp;
}

CFlvAudioDataSink::~CFlvAudioDataSink()
{
}

bool CFlvAudioDataSink::OnSample(int nPacketType, const unsigned char *pBuffer, int nBufLength, DWORD lSampleTime)
{
    if (m_pRecordImp)
    {
        m_pRecordImp->WriteData(nPacketType, pBuffer, nBufLength, lSampleTime);
    }
    return true;
}

CRecordImp::CRecordImp()
{
    InitializeCriticalSection(&m_csFlvFile);
    InitializeCriticalSection(&m_csProcessVideo);
    InitializeCriticalSection(&m_csProcessAudio);
    InitializeCriticalSection(&m_csAVInputList);

    Init();
}

CRecordImp::~CRecordImp()
{
    Release();

    DeleteCriticalSection(&m_csFlvFile);
    DeleteCriticalSection(&m_csProcessVideo);
    DeleteCriticalSection(&m_csProcessAudio);
    DeleteCriticalSection(&m_csAVInputList);
}

void CRecordImp::Init()
{
    m_pFile = NULL;
    memset(m_sFile, 0, MAX_PATHFILE);
    m_pDataBuffer = NULL;
    m_pScreenCapture = NULL;
    m_pWndCapture = NULL;
    m_pVideoEncoder = NULL;
    m_pAudioEncoder = NULL;
    m_pVideoDataSink = NULL;
    m_pAudioDataSink = NULL;
    m_pFlvVideo = NULL;
    m_bRecordVideo = false;
    m_bRecordAudio = false;

    m_enVideoCodec = RECORD_VIDEO_CODEC_NULL;
    m_enAudioCodec = RECORD_AUDIO_CODEC_NULL;
    m_uiFrameRate = 10;
    m_uiNumChannels = 2;
    m_uiSampleRate = 11025;
    m_uiBitsPerSample = 16;

    m_enRecordStatus = RECORD_STATUS_NULL;

    m_nPausedTime = 0;
    m_nPausedDuration = 0;
    m_dwDuration = 0;

    m_hWnd = NULL;
    m_uiX = 0;
    m_uiY = 0;
    m_uiWidth = 0;
    m_uiHeight = 0;
    m_hDataProcessThread = NULL;
}

void CRecordImp::Release()
{
    ClearInputDataList();
    if (m_hDataProcessThread)
    {
        DWORD dwRet = WaitForSingleObject(m_hDataProcessThread, 2000);
        switch(dwRet)
        {
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            break;
        case WAIT_ABANDONED:
            break;
        }

        CloseHandle(m_hDataProcessThread);
        m_hDataProcessThread = NULL;
    }

    if (m_pScreenCapture)
    {
        delete m_pScreenCapture;
        m_pScreenCapture = NULL;
    }

    if (m_pWndCapture)
    {
        delete m_pWndCapture;
        m_pWndCapture = NULL;
    }

    if (m_pVideoEncoder)
    {
        delete m_pVideoEncoder;
        m_pVideoEncoder = NULL;
    }

    if (m_pAudioEncoder)
    {
        delete m_pAudioEncoder;
        m_pAudioEncoder = NULL;
    }

    if (m_pFlvVideo)
    {
        delete m_pFlvVideo;
        m_pFlvVideo = NULL;
    }

    if (m_pVideoDataSink)
    {
        delete m_pVideoDataSink;
        m_pVideoDataSink = NULL;
    }

    if (m_pAudioDataSink)
    {
        delete m_pAudioDataSink;
        m_pAudioDataSink = NULL;
    }

    if (m_pDataBuffer)
    {
        delete m_pDataBuffer;
        m_pDataBuffer = NULL;
    }

    if (m_pFile)
    {
        fclose(m_pFile);
        m_pFile = NULL;
    }

    memset(m_sFile, 0, MAX_PATHFILE);
}

void CRecordImp::SetSink(SINK_DATA_TYPE sdType, IDataSink *pDataSink)
{
    switch(sdType)
    {
    case SINK_SCRN_DATA_H264:
    {
        if (pDataSink != NULL)
        {
            m_aDataSink.insert(pair<SINK_DATA_TYPE, IDataSink *>(sdType, pDataSink));
        }
    }
    break;
    }
}

void CRecordImp::SetVideoParam(UINT uiFrameRate, RECORD_VIDEO_CODEC enCodec)
{
    assert(m_enRecordStatus == RECORD_STATUS_NULL);
    assert(m_enVideoCodec == RECORD_VIDEO_CODEC_NULL);
    assert(m_hWnd == NULL);

    m_bRecordVideo = true;
    m_uiFrameRate = uiFrameRate;
    m_enVideoCodec = enCodec;

    m_hWnd = NULL;
    m_uiX = 0;
    m_uiY = 0;
    m_uiWidth = 0;
    m_uiHeight = 0;
}

void CRecordImp::SetVideoParam2(HWND hWnd, UINT uiX, UINT uiY, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, RECORD_VIDEO_CODEC enCodec)
{
    assert(m_enRecordStatus == RECORD_STATUS_NULL);
    assert(m_enVideoCodec == RECORD_VIDEO_CODEC_NULL);
    assert(m_hWnd == NULL);

    m_bRecordVideo = true;
    m_uiFrameRate = uiFrameRate;
    m_enVideoCodec = enCodec;

    m_hWnd = hWnd;
    m_uiX = uiX;
    m_uiY = uiY;
    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;
}

void CRecordImp::SetAudioParam(UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, RECORD_AUDIO_CODEC enCodec)
{
    assert(m_enRecordStatus == RECORD_STATUS_NULL);
    assert(m_enAudioCodec == RECORD_AUDIO_CODEC_NULL);

    m_bRecordAudio = true;
    m_uiNumChannels = uiNumChannels;
    m_uiSampleRate = uiSampleRate;
    m_uiBitsPerSample = uiBitsPerSample;
    m_enAudioCodec = enCodec;
}

bool CRecordImp::Start(char *sFile)
{
    bool bRet = true;
    do
    {
        m_enRecordStatus = RECORD_STATUS_NULL;
        m_dwDuration = 0;
        Release();

        memset(&m_FlvContext, 0, sizeof(FlvContext));

        m_pDataBuffer = new CDataBuffer;
        if (m_pDataBuffer == NULL)
        {
            bRet = false;
            break;
        }

        strcpy(m_sFile, sFile);

        m_dwStartTime = GetTickCount();
        if (m_bRecordVideo)
        {
            if (!StartVideo(m_dwStartTime))
            {
                bRet = false;
                break;
            }
        }

        if (m_bRecordAudio)
        {
            if (!StartAudio(m_dwStartTime))
            {
                bRet = false;
                break;
            }
        }

        m_enRecordStatus = RECORD_STATUS_RECORDING;

    }
    while (0);

    if (!bRet)
    {
        Release();
    }

    return bRet;
}

unsigned CRecordImp::DataProcessThread(void *pParam)
{
    CRecordImp *pThis = static_cast<CRecordImp *>(pParam);
    if (pThis)
    {
        pThis->DataProcessThreadProc();
    }

    return 0;
}

void CRecordImp::DataProcessThreadProc()
{
    while(m_enRecordStatus != RECORD_STATUS_NULL)
    {
        ProcessInputDataList();

        Sleep(1);
    }
}

bool CRecordImp::StartVideo(DWORD dwStartTime)
{
    bool bRet = true;

    if (m_hWnd == NULL && m_enVideoCodec == RECORD_VIDEO_CODEC_NULL)
    {
        do
        {
            m_pVideoDataSink = new CFlvVideoDataSink(this);
            if (m_pVideoDataSink == NULL)
            {
                bRet = false;
                break;
            }
        }
        while (0);

        return bRet;
    }

    do
    {
        switch(m_enVideoCodec)
        {
        case RECORD_H264:
        {
            m_pVideoEncoder = new CH264Encoder;
            if (m_pVideoEncoder == NULL)
            {
                bRet = false;
                break;
            }
        }
        break;
        default:
        {
            bRet = false;
        }
        break;
        }
        if (!bRet)
        {
            break;
        }

        m_pVideoDataSink = new CFlvVideoDataSink(this);
        if (m_pVideoDataSink == NULL)
        {
            bRet = false;
            break;
        }

        if (m_hWnd == NULL)
        {
            m_pScreenCapture = new CScreenCapture;
            if (m_pScreenCapture == NULL)
            {
                bRet = false;
                break;
            }

            if (!m_pScreenCapture->Start(m_pVideoDataSink, m_uiFrameRate, m_pVideoEncoder, dwStartTime))
            {
                bRet = false;
                break;
            }
        }
        else
        {
            if (m_hWnd == ::GetDesktopWindow())
            {
                m_pScreenCapture = new CScreenCapture;
                if (m_pScreenCapture == NULL)
                {
                    bRet = false;
                    break;
                }

                if (!m_pScreenCapture->Start2(m_pVideoDataSink, m_hWnd, m_uiX, m_uiY, m_uiWidth, m_uiHeight, m_uiFrameRate, m_pVideoEncoder, dwStartTime))
                {
                    bRet = false;
                    break;
                }
            }
            else
            {
                m_pWndCapture = new CWndCapture;
                if (m_pWndCapture == NULL)
                {
                    bRet = false;
                    break;
                }

                if (!m_pWndCapture->Start(m_pVideoDataSink, m_hWnd, m_uiFrameRate, m_pVideoEncoder, dwStartTime))
                {
                    bRet = false;
                    break;
                }
            }
        }
    }
    while (0);

    return bRet;
}

bool CRecordImp::StartAudio(DWORD dwStartTime)
{
    bool bRet = true;
    do
    {
        switch(m_enVideoCodec)
        {
        case RECORD_MP3:
        {
            m_pAudioEncoder = new CMp3Encoder;
            if (m_pAudioEncoder == NULL)
            {
                bRet = false;
            }
        }
        break;
        default:
        {
            bRet = false;
        }
        break;
        }
        if (!bRet)
        {
            break;
        }

        m_pAudioDataSink = new CFlvAudioDataSink(this);
        if (m_pAudioDataSink == NULL)
        {
            bRet = false;
            break;
        }

        if (!m_pAudioEncoder->Begin(m_pAudioDataSink, m_uiNumChannels, m_uiSampleRate, m_uiBitsPerSample, dwStartTime))
        {
            bRet = false;
            break;
        }
    }
    while (0);

    m_nPausedTime = 0;
    m_nPausedDuration = 0;

    return bRet;
}

bool CRecordImp::Stop()
{
    m_enRecordStatus = RECORD_STATUS_NULL;
    if (m_pScreenCapture)
    {
        m_pScreenCapture->Stop();
    }

    if (m_pWndCapture)
    {
        m_pWndCapture->Stop();
    }

    EnterCriticalSection(&m_csProcessAudio);
    if (m_pAudioEncoder)
    {
        m_pAudioEncoder->End();
    }
    LeaveCriticalSection(&m_csProcessAudio);

    EnterCriticalSection(&m_csProcessVideo);
    if (m_pFlvVideo)
    {
        m_pFlvVideo->WriteTailer();
    }
    LeaveCriticalSection(&m_csProcessVideo);

    WriteFlvTailer();

    m_aDataSink.clear();

    Release();

    m_bRecordVideo = false;
    m_bRecordAudio = false;

    m_enVideoCodec = RECORD_VIDEO_CODEC_NULL;
    m_enAudioCodec = RECORD_AUDIO_CODEC_NULL;

    m_nPausedTime = 0;
    m_nPausedDuration = 0;

    m_hWnd = NULL;
    m_uiX = 0;
    m_uiY = 0;
    m_uiWidth = 0;
    m_uiHeight = 0;

    return true;
}

bool CRecordImp::Pause()
{
    m_enRecordStatus = RECORD_STATUS_PAUSE;
    if (m_pScreenCapture)
    {
        m_pScreenCapture->Pause();
    }

    if (m_pWndCapture)
    {
        m_pWndCapture->Pause();
    }

    ClearInputDataList();

    m_nPausedTime = GetTickCount();

    return true;
}

bool CRecordImp::Resume()
{
    if (m_pScreenCapture)
    {
        m_pScreenCapture->Resume();
    }

    if (m_pWndCapture)
    {
        m_pWndCapture->Resume();
    }

    m_enRecordStatus = RECORD_STATUS_RECORDING;
    m_nPausedDuration += (GetTickCount() - m_nPausedTime);

    return true;
}

void CRecordImp::WriteAudioData(DWORD lSampleTime, BYTE *pBuf, int nLen)
{
    ProcessAudioData(lSampleTime - m_nPausedDuration, pBuf, nLen);
}

void CRecordImp::WriteVideoData(DWORD lSampleTime, BYTE *pBuf, int nLen)
{
    if (m_pFlvVideo == NULL)
    {
        m_pFlvVideo = new CFlvH264Packet;
        if (m_pFlvVideo == NULL)
        {

        }

        UINT nCodecId = 7; // AVC Video CodecID
        if (!m_pFlvVideo->WriteHeader(m_pVideoDataSink, 0, 0, nCodecId, lSampleTime))
        {
        }
    }

    ProcessVideoData(lSampleTime - m_nPausedDuration, pBuf, nLen);
}

void CRecordImp::PushInputDataToList(InputDataType inputDataType, BYTE *pBuf, int nLen, DWORD dwSampleTime)
{
    if (m_enRecordStatus != RECORD_STATUS_RECORDING)
    {
        return;
    }

    AVInputData *pAVInputData = NULL;
    EnterCriticalSection(&m_csAVInputList);

    if (m_aWorkList.size() + m_aFreeList.size() > MAX_INPUT_DATA_SIZE)
    {
        LeaveCriticalSection(&m_csAVInputList);
        return;
    }

    if (m_aFreeList.size() <= 0)
    {
        pAVInputData = new AVInputData;
    }
    else
    {
        pAVInputData = m_aFreeList.front();
        m_aFreeList.pop_front();
    }
    LeaveCriticalSection(&m_csAVInputList);

    if (pAVInputData == NULL)
    {
        return;
    }

    pAVInputData->m_inputDataType = inputDataType;
    pAVInputData->m_dwSampleTime = dwSampleTime;
    if (pAVInputData->m_pDataBuffer == NULL)
    {
        pAVInputData->m_pDataBuffer = new CDataBuffer;
    }
    if (pAVInputData->m_pDataBuffer == NULL)
    {
        delete pAVInputData;
        return;
    }

    pAVInputData->m_pDataBuffer->PutBuffer(pBuf, nLen);

    EnterCriticalSection(&m_csAVInputList);
    m_aWorkList.push_back(pAVInputData);
    LeaveCriticalSection(&m_csAVInputList);
}

void CRecordImp::ProcessInputDataList()
{
    if (m_enRecordStatus != RECORD_STATUS_RECORDING)
    {
        return;
    }

    AVInputData *pAVInputData = NULL;
    EnterCriticalSection(&m_csAVInputList);
    if (m_aWorkList.size() > 0)
    {
        pAVInputData = m_aWorkList.front();
        m_aWorkList.pop_front();
    }
    LeaveCriticalSection(&m_csAVInputList);

    if (pAVInputData && pAVInputData->m_pDataBuffer != NULL)
    {
        if (pAVInputData->m_inputDataType == InputDataAudio)
        {
            ProcessAudioData(pAVInputData->m_dwSampleTime, pAVInputData->m_pDataBuffer->m_pBuffer, pAVInputData->m_pDataBuffer->m_nBufCur);
        }
        else if (pAVInputData->m_inputDataType == InputDataVideo)
        {
            ProcessVideoData(pAVInputData->m_dwSampleTime, pAVInputData->m_pDataBuffer->m_pBuffer, pAVInputData->m_pDataBuffer->m_nBufCur);
        }
    }

    EnterCriticalSection(&m_csAVInputList);
    if (pAVInputData)
    {
        if (m_aWorkList.size() + m_aFreeList.size() > MAX_INPUT_DATA_SIZE)
        {
            delete pAVInputData;
        }
        else
        {
            if (pAVInputData->m_pDataBuffer)
            {
                pAVInputData->m_pDataBuffer->m_nBufCur = 0;
                m_aFreeList.push_back(pAVInputData);
            }
            else
            {
                delete pAVInputData;
            }
        }
    }
    LeaveCriticalSection(&m_csAVInputList);
}

void CRecordImp::ProcessVideoData(DWORD dwSampleTime, BYTE *pBuf, int nLen)
{
    if (m_enRecordStatus != RECORD_STATUS_RECORDING)
    {
        return;
    }

    EnterCriticalSection(&m_csProcessVideo);
    if (m_pFlvVideo)
    {
        m_pFlvVideo->BeginData();

        BYTE *pBufT = pBuf;
        UINT32 nLenT = nLen;
        int iNalLen = 0;
        while(nLenT > 0)
        {
            iNalLen = ReadOneNal(pBufT, nLenT);
            if (0 == iNalLen)
            {
                iNalLen = nLenT;
            }

            m_pFlvVideo->WriteData(pBufT, iNalLen, dwSampleTime);

            pBufT += iNalLen;
            nLenT -= iNalLen;
        }

        m_pFlvVideo->EndData(dwSampleTime);
    }
    LeaveCriticalSection(&m_csProcessVideo);
}

void CRecordImp::ProcessAudioData(DWORD dwSampleTime, BYTE *pBuf, int nLen)
{
    if (m_enRecordStatus != RECORD_STATUS_RECORDING)
    {
        return;
    }

    EnterCriticalSection(&m_csProcessAudio);

    if (m_pAudioEncoder)
    {
        m_pAudioEncoder->Encode(dwSampleTime, pBuf, nLen);
    }

    LeaveCriticalSection(&m_csProcessAudio);
}

void CRecordImp::ClearInputDataList()
{
    EnterCriticalSection(&m_csAVInputList);

    for (list<AVInputData *>::iterator it = m_aWorkList.begin(); it != m_aWorkList.end(); it++)
    {
        delete (*it);
    }
    for (list<AVInputData *>::iterator it = m_aFreeList.begin(); it != m_aFreeList.end(); it++)
    {
        delete (*it);
    }

    m_aWorkList.clear();
    m_aFreeList.clear();

    LeaveCriticalSection(&m_csAVInputList);
}

bool CRecordImp::WriteFlvHeader()
{
    unsigned char pBuf[] = "FLV";
    m_pDataBuffer->PutBuffer(pBuf, 3);

    DWORD dwRecordType = 0;
    if (m_bRecordAudio)
    {
        dwRecordType |= 0x04;
    }

    if (m_bRecordVideo)
    {
        dwRecordType |= 0x01;
    }

    m_pDataBuffer->PutChar(0x01); // version
    m_pDataBuffer->PutChar(dwRecordType); // type tag : video | audio, 00000001 | 00000100
    m_pDataBuffer->PutUI32(0x09); // reserved
    m_pDataBuffer->PutUI32(0x00); // pre tag

    // meta tag
    int metadata_size_pos, data_size;
    m_pDataBuffer->PutChar(0x12); // tag type META
    metadata_size_pos = m_pDataBuffer->m_nBufCur;
    m_pDataBuffer->PutUI24(0); // size of data part
    m_pDataBuffer->PutUI24(0); // time stamp
    m_pDataBuffer->PutUI32(0); // reserved

    m_pDataBuffer->PutChar(0x02); // string type
    m_pDataBuffer->PutString("onMetaData"); // 12 bytes

    m_pDataBuffer->PutChar(0x03);
    m_pDataBuffer->PutString("duration");
    m_pDataBuffer->PutChar(0x00);
    m_FlvContext.duration_offset = m_pDataBuffer->m_nBufCur;
    m_pDataBuffer->PutDouble(0);

    m_pDataBuffer->PutString("filesize");
    m_pDataBuffer->PutChar(0x00);
    m_FlvContext.filesize_offset = m_pDataBuffer->m_nBufCur;
    m_pDataBuffer->PutDouble(0);

    if (m_uiWidth > 0 && m_uiHeight > 0)
    {
        m_pDataBuffer->PutString("width");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(m_uiWidth);

        m_pDataBuffer->PutString("height");
        m_pDataBuffer->PutChar(0x00);
        m_pDataBuffer->PutDouble(m_uiHeight);
    }

    m_pDataBuffer->PutString("videodatarate");
    m_pDataBuffer->PutChar(0x00);
    m_FlvContext.bitrate_offset = m_pDataBuffer->m_nBufCur;
    m_pDataBuffer->PutDouble(0);

    m_pDataBuffer->PutString("");
    m_pDataBuffer->PutChar(0x09);

    data_size = m_pDataBuffer->m_nBufCur - metadata_size_pos - 10;
    m_pDataBuffer->PutUI32(data_size + 11);
    m_pDataBuffer->RewriteBufUI24(metadata_size_pos, data_size);

    bool bRet = true;
    if (m_pFile)
    {
        size_t len = fwrite(m_pDataBuffer->m_pBuffer, sizeof(char), m_pDataBuffer->m_nBufCur, m_pFile);
        bRet = (len == m_pDataBuffer->m_nBufCur);
    }
    m_pDataBuffer->m_nBufCur = 0;

    return bRet;
}

bool CRecordImp::WriteFlvTailer()
{
    if (m_pFile)
    {
        UpdataFileInfo();
        fclose(m_pFile);
        m_pFile = NULL;
    }
    memset(m_sFile, 0, sizeof(MAX_PATHFILE));

    return true;
}

void CRecordImp::WriteData(int nPacketType, const unsigned char *pBuffer, int nBufLength, DWORD lSampleTime)
{
    EnterCriticalSection(&m_csFlvFile);

    if (m_pFile == NULL)
    {
        m_pFile = fopen(m_sFile, "wb");
        if (m_pFile == NULL)
        {
        }
        WriteFlvHeader();
    }

    if (m_pFile)
    {
        int nBufLengthT = nBufLength;
        while(nBufLengthT > 0)
        {
            size_t nWriteLen = fwrite(pBuffer, sizeof(char), nBufLengthT, m_pFile);
            nBufLengthT -= nWriteLen;
        }

        if (nPacketType == 0x8 || nPacketType == 0x9)
        {
            m_FlvContext.duration = lSampleTime / CLOCKS_PER_SEC;
            UpdataFileInfo();
        }
    }

    LeaveCriticalSection(&m_csFlvFile);
}

RECORD_STATUS CRecordImp::GetStatus()
{
    return m_enRecordStatus;
}

DWORD CRecordImp::GetStartTime()
{
    return m_dwStartTime;
}

DWORD CRecordImp::GetDuration()
{
    return m_dwDuration;
}

void CRecordImp::UpdataFileInfo()
{
    if (m_pFile)
    {
        fseek(m_pFile, 0, SEEK_END);
        double dFileSize = (double)ftell(m_pFile);
        (m_FlvContext.duration <= 0) ? (m_FlvContext.duration = 1) : 0;
        RewriteFileDouble(m_FlvContext.duration_offset, (double)m_FlvContext.duration);
        RewriteFileDouble(m_FlvContext.filesize_offset, dFileSize);
        double dBitRate = dFileSize * 8 / (m_FlvContext.duration * 1000);
        RewriteFileDouble(m_FlvContext.bitrate_offset, dBitRate);

        m_dwDuration = m_FlvContext.duration;
    }
}

void CRecordImp::RewriteFileDouble(__int64 pos, double val)
{
    if (m_pFile)
    {
        m_pDataBuffer->PutDouble(val);
        fseek(m_pFile, (long)pos, SEEK_SET);
        size_t len = fwrite(m_pDataBuffer->m_pBuffer, sizeof(char), m_pDataBuffer->m_nBufCur, m_pFile);
        m_pDataBuffer->m_nBufCur = 0;
        fseek(m_pFile, 0, SEEK_END);
    }
    else
    {

    }
}

int CRecordImp::ReadOneNal(const unsigned char *buf, int buf_size)
{
    int i;
    unsigned int state = 7;
    int frame_start_found = 0;

    if(state > 13)
        state = 7;

    for (i = 0; i < buf_size; i++)
    {
        if (state == 7)
        {
            for (; i < buf_size; i++)
            {
                if (!buf[i])
                {
                    state = 2;
                    break;
                }
            }
        }
        else if (state <= 2)
        {
            if(buf[i] == 1)
                state ^= 5;
            else if(buf[i])
                state = 7;
            else
                state >>= 1;
        }
        else if (state <= 5)
        {
            int v = buf[i] & 0x1F;

            if (v == 1 || v == 2 || v == 5
                    || v == 6 || v == 7 || v == 8 || v == 9)
            {
                if(frame_start_found)
                {
                    i++;
                    goto found;
                }
                else
                {
                    frame_start_found = 1;
                }
            }
            state = 7;
        }
        else
        {
            state = 7;
        }
    }

    return 0;

found:
    return i - (state & 5);
}