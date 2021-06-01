//
// Created by yuan on 6/1/21.
//

//
// Created by yuan on 3/29/21.
//

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}
#endif

#include <iostream>
#include <assert.h>
#include "bmlib_runtime.h"
#include "bmcv_api_ext.h"
#include "common.h"

static AVBufferRef *hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;
static FILE *output_file = NULL;

static int avframe_to_rgb_packed(bm_handle_t handle, AVFrame *frame,int idx) {
    int ret = 0;
    bm_image out;
    ret = bm::avframe_to_bmimage(handle, frame, out, false, true);
    assert(BM_SUCCESS == ret);

    bm_image rgb_img;
    bm_image_create(handle, out.height, out.width, FORMAT_BGR_PACKED, DATA_TYPE_EXT_1N_BYTE, &rgb_img);
    ret = bmcv_image_vpp_convert(handle, 1, out, &rgb_img);
    assert(BM_SUCCESS == ret);
    char filepath[32];
    sprintf(filepath, "rgb-%d.bmp", idx);
    bm_image_write_to_bmp(rgb_img, filepath);
    bm_image_destroy(out);
    bm_image_destroy(rgb_img);
    return 0;
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type, int dev_id)
{
    int err = 0;
    char device_name[32];
    sprintf(device_name, "%d", dev_id);
    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      device_name, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    return err;
}
static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt) {
            ctx->hw_frames_ctx = av_hwframe_ctx_alloc(ctx->hw_device_ctx);
            if (!ctx->hw_frames_ctx) {
                return AV_PIX_FMT_NONE;
            }

            AVHWFramesContext *frames_ctx = (AVHWFramesContext*)ctx->hw_frames_ctx->data;
            frames_ctx->format = AV_PIX_FMT_BMCODEC;
            frames_ctx->sw_format = ctx->sw_pix_fmt;

            if (ctx->coded_width > 0)
                frames_ctx->width         = FFALIGN(ctx->coded_width, 32);
            else if (ctx->width > 0)
                frames_ctx->width         = FFALIGN(ctx->width, 32);
            else
                frames_ctx->width         = FFALIGN(1920, 32);

            if (ctx->coded_height > 0)
                frames_ctx->height        = FFALIGN(ctx->coded_height, 32);
            else if (ctx->height > 0)
                frames_ctx->height        = FFALIGN(ctx->height, 32);
            else
                frames_ctx->height        = FFALIGN(1088, 32);

            frames_ctx->initial_pool_size = 0; // Don't prealloc pool.

            int ret = av_hwframe_ctx_init(ctx->hw_frames_ctx);
            if (ret < 0)
                goto failed;

            av_log(ctx, AV_LOG_TRACE, "[%s,%d] Got HW surface format:%s.\n",
                   __func__, __LINE__, av_get_pix_fmt_name(AV_PIX_FMT_BMCODEC));
            return hw_pix_fmt;
        }
    }
failed:
    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

static int decode_write(bm_handle_t handle, AVCodecContext *avctx, AVPacket *packet, int idx)
{
    AVFrame *frame = NULL, *sw_frame = NULL;
    AVFrame *tmp_frame = NULL;
    uint8_t *buffer = NULL;
    int size;
    int ret = 0;
    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        return ret;
    }
    while (1) {
        if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
            fprintf(stderr, "Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            av_frame_free(&sw_frame);
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error while decoding\n");
            goto fail;
        }
        if (frame->format == hw_pix_fmt) {
            /* retrieve data from GPU to CPU */
            if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
                fprintf(stderr, "Error transferring the data to system memory\n");
                goto fail;
            }
            tmp_frame = sw_frame;
            avframe_to_rgb_packed(handle, frame, idx);
        } else
            tmp_frame = frame;
        size = av_image_get_buffer_size((AVPixelFormat)tmp_frame->format, tmp_frame->width,
                                        tmp_frame->height, 1);
        buffer = (uint8_t*)av_malloc(size);
        if (!buffer) {
            fprintf(stderr, "Can not alloc buffer\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        ret = av_image_copy_to_buffer(buffer, size,
                                      (const uint8_t * const *)tmp_frame->data,
                                      (const int *)tmp_frame->linesize, (AVPixelFormat)tmp_frame->format,
                                      tmp_frame->width, tmp_frame->height, 1);
        if (ret < 0) {
            fprintf(stderr, "Can not copy image to buffer\n");
            goto fail;
        }
        if ((ret = fwrite(buffer, 1, size, output_file)) < 0) {
            fprintf(stderr, "Failed to dump raw data.\n");
            goto fail;
        }
        fail:
        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        av_freep(&buffer);
        if (ret < 0)
            return ret;
    }
}
int main(int argc, char *argv[])
{
    AVFormatContext *input_ctx = NULL;
    int video_stream, ret;
    AVStream *video = NULL;
    AVCodecContext *decoder_ctx = NULL;
    AVCodec *decoder = NULL;
    AVPacket packet;
    enum AVHWDeviceType type;

    int i;
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <device type> <input file> <output file>\n", argv[0]);
        return -1;
    }

    av_log_set_level(AV_LOG_DEBUG);

    bm_handle_t  bm_handle;
    bm_dev_request(&bm_handle, 0);

    type = av_hwdevice_find_type_by_name(argv[1]);
    if (type == AV_HWDEVICE_TYPE_NONE) {
        fprintf(stderr, "Device type %s is not supported.\n", argv[1]);
        fprintf(stderr, "Available device types:");
        while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
        fprintf(stderr, "\n");
        return -1;
    }
    /* open the input file */
    if (avformat_open_input(&input_ctx, argv[2], NULL, NULL) != 0) {
        fprintf(stderr, "Cannot open input file '%s'\n", argv[2]);
        return -1;
    }
    if (avformat_find_stream_info(input_ctx, NULL) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        return -1;
    }
    /* find the video stream information */
    ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return -1;
    }
    video_stream = ret;
    for (i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
        if (!config) {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    decoder->name, av_hwdevice_get_type_name(type));
            return -1;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }
    if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
        return AVERROR(ENOMEM);
    video = input_ctx->streams[video_stream];
    if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
        return -1;

    if (hw_decoder_init(decoder_ctx, type, 0) < 0)
        return -1;
    decoder_ctx->get_format  = get_hw_format;


    AVDictionary *opts = nullptr;
    av_dict_set_int(&opts, "sophon_idx", 0, 0);
    if ((ret = avcodec_open2(decoder_ctx, decoder, &opts)) < 0) {
        fprintf(stderr, "Failed to open codec for stream #%u\n", video_stream);
        return -1;
    }
    /* open the file to dump raw data */
    output_file = fopen(argv[3], "w+");
    i = 0;
    /* actual decoding and dump the raw data */
    while (ret >= 0) {
        if ((ret = av_read_frame(input_ctx, &packet)) < 0)
            break;
        if (video_stream == packet.stream_index)
            ret = decode_write(bm_handle, decoder_ctx, &packet, i);
        av_packet_unref(&packet);
        i++;
        if (i > 10) break;
    }
    /* flush the decoder */
    packet.data = NULL;
    packet.size = 0;
    ret = decode_write(bm_handle, decoder_ctx, &packet, i);
    av_packet_unref(&packet);
    if (output_file)
        fclose(output_file);
    avcodec_free_context(&decoder_ctx);
    avformat_close_input(&input_ctx);
    av_buffer_unref(&hw_device_ctx);
    return 0;
}