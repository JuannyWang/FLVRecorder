#pragma once

#include "VideoEncoder.h"


class CScreenCapture
{
public:
    CScreenCapture(void);
    ~CScreenCapture(void);

public:
    bool Start(IDataSink *pDataSink, UINT uiFrameRate, CVideoEncoder *pVideoEncoder, DWORD dwStartTime);
    bool Start2(IDataSink *pDataSink, HWND hWnd, UINT uiX, UINT uiY, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, CVideoEncoder *pVideoEncoder, DWORD dwStartTime);
    bool Stop();
    bool Pause();
    bool Resume();

protected:
    bool Init();
    void Release();
    static unsigned	__stdcall CaptureThread(void *pParam);
    void CaptureThreadProc();
    bool CaptureScreen();
    bool DrawCursor(HDC hDC);

    HANDLE m_hCaptureThread;
    bool m_bStop;
    bool m_bPause;
    clock_t m_nStartTime;
    clock_t m_nCaptureTime;
    long m_nCaptureInterval;

    HWND m_hSrcnWnd;
    HDC m_hSrcnDC;
    HDC m_hSrcnBmpDC;
    HBITMAP m_hSrcnBitmap;
    LPVOID m_pSrcnBuf;
    BITMAPINFO m_bmpInfo;

    UINT m_uiX;
    UINT m_uiY;
    UINT m_uiWidth;
    UINT m_uiHeight;

    CVideoEncoder *m_pVideoEncoder;

    DWORD m_nPausedTime;
    DWORD m_nPausedDuration;
};
