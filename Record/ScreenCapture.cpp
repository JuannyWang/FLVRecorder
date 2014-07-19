#include "StdAfx.h"
#include "ScreenCapture.h"
#include "H264Encoder.h"
#include <process.h>


CScreenCapture::CScreenCapture(void) :
    m_hCaptureThread(NULL),
    m_bStop(false),
    m_bPause(false),
    m_pVideoEncoder(NULL)
{
    m_hSrcnWnd = NULL;
    m_hSrcnDC = NULL;
    m_hSrcnBmpDC = NULL;
    m_hSrcnBitmap = NULL;
    m_pSrcnBuf = NULL;
    m_nCaptureTime = 0;
    m_nCaptureInterval = 0;
    m_nPausedDuration = 0;
    m_nPausedTime = 0;
    m_uiX = 0;
    m_uiY = 0;
    m_uiWidth = 0;
    m_uiHeight = 0;
}

CScreenCapture::~CScreenCapture(void)
{
    Stop();
}

bool CScreenCapture::Start( IDataSink *pDataSink, UINT uiFrameRate, CVideoEncoder *pVideoEncoder, DWORD dwStartTime )
{
    assert(pVideoEncoder);

    m_hSrcnWnd = NULL;
    if (pVideoEncoder == NULL || uiFrameRate <= 0)
    {
        return false;
    }

    if (!Init())
    {
        return false;
    }

    m_pVideoEncoder = pVideoEncoder;

    m_nCaptureInterval = (long)(1000 / uiFrameRate);
    if (!m_pVideoEncoder->Begin(pDataSink, m_uiWidth, m_uiHeight, uiFrameRate, dwStartTime))
    {
        return false;
    }

    m_bStop = false;
    m_bPause = false;
    unsigned uThreadId = 0;
    m_hCaptureThread = (HANDLE)_beginthreadex(NULL, 0, &CScreenCapture::CaptureThread, this, 0, &uThreadId);
    if (m_hCaptureThread == NULL)
    {
        return false;
    }

    return true;
}

bool CScreenCapture::Start2( IDataSink *pDataSink, HWND hWnd, UINT uiX, UINT uiY, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, CVideoEncoder *pVideoEncoder, DWORD dwStartTime )
{
    assert(pVideoEncoder);
    assert(uiFrameRate > 0);

    if (hWnd == NULL || !::IsWindow(hWnd) || hWnd != ::GetDesktopWindow())
    {
        return false;
    }

    if (pVideoEncoder == NULL || uiFrameRate <= 0)
    {
        return false;
    }

    if (uiX < 0 || uiY < 0)
    {
        return false;
    }

    UINT uiSrcnWidth = ::GetSystemMetrics(SM_CXSCREEN);
    UINT uiSrcnHeight = ::GetSystemMetrics(SM_CYSCREEN);

    if (uiX >= uiSrcnWidth)
    {
        return false;
    }

    if (uiY >= uiSrcnHeight)
    {
        return false;
    }

    m_hSrcnWnd = hWnd;
    m_uiX = uiX;
    m_uiY = uiY;
    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;

    if (uiWidth <= 0 || uiHeight <= 0)
    {
        m_uiX = 0;
        m_uiY = 0;
        m_uiWidth = uiSrcnWidth;
        m_uiHeight = uiSrcnHeight;
    }

    if (uiWidth % 2 == 1)
    {
        uiWidth -= 1;
        if (uiWidth <= 0)
        {
            uiWidth += 2;
        }
    }
    if (uiHeight % 2 == 1)
    {
        uiHeight -= 1;
        if (uiHeight <= 0)
        {
            uiHeight += 2;
        }
    }

    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;

    if (m_uiX + m_uiWidth > uiSrcnWidth)
    {
        m_uiWidth = uiSrcnWidth - m_uiX;
    }
    if (m_uiY + m_uiHeight > uiSrcnHeight)
    {
        m_uiHeight = uiSrcnHeight - m_uiY;
    }

    if (!Init())
    {
        return false;
    }

    m_pVideoEncoder = pVideoEncoder;

    m_nCaptureInterval = (long)(1000 / uiFrameRate);
    if (!m_pVideoEncoder->Begin(pDataSink, m_uiWidth, m_uiHeight, uiFrameRate, dwStartTime))
    {
        Release();
        return false;
    }

    m_bStop = false;
    m_bPause = false;
    unsigned uThreadId = 0;
    m_hCaptureThread = (HANDLE)_beginthreadex(NULL, 0, &CScreenCapture::CaptureThread, this, 0, &uThreadId);
    if (m_hCaptureThread == NULL)
    {
        Release();
        return false;
    }

    return true;
}

bool CScreenCapture::Stop()
{
    m_bStop = true;
    m_bPause = true;
    if (m_hCaptureThread)
    {
        DWORD dwRet = WaitForSingleObject(m_hCaptureThread, 3000);
        switch (dwRet)
        {
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            break;
        case WAIT_ABANDONED:
            break;
        }

        CloseHandle(m_hCaptureThread);
        m_hCaptureThread = NULL;
    }

    Release();
    return true;
}

bool CScreenCapture::Pause()
{
    m_nPausedTime = GetTickCount();

    m_bPause = true;
    return true;
}

bool CScreenCapture::Resume()
{
    m_nPausedDuration += (GetTickCount() - m_nPausedTime);

    m_bPause = false;
    return true;
}

bool CScreenCapture::Init()
{
    if (m_hSrcnWnd == NULL)
    {
        m_hSrcnWnd = ::GetDesktopWindow();
        m_uiX = 0;
        m_uiY = 0;
        m_uiWidth = ::GetSystemMetrics(SM_CXSCREEN);
        m_uiHeight = ::GetSystemMetrics(SM_CYSCREEN);
    }

    m_hSrcnDC = ::GetDC(m_hSrcnWnd);

    if ((m_hSrcnBmpDC = ::CreateCompatibleDC(m_hSrcnDC)) == NULL)
    {
        return false;
    }

    if ((m_hSrcnBitmap = ::CreateCompatibleBitmap(m_hSrcnDC, m_uiWidth, m_uiHeight)) == NULL)
    {
        return false;
    }

    ZeroMemory(&m_bmpInfo, sizeof(BITMAPINFO));
    m_bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ::GetDIBits(m_hSrcnBmpDC, m_hSrcnBitmap, 0, 0, NULL, &m_bmpInfo, DIB_RGB_COLORS);

    m_bmpInfo.bmiHeader.biCompression = BI_RGB;
    m_bmpInfo.bmiHeader.biBitCount = 24;
    m_bmpInfo.bmiHeader.biSizeImage = m_bmpInfo.bmiHeader.biWidth * abs(m_bmpInfo.bmiHeader.biHeight)
                                      * (m_bmpInfo.bmiHeader.biBitCount + 7) / 8;
    m_bmpInfo.bmiHeader.biHeight = -m_bmpInfo.bmiHeader.biHeight;

    if ((m_pSrcnBuf = malloc(m_bmpInfo.bmiHeader.biSizeImage)) == NULL)
    {
        return false;
    }

    m_nStartTime = GetTickCount();
    m_nPausedTime = 0;
    m_nPausedDuration = 0;

    return true;
}

void CScreenCapture::Release()
{
    if (m_pSrcnBuf)
    {
        free(m_pSrcnBuf);
        m_pSrcnBuf = NULL;
    }

    if (m_hSrcnBitmap)
    {
        ::DeleteObject(m_hSrcnBitmap);
        m_hSrcnBitmap = NULL;
    }

    if (m_hSrcnBmpDC)
    {
        ::DeleteDC(m_hSrcnBmpDC);
        m_hSrcnBmpDC = NULL;
    }

    if (m_hSrcnWnd && m_hSrcnDC)
    {
        ::ReleaseDC(m_hSrcnWnd, m_hSrcnDC);
        m_hSrcnWnd = NULL;
        m_hSrcnDC = NULL;
    }

    if (m_pVideoEncoder)
    {
        m_pVideoEncoder->End();
    }
    m_pVideoEncoder = NULL;
}

unsigned __stdcall CScreenCapture::CaptureThread( void *pParam )
{
    CScreenCapture *pThis = static_cast<CScreenCapture *>(pParam);
    if (pThis)
    {
        pThis->CaptureThreadProc();
    }

    return 0;
}

void CScreenCapture::CaptureThreadProc()
{
    while(!m_bStop)
    {
        if (m_bPause)
        {
            Sleep(1);
            continue;
        }

        if (!CaptureScreen())
        {
            Sleep(1);
            continue;
        }

        clock_t cur = clock();
        long nCaptureInterval = m_nCaptureInterval;
        if (m_nCaptureTime > 0)
        {
            nCaptureInterval = nCaptureInterval - (cur - m_nCaptureTime);
        }
        m_nCaptureTime = cur;

        if (nCaptureInterval < 0)
        {
            nCaptureInterval = 1;
        }

        Sleep(nCaptureInterval);
    }
}

bool CScreenCapture::CaptureScreen()
{
    HBITMAP hOldBitmap = (HBITMAP)::SelectObject(m_hSrcnBmpDC, m_hSrcnBitmap);
    if (!::BitBlt(m_hSrcnBmpDC, 0, 0, m_uiWidth, m_uiHeight, m_hSrcnDC, m_uiX, m_uiY, SRCCOPY))
    {
        return false;
    }

    if (!DrawCursor(m_hSrcnBmpDC))
    {
    }

    ::GetDIBits(m_hSrcnBmpDC, m_hSrcnBitmap, 0, m_uiHeight, m_pSrcnBuf, &m_bmpInfo, DIB_RGB_COLORS);
    ::SelectObject(m_hSrcnBmpDC, hOldBitmap);

    UINT nSize = m_bmpInfo.bmiHeader.biSizeImage;
    LPVOID pDest = m_pSrcnBuf;
    if (m_pVideoEncoder)
    {
        m_pVideoEncoder->Encode(GetTickCount() - m_nPausedDuration, (unsigned char *)pDest, nSize);
    }

#if 0
    FILE *pFile = NULL;
    BITMAPFILEHEADER bmpFileHeader;
    char szName[256] = "";
    sprintf(szName, "C:\\desktop.bmp");
    if ((pFile = fopen(szName, "wb")) == NULL)
    {
        return false;
    }

    bmpFileHeader.bfReserved1 = 0;
    bmpFileHeader.bfReserved2 = 0;
    bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + m_bmpInfo.bmiHeader.biSizeImage;
    bmpFileHeader.bfType = 'MB';
    bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, pFile);
    fwrite(&m_bmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER), 1, pFile);
    fwrite(pDest, nSize, 1, pFile);
    if (pFile)
        fclose(pFile);
#endif

    return true;
}

bool CScreenCapture::DrawCursor( HDC hDC )
{
    BOOL bRet = TRUE;
    DWORD dwErr;
    CURSORINFO ci;
    ICONINFO ii;
    memset(&ci, 0, sizeof(CURSORINFO));
    memset(&ii, 0, sizeof(ICONINFO));

    do
    {
        ci.cbSize = sizeof(CURSORINFO);
        bRet = ::GetCursorInfo(&ci);
        if (!bRet)
        {
            dwErr = GetLastError();
            break;
        }

        bRet = ::GetIconInfo(ci.hCursor, &ii);
        if (!bRet)
        {
            dwErr = GetLastError();
            break;
        }

        bRet = ::DrawIcon(hDC, ci.ptScreenPos.x - ii.xHotspot - m_uiX, ci.ptScreenPos.y - ii.yHotspot - m_uiY, ci.hCursor);
        if (!bRet)
        {
            dwErr = GetLastError();
            break;
        }
    }
    while (0);

    if (ii.hbmMask)
    {
        ::DeleteObject(ii.hbmMask);
    }

    if (ii.hbmColor)
    {
        ::DeleteObject(ii.hbmColor);
    }

    return bRet ? true : false;
}
