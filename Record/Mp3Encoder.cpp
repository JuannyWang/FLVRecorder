#include "StdAfx.h"
#include "Mp3Encoder.h"


CMp3Encoder::CMp3Encoder(void)
{
    m_pFileOut = NULL;
    m_pMP3Buffer = NULL;
    m_hbeStream = 0;
    m_pMp3SampleHeader = NULL;
    m_lLastSampleTime = 0;
    m_pFlv = NULL;
    m_pOutDataSink = NULL;
}

CMp3Encoder::~CMp3Encoder(void)
{
    End();
}

void CMp3Encoder::SetOutDataSink(IDataSink *pDataSink)
{
    m_pOutDataSink = pDataSink;
}

bool CMp3Encoder::Begin(IDataSink *pDataSink, UINT uiNumChannels, UINT uiSampleRate, UINT uiBitsPerSample, DWORD dwStartTime)
{
    m_lLastSampleTime = 0;

    BE_VERSION Version = {0.};
    beVersion(&Version);

    printf(
        "lame_enc.dll version %u. %02u (%u/%u/%u)\n"
        "lame_enc Engine %u %02u\n"
        "lame_enc homepage at %s\n\n",
        Version.byDLLMajorVersion, Version.byDLLMinorVersion,
        Version.byDay, Version.byMonth, Version.wYear,
        Version.byMajorVersion, Version.byMinorVersion,
        Version.zHomepage
    );

    BE_CONFIG beConfig;
    memset(&beConfig, 0, sizeof(beConfig));
    beConfig.dwConfig = BE_CONFIG_LAME;

    beConfig.format.LHV1.dwStructVersion = 1;
    beConfig.format.LHV1.dwStructSize = sizeof(beConfig);
    beConfig.format.LHV1.dwSampleRate = 11025;
    beConfig.format.LHV1.dwReSampleRate = 0;
    beConfig.format.LHV1.nMode = BE_MP3_MODE_JSTEREO;
    beConfig.format.LHV1.dwBitrate = 16;
    beConfig.format.LHV1.nPreset = LQP_R3MIX;
    beConfig.format.LHV1.dwMpegVersion = MPEG1;
    beConfig.format.LHV1.dwPsyModel = 0;
    beConfig.format.LHV1.dwEmphasis = 0;
    beConfig.format.LHV1.bOriginal = TRUE;
    beConfig.format.LHV1.bWriteVBRHeader = TRUE;

    beConfig.format.LHV1.bNoRes = TRUE;

    DWORD dwMP3Buffer = 0;
    BE_ERR err = beInitStream(&beConfig, &m_dwSamples, &dwMP3Buffer, &m_hbeStream);

    if (err != BE_ERR_SUCCESSFUL)
    {
        fprintf(stderr, "Error opening encoding stream (%lu)", err);
        return false;
    }

    m_pMP3Buffer = new BYTE[dwMP3Buffer];
    memset(m_pMP3Buffer, 0, dwMP3Buffer);

    m_nBufCur = 0;
    m_nBugLength = dwMP3Buffer;
    m_pBuffer = (unsigned char *)malloc(m_nBugLength);

    if (m_pFlv == NULL)
    {
        m_pFlv = new CFlvMp3Packet;
    }
    if (m_pFlv == NULL)
    {
        DWORD dwWrite = 0;
        beDeinitStream(m_hbeStream, m_pMP3Buffer, &dwWrite);
        return false;
    }

    if (m_pFlv)
    {
        UINT nCodecID = 2;
        if (!m_pFlv->WriteHeader(pDataSink, uiNumChannels, uiSampleRate, uiBitsPerSample, nCodecID, dwStartTime))
        {
            return false;
        }
    }

#ifdef MP3ENCODER_FILE
    if (strlen(MP3ENCODER_FILE) > 0)
    {
        m_pFileOut = fopen(MP3ENCODER_FILE, "wb+");
    }
#endif

    return true;
}

bool CMp3Encoder::End()
{
    DWORD dwWrite = 0;
    if (m_hbeStream > 0)
    {
        BE_ERR err = beDeinitStream(m_hbeStream, m_pMP3Buffer, &dwWrite);
        if (err != BE_ERR_SUCCESSFUL)
        {
            beCloseStream(m_hbeStream);
            fprintf(stderr, "beExitStream failed (%lu)", err);
            m_hbeStream = 0;
        }
    }

    if (dwWrite > 0 && m_lLastSampleTime > 0)
    {
        WriteFrame(m_pMP3Buffer, dwWrite, m_lLastSampleTime);
        if (m_nBufCur > 0)
        {
            if(m_pFlv)
            {
                m_pFlv->BeginData();
            }

            int dwWrite = m_nBufCur;

            if (m_pFileOut)
            {
                if (fwrite(m_pBuffer, 1, dwWrite, m_pFileOut) != dwWrite)
                {
                    fprintf(stderr, "Output file write error");
                }
            }

            if (m_pFlv)
            {
                m_pFlv->WriteData(m_pBuffer, dwWrite, m_lLastSampleTime);
            }

            if (m_pFlv)
            {
                m_pFlv->EndData(m_lLastSampleTime);
            }
            m_nBufCur = 0;
        }
    }

    if (m_hbeStream > 0)
    {
        beCloseStream(m_hbeStream);
    }

    if (m_pMP3Buffer)
    {
        delete[] m_pMP3Buffer;
        m_pMP3Buffer = NULL;
    }

    if (m_pFlv)
    {
        m_pFlv->WriteTailer();
    }

    if (m_pFlv)
    {
        delete m_pFlv;
        m_pFlv = NULL;
    }

    m_pOutDataSink = NULL;

    if (m_pFileOut)
    {
        fclose(m_pFileOut);
        m_pFileOut = NULL;
    }

#ifdef MP3ENCODER_FILE
    if (m_hbeStream > 0)
    {
        beWriteInfoTag(m_hbeStream, MP3ENCODER_FILE);
    }
#endif

    m_hbeStream = 0;
    if (m_pMp3SampleHeader)
    {
        delete[] m_pMp3SampleHeader;
        m_pMp3SampleHeader = NULL;
    }

    return true;
}

bool CMp3Encoder::Encode(DWORD lSampleTime, BYTE *pBuffer, long lBufferSize)
{
    if (pBuffer != NULL && lBufferSize > 0 && m_pMP3Buffer != NULL)
    {
        m_lLastSampleTime = lSampleTime;
        unsigned long inputBytes = m_dwSamples * sizeof(short);
        do
        {
            inputBytes = inputBytes >= lBufferSize ? lBufferSize : inputBytes;

            // Encode samples
            DWORD dwWrite = 0;
            DWORD dwRead = inputBytes / sizeof(short);
            PSHORT pWAVBuffer = (PSHORT)pBuffer;
            BE_ERR err = beEncodeChunk(m_hbeStream, dwRead, pWAVBuffer, m_pMP3Buffer, &dwWrite);

            // Check result
            if (err != BE_ERR_SUCCESSFUL)
            {
                beCloseStream(m_hbeStream);
                fprintf(stderr, "beEncodeChunk() failed (%lu)", err);
                return false;
            }

            if (dwWrite > 0)
            {
                if (m_pMp3SampleHeader == NULL)
                {
                    m_pMp3SampleHeader = new BYTE[5];
                    if (m_pMp3SampleHeader == NULL)
                    {
                        return false;
                    }

                    memset(m_pMp3SampleHeader, 0, 5);
                    memcpy(m_pMp3SampleHeader, m_pMP3Buffer, 4);
                }

                WriteFrame(m_pMP3Buffer, dwWrite, lSampleTime);
            }

            lBufferSize -= inputBytes;
            pBuffer += inputBytes;
        }
        while (lBufferSize > inputBytes);
    }

    return true;
}

void CMp3Encoder::WriteFrame(const unsigned char *pBuf, int nSize, DWORD lSampleTime)
{
    assert(m_pBuffer);
    assert(pBuf);

    int nTotal = m_nBufCur + nSize;
    if (nTotal >= m_nBugLength)
    {
        unsigned char *pNew = (unsigned char *)realloc(m_pBuffer, nTotal + 1);
        assert(pNew);
        m_pBuffer = pNew;
        m_nBugLength = nTotal + 1;
    }
    unsigned char *pCur = m_pBuffer + m_nBufCur;
    memcpy(pCur, pBuf, nSize);
    m_nBufCur += nSize;

    while(1)
    {
        BYTE *pTmp = memstr(m_pBuffer, m_nBufCur, m_pMp3SampleHeader);
        if (pTmp == NULL)
        {
            return;
        }

        int nStartPos = (int)(pTmp - m_pBuffer);
        if (nStartPos > 0)
        {
            m_nBufCur -= nStartPos;
            memcpy(m_pBuffer, m_pBuffer + nStartPos, m_nBufCur);
        }

        if (m_nBufCur <= 4)
        {
            return;
        }
        pTmp = memstr(m_pBuffer + 4, m_nBufCur - 4, m_pMp3SampleHeader);
        if (pTmp == NULL)
        {
            return;
        }

        int nEndPos = (int)(pTmp - m_pBuffer);
        if (nEndPos > 0)
        {
            if (m_pFlv)
            {
                m_pFlv->BeginData();
            }

            int dwWrite = nEndPos;

            if (m_pFileOut)
            {
                if (fwrite(m_pBuffer, 1, dwWrite, m_pFileOut) != dwWrite)
                {
                    fprintf(stderr, "Output file write error");
                }
            }

            if (m_pFlv)
            {
                m_pFlv->WriteData(m_pBuffer, dwWrite, lSampleTime);
            }

            if (m_pOutDataSink)
            {
                m_pOutDataSink->OnSample(0, m_pBuffer, dwWrite, lSampleTime);
            }

            if (m_pFlv)
            {
                m_pFlv->EndData(lSampleTime);
            }

            m_nBufCur -= nEndPos;
            memcpy(m_pBuffer, m_pBuffer + nEndPos, m_nBufCur);
        }
    }
}

BYTE *CMp3Encoder::memstr(const BYTE *str1, int nLen1, const BYTE *str2)
{
    if ((NULL == str1) || (NULL == str2) || (nLen1 <= 0))
    {
        return NULL;
    }

    long ls1 = nLen1;
    BYTE *cp = (BYTE *)str1;
    BYTE *s1, *s2;

    if (!*str2)
        return ((BYTE *)str1);

    while(ls1 > 0)
    {
        s1 = cp;
        s2 = (BYTE *)str2;

        int nExcludeIdx = 0;
        while(*s1 && *s2 && ((*s1 == *s2) || nExcludeIdx == 2))
        {
            nExcludeIdx++;
            s1++, s2++;
        }

        if(!*s2)
            return (cp);

        cp++;
        ls1--;
    }

    return (NULL);
}