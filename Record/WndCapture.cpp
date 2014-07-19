#include "StdAfx.h"
#include "WndCapture.h"
#include "H264Encoder.h"
#include <process.h>


CWndCapture::CWndCapture(void) :
    m_hCaptureThread(NULL),
    m_bStop(false),
    m_bPause(false),
    m_pVideoEncoder(NULL)
{
    m_hScrnWnd = NULL;
    m_hScrnDC = NULL;
    m_hScrnBmpDC = NULL;
    m_hScrnBitmap = NULL;
    m_pScrnBuf = NULL;
    m_nCaptureTime = 0;
    m_nCaptureInterval = 0;
    m_nPausedDuration = 0;
    m_nPausedTime = 0;
    m_uiWidth = 0;
    m_uiHeight = 0;

    m_hWnd = NULL;
    m_hDC = NULL;
    m_hBmpDC = NULL;
    m_pBuf = NULL;
    m_pBkgBuf = NULL;
}

CWndCapture::~CWndCapture(void)
{
    Stop();
}

bool CWndCapture::Start( IDataSink *pDataSink, HWND hWnd, UINT uiFrameRate, CVideoEncoder *pVideoEncoder, DWORD dwStartTime )
{
    assert(pVideoEncoder);
    assert(uiFrameRate > 0);

    if(hWnd == NULL || !::IsWindow(hWnd))
    {
        return false;
    }

    if(pVideoEncoder == NULL || uiFrameRate <= 0)
    {
        return false;
    }

    m_hWnd = hWnd;
    m_uiWidth = ::GetSystemMetrics(SM_CXSCREEN);
    m_uiHeight = ::GetSystemMetrics(SM_CYSCREEN);

    if (!Init())
    {
        Release();
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
    unsigned uThreadID = 0;
    m_hCaptureThread = (HANDLE)_beginthreadex(NULL, 0, &CWndCapture::CaptureThread, this, 0, &uThreadID);
    if (m_hCaptureThread == NULL)
    {
        Release();
        return false;
    }

    return true;
}

bool CWndCapture::Stop()
{
    m_bStop = true;
    m_bPause = true;

    if (m_hCaptureThread)
    {
        DWORD dwRet = WaitForSingleObject(m_hCaptureThread, 3000);
        switch(dwRet)
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

bool CWndCapture::Pause()
{
    m_nPausedTime = GetTickCount();

    m_bPause = true;
    return true;
}

bool CWndCapture::Resume()
{
    m_nPausedDuration += (GetTickCount() - m_nPausedTime);

    m_bPause = false;
    return true;
}

bool CWndCapture::Init()
{
    m_hScrnDC = ::GetDC(m_hScrnWnd);

    if ((m_hScrnBmpDC = ::CreateCompatibleDC(m_hScrnDC)) == NULL)
    {
        return false;
    }

    if ((m_hScrnBitmap = ::CreateCompatibleBitmap(m_hScrnDC, m_uiWidth, m_uiHeight)) == NULL)
    {
        return false;
    }

    ZeroMemory(&m_bmpScrnInfo, sizeof(BITMAPINFO));
    m_bmpScrnInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    GetDIBits(m_hScrnBmpDC, m_hScrnBitmap, 0, 0, NULL, &m_bmpScrnInfo, DIB_RGB_COLORS);

    m_bmpScrnInfo.bmiHeader.biCompression = BI_RGB;
    m_bmpScrnInfo.bmiHeader.biBitCount = 24;
    m_bmpScrnInfo.bmiHeader.biSizeImage = m_bmpScrnInfo.bmiHeader.biWidth * abs(m_bmpScrnInfo.bmiHeader.biHeight)
                                          * (m_bmpScrnInfo.bmiHeader.biBitCount);
    m_bmpScrnInfo.bmiHeader.biHeight = -m_bmpScrnInfo.bmiHeader.biHeight;

    if ((m_pScrnBuf = malloc(m_bmpScrnInfo.bmiHeader.biSizeImage)) == NULL)
    {
        return false;
    }

    if ((m_pBkgBuf = malloc(m_bmpScrnInfo.bmiHeader.biSizeImage)) == NULL)
    {
        return false;
    }

    m_hDC = ::GetWindowDC(m_hWnd);

    if ((m_hBmpDC = ::CreateCompatibleDC(m_hDC)) == NULL)
    {
        return false;
    }

    if ((m_pBuf = malloc(m_bmpScrnInfo.bmiHeader.biSizeImage)) == NULL)
    {
        return false;
    }

    if (!DrawBkg(m_hScrnBmpDC))
    {
        return false;
    }

    m_nStartTime = GetTickCount();
    m_nPausedTime = 0;
    m_nPausedDuration = 0;

    return true;
}

void CWndCapture::Release()
{
    if (m_pScrnBuf)
    {
        free(m_pScrnBuf);
        m_pScrnBuf = NULL;
    }

    if (m_hScrnBitmap)
    {
        ::DeleteObject(m_hScrnBitmap);
        m_hScrnBitmap = NULL;
    }

    if (m_hScrnBmpDC)
    {
        ::DeleteDC(m_hScrnBmpDC);
        m_hScrnBmpDC = NULL;
    }

    if (m_hScrnWnd && m_hScrnDC)
    {
        ::ReleaseDC(m_hScrnWnd, m_hScrnDC);
    }
    m_hScrnWnd = NULL;
    m_hScrnDC = NULL;

    if (m_pBkgBuf)
    {
        free(m_pBkgBuf);
        m_pBkgBuf = NULL;
    }

    if (m_pBuf)
    {
        free(m_pBuf);
        m_pBuf = NULL;
    }

    if (m_hBmpDC)
    {
        DeleteDC(m_hBmpDC);
        m_hBmpDC = NULL;
    }

    if (m_hWnd && m_hDC)
    {
        ReleaseDC(m_hWnd, m_hDC);
    }
    m_hWnd = NULL;
    m_hDC = NULL;

    if (m_pVideoEncoder)
    {
        m_pVideoEncoder->End();
    }
    m_pVideoEncoder = NULL;
}

unsigned __stdcall CWndCapture::CaptureThread( void *pParam )
{
    CWndCapture *pThis = static_cast<CWndCapture *>(pParam);
    if (pThis)
    {
        pThis->CaptureThreadProc();
    }

    return 0;
}

void CWndCapture::CaptureThreadProc()
{
    while(!m_bStop)
    {
        if(m_bPause)
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

bool CWndCapture::CaptureScreen()
{
    if (m_hWnd == NULL || !::IsWindow(m_hWnd))
    {
        return false;
    }

    RECT rect;
    if (!::GetWindowRect(m_hWnd, &rect))
    {
        return false;
    }

    if (rect.left < 0)
    {
        rect.left = 0;
    }
    if (rect.top < 0)
    {
        rect.top = 0;
    }

    UINT uiWidth = (rect.right - rect.left);
    UINT uiHeight = (rect.bottom - rect.top);
    if (rect.left + uiWidth > m_uiWidth)
    {
        uiWidth = m_uiWidth - rect.left;
    }
    if (rect.top + uiHeight > m_uiHeight)
    {
        uiHeight = m_uiHeight - rect.top;
    }

    bool bResult = true;
    HBITMAP hOldBitmap = NULL;
    HBITMAP hBitmap = NULL;
    BITMAPINFO bmpInfo;
    ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));

    do
    {
        if ((hBitmap = ::CreateCompatibleBitmap(m_hDC, uiWidth, uiHeight)) == NULL)
        {
            bResult = false;
            break;
        }

        bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        int iLine = ::GetDIBits(m_hBmpDC, hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);
        bmpInfo.bmiHeader.biCompression = BI_RGB;
        bmpInfo.bmiHeader.biBitCount = 24;
        bmpInfo.bmiHeader.biSizeImage = bmpInfo.bmiHeader.biWidth * abs(bmpInfo.bmiHeader.biHeight)
                                        * (bmpInfo.bmiHeader.biBitCount + 7) / 8;
        bmpInfo.bmiHeader.biHeight = -bmpInfo.bmiHeader.biHeight;
        bool bRetT = (iLine == 1);
        if (!bRetT)
        {
            bResult = false;
            break;
        }

        HBITMAP hOldBitmap = (HBITMAP)::SelectObject(m_hBmpDC, hBitmap);

        if (!::BitBlt(m_hBmpDC, 0, 0, uiWidth, uiHeight, m_hDC, 0, 0, SRCCOPY))
        {
            bResult = false;
            break;
        }

        m_uiX = rect.left;
        m_uiY = rect.top;
        if (!DrawCursor(m_hBmpDC))
        {
        }

        iLine = ::GetDIBits(m_hBmpDC, hBitmap, 0, uiHeight, m_pBuf, &bmpInfo, DIB_RGB_COLORS);
        bRetT = (iLine == uiHeight);
        if (!bRetT)
        {
            bResult = false;
            break;
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
        bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpInfo.bmiHeader.biSizeImage;
        bmpFileHeader.bfType = 'MB';
        bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, pFile);
        fwrite(&bmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER), 1, pFile);
        fwrite(m_pBuf, bmpInfo.bmiHeader.biSizeImage, 1, pFile);
        if(pFile)
            fclose(pFile);
#endif

    }
    while (0);

    if (hOldBitmap)
    {
        SelectObject(m_hBmpDC, hOldBitmap);
    }

    if (hBitmap)
    {
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }

    if (bResult)
    {
        memcpy(m_pBkgBuf, m_pScrnBuf, m_bmpScrnInfo.bmiHeader.biSizeImage);

        MixData((unsigned char *)m_pBkgBuf, m_bmpScrnInfo.bmiHeader.biSizeImage, rect.left, rect.top, m_bmpScrnInfo.bmiHeader.biWidth,
                abs(m_bmpScrnInfo.bmiHeader.biHeight), (unsigned char *)m_pBuf, bmpInfo.bmiHeader.biSizeImage, bmpInfo.bmiHeader.biWidth,
                abs(bmpInfo.bmiHeader.biHeight));

        if (m_pVideoEncoder)
        {
            m_pVideoEncoder->Encode(GetTickCount() - m_nPausedDuration, (unsigned char *)m_pBkgBuf, m_bmpScrnInfo.bmiHeader.biSizeImage);
        }
    }

    return bResult;
}

bool CWndCapture::DrawBkg( HDC hDC )
{
    HBITMAP hOldBitmap = (HBITMAP)::SelectObject(m_hScrnBmpDC, m_hScrnBitmap);

    HBRUSH hOldBrush = NULL;
    HBRUSH hBrush = ::CreateSolidBrush(RGB(0, 128, 192));
    if (hBrush)
    {
        hOldBrush = (HBRUSH)::SelectObject(m_hScrnBmpDC, hBrush);
    }

    ::Rectangle(hDC, 0, 0, m_uiWidth, m_uiHeight);

    if (hOldBrush)
    {
        ::SelectObject(m_hScrnBmpDC, hOldBrush);
    }

    if (hBrush)
    {
        ::DeleteObject(hBrush);
        hBrush = NULL;
    }

    bool bRet = true;

    do
    {
        int iLine = GetDIBits(m_hScrnBmpDC, m_hScrnBitmap, 0, m_uiHeight, m_pScrnBuf, &m_bmpScrnInfo, DIB_RGB_COLORS);
        BOOL bRetT = (iLine == m_uiHeight);
        if (!bRetT)
        {
            bRetT = false;
            break;
        }
    }
    while (0);

    ::SelectObject(m_hScrnBmpDC, hOldBitmap);
    return bRet;
}

bool CWndCapture::DrawCursor( HDC hDC )
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

bool CWndCapture::MixData( unsigned char *pBkgBuf, DWORD dwBkgBufLen, long iX, long iY, DWORD dwBkgWidth, DWORD dwBkgHeight, unsigned char *pBuf, DWORD dwBufLen, DWORD dwWidth, DWORD dwHeight )
{
    if ((dwBufLen > dwBkgBufLen) || (pBkgBuf == NULL))
    {
        return false;
    }

    DWORD dwPixWidth = dwWidth * 3 + (((dwWidth * 3 % 4) != 0) ? (4 - dwWidth * 3 % 4) : 0);
    if (dwBufLen > 0 && pBuf != NULL)
    {
        for (DWORD i = 0; i < dwHeight; i++)
        {
            for (DWORD j = 0; j < dwPixWidth; j++)
            {
                DWORD lBkgBufPos = (i + iY) * dwBkgWidth * 3 + (j + iX * 3);
                DWORD lBufPos = i * dwPixWidth + j;
                pBkgBuf[lBkgBufPos] = pBuf[lBufPos];
            }
        }
    }

    return true;
}
