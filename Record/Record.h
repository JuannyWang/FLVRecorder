#pragma once

#include "Sink.h"


#ifdef RECORD_EXPORTS
#define RECORD_EXPORTS_FUNC _declspec(dllexport)
#else
#define RECORD_EXPORTS_FUNC _declspec(dllimport)
#endif

#define RECORD_VIDEO_FRAME_RATE			5
#define RECORD_AUDIO_CHANNEL			2
#define RECORD_AUDIO_SAMPLE_RATE		11025
#define RECORD_AUDIO_BIT_PER_SAMPLE		16
#define RECORD_VIDEO_FILE_NAME			"Record.flv"


// ��Ƶ�����ʽ
typedef enum _RECORD_VIDEO_CODEC
{
    RECORD_VIDEO_CODEC_NULL = 0,
    RECORD_H264
} RECORD_VIDEO_CODEC;

// ��Ƶ�����ʽ
typedef enum _RECORD_AUDIO_CODEC
{
    RECORD_AUDIO_CODEC_NULL = 0,
    RECORD_MP3
} RECORD_AUDIO_CODEC;

// ¼��״̬
typedef enum _RECORD_STATUS
{
    RECORD_STATUS_NULL = 0,
    RECORD_STATUS_RECORDING,
    RECORD_STATUS_PAUSE
} RECORD_STATUS;

// ���ݻص�����
typedef enum _SINK_DATA_TYPE
{
    SINK_SCRN_DATA_H264 = 0
} SINK_DATA_TYPE;


// ¼��ģ��ӿ���

class IRecord
{
public:
    IRecord(void) {};
    virtual ~IRecord(void) {};

    // ���÷�����¼�ƻص�����
    virtual void SetSink(SINK_DATA_TYPE sdType, IDataSink *pDataSink) = 0;

    // ��������������
    virtual void SetVideoParam(UINT uiFrameRate, RECORD_VIDEO_CODEC enCodec) = 0;

    // ��������������
    virtual void SetVideoParam2(HWND hWnd, UINT uiX, UINT uiY, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, RECORD_VIDEO_CODEC enCodec) = 0;

    // ������Ƶ������
    virtual void SetAudioParam(UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, RECORD_AUDIO_CODEC enCodec) = 0;

    // ����¼��
    virtual bool Start(char *sFile) = 0;

    // ֹͣ¼��
    virtual bool Stop() = 0;

    // ��ͣ¼��
    virtual bool Pause() = 0;

    // �ָ�¼��
    virtual bool Resume() = 0;

    // �����ƵԭʼPCM����
    virtual void WriteAudioData(DWORD lSampleTime, BYTE *pBuf, int nLen) = 0;

    // ������ƵH264һ֡����
    virtual void WriteVideoData(DWORD lSampleTime, BYTE *pBuf, int nLen) = 0;

    // ��ȡ��ǰ¼��״̬
    virtual RECORD_STATUS GetStatus() = 0;

    // ��ȡ��ʼ¼��ʱ��
    virtual DWORD GetStartTime() = 0;

    // ��ȡ¼��ʱ�䳤��
    virtual DWORD GetDuration() = 0;
};

#ifdef __cplusplus
extern "C" {
#endif

    RECORD_EXPORTS_FUNC IRecord *CreateRecord();
    RECORD_EXPORTS_FUNC void DestoryRecord(IRecord *pRecord);


#ifdef __cplusplus
}
#endif
