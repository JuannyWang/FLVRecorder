#pragma once

#include "VideoEncoder.h"
#include "FlvH264Packet.h"

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#endif

extern "C"
{
#include <x264.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
};


typedef void (*f_x264_param_default)( x264_param_t * );
typedef void (*f_x264_param_apply_profile)( x264_param_t *, const char *profile );
typedef void (*f_x264_param_default_preset)( x264_param_t *, const char *preset, const char *tune );
typedef x264_t *(*f_x264_encoder_open)( x264_param_t * );
typedef int (*f_x264_picture_alloc)( x264_picture_t *pic, int i_csp, int i_width, int i_height );
typedef void (*f_x264_picture_clean)( x264_picture_t *pic );
typedef int (*f_x264_encoder_encode)( x264_t *, x264_nal_t **pp_nal, int *pi_nal, x264_picture_t *pic_in, x264_picture_t *pic_out );
typedef void (*f_x264_encoder_close)( x264_t * );
typedef void (*f_x264_nal_encode)( x264_t *h, uint8_t *dst, x264_nal_t *nal );

typedef struct SwsContext *(*f_sws_getContext)(int srcW, int srcH, enum PixelFormat srcFormat, int dstW, int dstH, enum PixelFormat dstFormat, int flags, SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param);
typedef int (*f_sws_scale)(struct SwsContext *c, const uint8_t *const srcSlice[], const int srcStride[], int srcSliceY, int srcSliceH, uint8_t *const dst[], const int dstStride[]);
typedef void (*f_sws_freeContext)(struct SwsContext *swsContext);

class CH264Encoder : public CVideoEncoder
{
public:
    CH264Encoder(void);
    ~CH264Encoder(void);

public:
    void SetOutDataSink(IDataSink *pDataSink);
    bool Begin(IDataSink *pDataSink, UINT uiWidth, UINT uiHeight, UINT uiFrameRate, DWORD dwStartTime);
    bool End();
    bool Encode(DWORD lSampleTime, unsigned char *pData, int nSize);

private:
    bool Init(UINT uiWidth, UINT uiHeight, UINT uiFrameRate);
    bool EncodeFrame(DWORD lSampleTime, x264_t *h, x264_picture_t *pic);

private:
    x264_t *m_px264;
    x264_param_t m_x264_param;
    int m_nFrameRate;
    x264_picture_t m_pic;
    CFlvH264Packet *m_pFlv;
    unsigned int m_nFrameNum;
    UINT m_uiWidth, m_uiHeight;
    IDataSink *m_pOutDataSink;

    struct SwsContext *m_pSwsContext;
    uint8_t *mux_buffer;
    int mux_buffer_size;

    HINSTANCE m_hx264;
    f_x264_param_default x264_param_default;
    f_x264_param_apply_profile x264_param_apply_profile;
    f_x264_param_default_preset x264_param_default_preset;
    f_x264_encoder_open x264_encoder_open;
    f_x264_picture_alloc x264_picture_alloc;
    f_x264_picture_clean x264_picture_clean;
    f_x264_encoder_encode x264_encoder_encode;
    f_x264_encoder_close x264_encoder_close;
    f_x264_nal_encode x264_nal_encode;

    HINSTANCE m_hSwScale;
    f_sws_getContext sws_getContext;
    f_sws_scale sws_scale;
    f_sws_freeContext sws_freeContext;
};
