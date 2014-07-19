#pragma once

#include "Record.h"
#include "Sink.h"
#include "DataBuffer.h"
#include "ScreenCapture.h"
#include "VideoEncoder.h"
#include "AudioEncoder.h"
#include "WndCapture.h"
#include "Flv.h"
#include "FlvH264Packet.h"

#include <map>
#include <list>
using namespace std;

#define MAX_PATHFILE (_MAX_DIR + _MAX_PATH + _MAX_FNAME + _MAX_EXT)
#define MAX_INPUT_DATA_SIZE 30


typedef enum _InputDataType
{
    InputDataAudio,
    InputDataVideo
} InputDataType;

struct AVInputData
{
    AVInputData()
    {
        m_inputDataType = InputDataAudio;
        m_pDataBuffer = NULL;
        m_dwSampleTime = 0;
    }

    ~AVInputData()
    {
        if (m_pDataBuffer)
        {
            delete m_pDataBuffer;
            m_pDataBuffer = NULL;
        }
    }

    InputDataType m_inputDataType;
    CDataBuffer *m_pDataBuffer;
    DWORD m_dwSampleTime;
};


class CRecordImp;

class CFlvVideoDataSink : public IDataSink
{
public:
    CFlvVideoDataSink(CRecordImp *pRecordImp);
    ~CFlvVideoDataSink();

    virtual bool OnSample(int nPacketType, const unsigned char *pBuffer, int nBufLength, DWORD lSampleTime);

private:
    CRecordImp *m_pRecordImp;
};


class CFlvAudioDataSink : public IDataSink
{
public:
    CFlvAudioDataSink(CRecordImp *pRecordImp);
    ~CFlvAudioDataSink();

    virtual bool OnSample(int nPacketType, const unsigned char *pBuffer, int nBufLength, DWORD lSampleTime);

private:
    CRecordImp *m_pRecordImp;
};

class CRecordImp : public IRecord
{
public:
    CRecordImp();
    virtual ~CRecordImp();

    virtual void SetSink(SINK_DATA_TYPE sdType, IDataSink *pDataSink);
    virtual void SetVideoParam(UINT uiFrameRate, RECORD_VIDEO_CODEC enCodec);
    virtual void SetVideoParam2(HWND hWnd, UINT uiX, UINT uiY, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, RECORD_VIDEO_CODEC enCodec);
    virtual void SetAudioParam(UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, RECORD_AUDIO_CODEC enCodec);
    virtual bool Start(char *sFile);
    virtual bool Stop();
    virtual bool Pause();
    virtual bool Resume();
    virtual void WriteAudioData(DWORD lSampleTime, BYTE *pBuf, int nLen);
    virtual void WriteVideoData(DWORD lSampleTime, BYTE *pBuf, int nLen);

    void WriteData(int nPacketType, const unsigned char *pBuffer, int nBufLength, DWORD lSampleTime);

    virtual RECORD_STATUS GetStatus();
    virtual DWORD GetStartTime();
    virtual DWORD GetDuration();

protected:
    bool WriteFlvHeader();
    bool WriteFlvTailer();
    void Init();
    void Release();
    bool StartVideo(DWORD dwStartTime);
    bool StartAudio(DWORD dwStartTime);
    void RewriteFileDouble(__int64 pos, double val);
    void UpdataFileInfo();
    int ReadOneNal(const unsigned char *buf, int buf_size);

    void ProcessVideoData(DWORD dwSampleTime, BYTE *pBuf, int nLen);
    void ProcessAudioData(DWORD dwSampleTime, BYTE *pBuf, int nLen);

    UINT m_uiFrameRate;
    UINT m_uiNumChannels;
    UINT m_uiSampleRate;
    UINT m_uiBitsPerSample;

    // 可指定窗口捕获或区域捕获
    HWND m_hWnd;
    UINT m_uiX;
    UINT m_uiY;
    UINT m_uiWidth;
    UINT m_uiHeight;

    RECORD_VIDEO_CODEC	m_enVideoCodec;
    RECORD_AUDIO_CODEC	m_enAudioCodec;
    RECORD_STATUS		m_enRecordStatus;

    FILE *m_pFile;
    char m_sFile[MAX_PATHFILE];
    CDataBuffer *m_pDataBuffer;
    CScreenCapture *m_pScreenCapture;
    CVideoEncoder *m_pVideoEncoder;
    CAudioEncoder *m_pAudioEncoder;
    CFlvVideoDataSink *m_pVideoDataSink;
    CFlvAudioDataSink *m_pAudioDataSink;
    CWndCapture *m_pWndCapture;
    FlvContext m_FlvContext;
    CFlvH264Packet *m_pFlvVideo;
    bool m_bRecordVideo;
    bool m_bRecordAudio;
    DWORD m_dwStartTime;
    DWORD m_dwDuration;

    DWORD m_nPausedTime;
    DWORD m_nPausedDuration;

    CRITICAL_SECTION m_csFlvFile;
    CRITICAL_SECTION m_csProcessVideo;
    CRITICAL_SECTION m_csProcessAudio;

    map<SINK_DATA_TYPE, IDataSink *> m_aDataSink;

private:
    static unsigned __stdcall DataProcessThread(void *pParam);
    void DataProcessThreadProc();
    void PushInputDataToList(InputDataType inputDataType, BYTE *pBuf, int nLen, DWORD dwSampleTime);
    void ProcessInputDataList();
    void ClearInputDataList();

    HANDLE m_hDataProcessThread;
    CRITICAL_SECTION m_csAVInputList;
    list<AVInputData *> m_aWorkList;
    list<AVInputData *> m_aFreeList;
};

