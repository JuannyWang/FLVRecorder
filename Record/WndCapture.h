#pragma once

#include "VideoEncoder.h"


class CWndCapture
{
public:
    CWndCapture(void);
    ~CWndCapture(void);

public:
    bool Start(IDataSink *pDataSink, HWND hWnd, UINT uiFrameRate, CVideoEncoder *pVideoEncoder, DWORD dwStartTime);
    bool Stop();
    bool Pause();
    bool Resume();

protected:
    bool Init();
    void Release();
    static unsigned __stdcall CaptureThread(void *pParam);
    void CaptureThreadProc();
    bool CaptureScreen();
    bool DrawBkg(HDC hDC);
    bool DrawCursor(HDC hDC);
    bool MixData(unsigned char *pBkgBuf, DWORD dwBkgBufLen, long iX, long iY, DWORD dwBkgWidth, DWORD dwBkgHeight,
                 unsigned char *pBuf, DWORD dwBufLen, DWORD dwWidth, DWORD dwHeight);

    HANDLE m_hCaptureThread;
    bool m_bStop;
    bool m_bPause;
    clock_t m_nStartTime;
    clock_t m_nCaptureTime;
    long m_nCaptureInterval;

    HWND m_hScrnWnd;
    HDC m_hScrnDC;
    HDC m_hScrnBmpDC;
    HBITMAP m_hScrnBitmap;
    LPVOID m_pScrnBuf;
    BITMAPINFO m_bmpScrnInfo;

    UINT m_uiX;
    UINT m_uiY;
    UINT m_uiWidth;
    UINT m_uiHeight;

    HWND m_hWnd;
    HDC m_hDC;
    HDC m_hBmpDC;
    LPVOID m_pBkgBuf;
    LPVOID m_pBuf;

    CVideoEncoder *m_pVideoEncoder;

    DWORD m_nPausedTime;
    DWORD m_nPausedDuration;
};
