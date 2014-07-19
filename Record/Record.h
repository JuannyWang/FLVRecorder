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


// 视频编码格式
typedef enum _RECORD_VIDEO_CODEC
{
    RECORD_VIDEO_CODEC_NULL = 0,
    RECORD_H264
} RECORD_VIDEO_CODEC;

// 音频编码格式
typedef enum _RECORD_AUDIO_CODEC
{
    RECORD_AUDIO_CODEC_NULL = 0,
    RECORD_MP3
} RECORD_AUDIO_CODEC;

// 录制状态
typedef enum _RECORD_STATUS
{
    RECORD_STATUS_NULL = 0,
    RECORD_STATUS_RECORDING,
    RECORD_STATUS_PAUSE
} RECORD_STATUS;

// 数据回调类型
typedef enum _SINK_DATA_TYPE
{
    SINK_SCRN_DATA_H264 = 0
} SINK_DATA_TYPE;


// 录制模块接口类

class IRecord
{
public:
    IRecord(void) {};
    virtual ~IRecord(void) {};

    // 设置服务器录制回调函数
    virtual void SetSink(SINK_DATA_TYPE sdType, IDataSink *pDataSink) = 0;

    // 设置桌面流参数
    virtual void SetVideoParam(UINT uiFrameRate, RECORD_VIDEO_CODEC enCodec) = 0;

    // 设置桌面流参数
    virtual void SetVideoParam2(HWND hWnd, UINT uiX, UINT uiY, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, RECORD_VIDEO_CODEC enCodec) = 0;

    // 设置音频流参数
    virtual void SetAudioParam(UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, RECORD_AUDIO_CODEC enCodec) = 0;

    // 启动录制
    virtual bool Start(char *sFile) = 0;

    // 停止录制
    virtual bool Stop() = 0;

    // 暂停录制
    virtual bool Pause() = 0;

    // 恢复录制
    virtual bool Resume() = 0;

    // 输出音频原始PCM数据
    virtual void WriteAudioData(DWORD lSampleTime, BYTE *pBuf, int nLen) = 0;

    // 输入视频H264一帧数据
    virtual void WriteVideoData(DWORD lSampleTime, BYTE *pBuf, int nLen) = 0;

    // 获取当前录制状态
    virtual RECORD_STATUS GetStatus() = 0;

    // 获取开始录制时间
    virtual DWORD GetStartTime() = 0;

    // 获取录制时间长度
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
