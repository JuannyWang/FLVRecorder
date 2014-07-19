#pragma once

#include <time.h>
#include "DataBuffer.h"
#include "Sink.h"


typedef struct _FlvContext
{
    __int64 duration_offset;
    __int64 filesize_offset;
    __int64 bitrate_offset;
    __int64 framerate_offset;
    DWORD start, finish, current;
    __int64 duration;
} FlvContext;

typedef struct _FrameContext
{
    int m_nFrame;
    int m_nFrameSize;
    __int64 m_nFrameSizeOffset;
    bool m_bWriteFrame;
} FrameContext;

class CFlv
{
public:
    CFlv(void);
    ~CFlv(void);

protected:
    void Init();

    IDataSink *m_pDataSink;

protected:
    bool FlushBuf(int nPacketType, DWORD lSampleTime);
    void RewriteFileDouble(__int64 pos, double val);

    FrameContext m_FrameContext;

    FlvContext m_FlvContext;
    FILE *m_pFlvFile;

    CDataBuffer *m_pDataBuffer;
};
