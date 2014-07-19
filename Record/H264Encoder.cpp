#include "StdAfx.h"
#include "H264Encoder.h"


#define X264Dll "libx264-125.dll"
#define x264_encoder_open_name "x264_encoder_open_125"
#define SWSCALEDLL "swscale-2.dll"

//#define YUV_FILE "testrecord.yuv"
//#define H264_FILE "testrecord.264"

FILE *pYUVFile = NULL;
FILE *pH264File = NULL;


CH264Encoder::CH264Encoder(void) :
    m_px264(NULL),
    m_pSwsContext(NULL)
{
    m_nFrameNum = 0;
    memset(&m_pic, 0, sizeof(x264_picture_t));
    m_pFlv = NULL;
    m_hx264 = NULL;
    mux_buffer = NULL;
    mux_buffer_size = 0;
    m_hSwScale = NULL;
    m_pOutDataSink = NULL;
}

CH264Encoder::~CH264Encoder(void)
{
    End();

    if (m_hSwScale)
    {
        ::FreeLibrary(m_hSwScale);
        m_hSwScale = NULL;
    }
    if (m_hx264)
    {
        ::FreeLibrary(m_hx264);
        m_hx264 = NULL;
    }
}

bool CH264Encoder::Init( UINT uiWidth, UINT uiHeight, UINT uiFrameRate )
{
    assert(uiWidth > 0 && uiHeight > 0);

    char szPath[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, szPath, MAX_PATH) == 0)
    {
        return false;
    }

    char drive[_MAX_DRIVE] = {0};
    char dir[_MAX_DIR] = {0};
    char fname[_MAX_FNAME] = {0};
    char ext[_MAX_EXT] = {0};
    _splitpath(szPath, drive, dir, fname, ext);
    char szX264DllPath[MAX_PATH] = "";

    if (m_hx264 == NULL)
    {
        sprintf(szX264DllPath, "%s%s%s", drive, dir, X264Dll);
        m_hx264 = ::LoadLibraryA(szX264DllPath);
    }
    if (m_hx264 == NULL)
    {
        return false;
    }

    x264_param_default = (f_x264_param_default)::GetProcAddress(m_hx264, "x264_param_default");
    x264_param_apply_profile = (f_x264_param_apply_profile)::GetProcAddress(m_hx264, "x264_param_apply_profile");
    x264_param_default_preset = (f_x264_param_default_preset)::GetProcAddress(m_hx264, "x264_param_default_preset");
    x264_encoder_open = (f_x264_encoder_open)::GetProcAddress(m_hx264, x264_encoder_open_name);
    x264_picture_alloc = (f_x264_picture_alloc)::GetProcAddress(m_hx264, "x264_picture_alloc");
    x264_picture_clean = (f_x264_picture_clean)::GetProcAddress(m_hx264, "x264_picture_clean");
    x264_encoder_encode = (f_x264_encoder_encode)::GetProcAddress(m_hx264, "x264_encoder_encode");
    x264_encoder_close = (f_x264_encoder_close)::GetProcAddress(m_hx264, "x264_encoder_close");
    x264_nal_encode = (f_x264_nal_encode)::GetProcAddress(m_hx264, "x264_nal_encode");

    if (x264_param_default == NULL
            || x264_param_apply_profile == NULL
            || x264_param_default_preset == NULL
            || x264_encoder_open == NULL
            || x264_picture_alloc == NULL
            || x264_picture_clean == NULL
            || x264_encoder_encode == NULL
            || x264_encoder_close == NULL
            || x264_nal_encode == NULL)
    {
        return false;
    }

    if (m_hSwScale == NULL)
    {
        char szSWScaleDllPath[MAX_PATH] = "";
        sprintf(szSWScaleDllPath, "%s%s%s", drive, dir, SWSCALEDLL);
        m_hSwScale = ::LoadLibraryA(szSWScaleDllPath);
    }

    if (m_hSwScale == NULL)
    {
        return false;
    }

    sws_getContext = (f_sws_getContext)::GetProcAddress(m_hSwScale, "sws_getContext");
    sws_scale = (f_sws_scale)::GetProcAddress(m_hSwScale, "sws_scale");
    sws_freeContext = (f_sws_freeContext)::GetProcAddress(m_hSwScale, "sws_freeContext");

    if (sws_getContext == NULL
            || sws_scale == NULL
            || sws_freeContext == NULL)
    {
        return false;
    }

    x264_param_default(&m_x264_param);

    x264_param_default_preset(&m_x264_param, x264_preset_names[0], x264_tune_names[7]);
    //x264_param_apply_profile(&m_x264_param, x264_profile_names[1]);

    m_x264_param.i_width = uiWidth;
    m_x264_param.i_height = uiHeight;
    m_x264_param.i_fps_num = uiFrameRate;
    m_x264_param.b_annexb = 1;

    if ((m_px264 = x264_encoder_open(&m_x264_param)) == NULL)
    {
        return false;
    }

    mux_buffer_size = uiWidth * uiHeight * 4;
    mux_buffer = (uint8_t *)malloc(mux_buffer_size);
    memset(mux_buffer, 0, mux_buffer_size);
    if (mux_buffer == NULL)
    {
        return false;
    }

    return true;
}

void CH264Encoder::SetOutDataSink( IDataSink *pDataSink )
{
    m_pOutDataSink = pDataSink;
}

bool CH264Encoder::Begin( IDataSink *pDataSink, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, DWORD dwStartTime )
{
    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;

    bool ret = true;
    do
    {
        if (!Init(uiWidth, uiHeight, uiFrameRate))
        {
            ret = false;
            break;
        }

        if (m_pFlv == NULL)
        {
            m_pFlv = new CFlvH264Packet;
        }
        if (m_pFlv == NULL)
        {
            ret = false;
            break;
        }

        UINT nCodecID = 7;
        if (!m_pFlv->WriteHeader(pDataSink, uiWidth, uiHeight, nCodecID, dwStartTime))
        {
            ret = false;
            break;
        }

        x264_picture_alloc(&m_pic, X264_CSP_YV12, m_x264_param.i_width, m_x264_param.i_height);

        m_pic.img.i_csp = X264_CSP_YV12;
        m_pic.i_type = X264_TYPE_AUTO;
        m_pic.i_qpplus1 = 0;

        assert(m_pSwsContext == NULL);
        m_pSwsContext = sws_getContext(uiWidth, uiHeight, PIX_FMT_RGB24, m_x264_param.i_width, m_x264_param.i_height,
                                       PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        if (m_pSwsContext == NULL)
        {
            ret = false;
            break;
        }
    }
    while (0);

    if (!ret)
    {
        End();
    }

    m_nFrameNum = 0;

#ifdef YUV_FILE
    if (pYUVFile == NULL)
    {
        pYUVFile = fopen(YUV_FILE, "wb");
    }
#endif

#ifdef H264_FILE
    if (pH264File == NULL)
    {
        pH264File = fopen(H264_FILE, "wb");
    }
#endif

    return ret;
}

bool CH264Encoder::End()
{
    if (m_pic.img.plane[0])
    {
        x264_picture_clean(&m_pic);
    }

    if (m_pSwsContext)
    {
        sws_freeContext(m_pSwsContext);
        m_pSwsContext = NULL;
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

    if (mux_buffer)
    {
        free(mux_buffer);
    }
    mux_buffer = NULL;
    mux_buffer_size = 0;

    if (m_px264)
    {
        x264_encoder_close(m_px264);
        m_px264 = NULL;
    }

    m_pOutDataSink = NULL;

#ifdef YUV_FILE
    if (pYUVFile)
    {
        fclose(pYUVFile);
        pYUVFile = NULL;
    }
#endif

#ifdef H264_FILE
    if (pH264File)
    {
        fclose(pH264File);
        pH264File = NULL;
    }
#endif

    return true;
}

bool CH264Encoder::EncodeFrame( DWORD lSampleTime, x264_t *h, x264_picture_t *pic )
{
    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal, i;
    int i_file = 0;

    if (x264_encoder_encode(h, &nal, &i_nal, pic, &pic_out) < 0)
    {
        return false;
    }

    if (i_nal <= 0)
    {
        return false;
    }

    if (m_pFlv)
    {
        m_pFlv->BeginData();
    }

    for (i = 0; i < i_nal; i++)
    {
        if (pH264File)
        {
            fwrite(nal[i].p_payload, 1, nal[i].i_payload, pH264File);
        }

        int i_size;
        int i_payload = nal[i].i_payload;
        uint8_t *p_payload = nal[i].p_payload;
        if (m_pFlv && i_payload > 0)
        {
            if (i_payload >= mux_buffer_size)
            {
                unsigned char *pNew = (unsigned char *)realloc(mux_buffer, i_payload + 1);
                assert(pNew);
                if (pNew == NULL)
                {
                    break;
                }
                mux_buffer = pNew;
                mux_buffer_size = i_payload + 1;
            }

            i_size = 0;
            memset(mux_buffer + i_size, 0, 3);
            i_size += 3;
            memset(mux_buffer + i_size, 1, 1);
            i_size += 1;

            int b_long_startcode = nal[i].b_long_startcode;
            if (b_long_startcode)
            {
                memcpy(mux_buffer + i_size, nal[i].p_payload + 4, nal[i].i_payload - 4);
                i_size += (nal[i].i_payload - 4);
            }
            else
            {
                memcpy(mux_buffer + i_size, nal[i].p_payload + 3, nal[i].i_payload - 3);
                i_size += (nal[i].i_payload - 3);
            }

            m_pFlv->WriteData(mux_buffer, i_size, lSampleTime);

            if (m_pOutDataSink)
            {
                m_pOutDataSink->OnSample(0, mux_buffer, i_size, lSampleTime);
            }
        }
    }

    if (m_pFlv)
    {
        m_pFlv->EndData(lSampleTime);
    }

    return true;
}

bool CH264Encoder::Encode( DWORD lSampleTime, unsigned char *pData, int nSize )
{
    ++m_nFrameNum;
    m_pic.i_pts = (int64_t)m_nFrameNum * m_x264_param.i_fps_den;

    unsigned char *pDataT = pData;
    int nSrcWidth = m_uiWidth;

    uint8_t *src[3] = {pDataT, NULL, NULL};
    int srcStride[4] = {nSrcWidth * 3 + (((nSrcWidth * 3 % 4) != 0) ? (4 - nSrcWidth * 3 % 4) : 0), 0, 0, 0};
    int dstStride[4] = {m_x264_param.i_width, m_x264_param.i_width / 2, m_x264_param.i_width / 2, 0};
    assert(m_pSwsContext);
    sws_scale(m_pSwsContext, src, srcStride, 0, m_uiHeight, m_pic.img.plane, dstStride);

    if (pYUVFile)
    {
        int iYSize = m_pic.img.i_stride[0] * m_uiHeight;
        int iRet = fwrite(m_pic.img.plane[0], sizeof(uint8_t), iYSize, pYUVFile);
        int iVSize = m_pic.img.i_stride[1] * m_uiHeight / 2;
        iRet = fwrite(m_pic.img.plane[1], sizeof(uint8_t), iVSize, pYUVFile);
        int iUSize = m_pic.img.i_stride[2] * m_uiHeight / 2;
        iRet = fwrite(m_pic.img.plane[2], sizeof(uint8_t), iUSize, pYUVFile);
    }

    bool bRet = EncodeFrame(lSampleTime, m_px264, &m_pic);
    return bRet;
}
