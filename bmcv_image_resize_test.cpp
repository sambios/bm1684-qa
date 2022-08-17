//
// Created by yuan on 8/17/22.
//
#include <memory.h>
#include <iostream>
#include <memory>
#include "bmcv_api_ext.h"
#include "bm_wrapper.hpp"
#include "common.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
}


class VideoDec_FFMPEG
{
public:
  VideoDec_FFMPEG();
  ~VideoDec_FFMPEG();

  int openDec( const char* filename,int extra_frame_buffer_num = 5);
  AVCodecParameters* getCodecPara();
  void closeDec();
  AVFrame * grabFrame();
  int openCodecContext(int *stream_idx,AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type, int extra_frame_buffer_num = 5);
  static void bm_find_decoder_name(int dec_id, std::string &dec_name)
  {
    switch (dec_id)
    {
    case AV_CODEC_ID_MJPEG:      dec_name = "jpeg_bm";    break;
    case AV_CODEC_ID_H264:       dec_name = "h264_bm";    break;
    case AV_CODEC_ID_HEVC:       dec_name = "hevc_bm";    break;
    case AV_CODEC_ID_MPEG1VIDEO: dec_name = "mpeg1_bm";   break;
    case AV_CODEC_ID_MPEG2VIDEO: dec_name = "mpeg2_bm";   break;
    case AV_CODEC_ID_MPEG4:      dec_name = "mpeg4_bm";   break;
    case AV_CODEC_ID_MSMPEG4V3:  dec_name = "mpeg4v3_bm"; break;
    case AV_CODEC_ID_FLV1:       dec_name = "flv1_bm";    break;
    case AV_CODEC_ID_H263:       dec_name = "h263_bm";    break;
    case AV_CODEC_ID_CAVS:       dec_name = "cavs_bm";    break;
    case AV_CODEC_ID_AVS:        dec_name = "avs_bm";     break;
    case AV_CODEC_ID_VP3:        dec_name = "vp3_bm";     break;
    case AV_CODEC_ID_VP8:        dec_name = "vp8_bm";     break;
    case AV_CODEC_ID_VC1:        dec_name = "vc1_bm";     break;
    case AV_CODEC_ID_WMV1:       dec_name = "wmv1_bm";    break;
    case AV_CODEC_ID_WMV2:       dec_name = "wmv2_bm";    break;
    case AV_CODEC_ID_WMV3:       dec_name = "wmv3_bm";    break;
    default:                     dec_name = "";           break;
    }

  }
private:
  AVFormatContext *fmt_ctx;
  AVCodecContext *video_dec_ctx ;
  AVCodec *decoder;
  AVCodecParameters *codecparamater;
  int width;
  int height;
  int pix_fmt;
  uint8_t *video_dst_data[4];
  int      video_dst_linesize[4];

  int video_stream_idx;
  AVFrame *frame;
  AVPacket pkt;
  int video_frame_count;
  int refcount;

};
VideoDec_FFMPEG::VideoDec_FFMPEG()
{
  fmt_ctx = NULL;
  video_dec_ctx = NULL;
  decoder = NULL;
  width= 0;
  height = 0;
  pix_fmt = 0;
  video_stream_idx  =-1;
  frame = NULL;
  video_frame_count = 0;
  refcount = 1;
  av_init_packet(&pkt);
  pkt.data = NULL;
  pkt.size = 0;
  codecparamater = NULL;
  frame = av_frame_alloc();
}
VideoDec_FFMPEG::~VideoDec_FFMPEG()
{
  printf("#######VideoDec_FFMPEG exit \n");
  av_frame_free(&frame);
  avcodec_free_context(&video_dec_ctx);
  avformat_close_input(&fmt_ctx);

}

AVCodecParameters* VideoDec_FFMPEG::getCodecPara()
{
  return codecparamater;
}


int VideoDec_FFMPEG::openDec( const char* filename,int extra_frame_buffer_num)
{
  int ret = 0;
  if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
    printf("Cannot open input file\n");
    return ret;
  }

  if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
    printf("Cannot find stream information\n");
    return ret;
  }
  if (openCodecContext(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO , extra_frame_buffer_num) >= 0) {
    //video_stream = fmt_ctx->streams[video_stream_idx];
    width = video_dec_ctx->width;
    height = video_dec_ctx->height;
    pix_fmt = video_dec_ctx->pix_fmt;
  }
  printf("openDec video_stream_idx = %d,pix_fmt = %d \n",video_stream_idx,pix_fmt);
  return ret;
}

int VideoDec_FFMPEG::openCodecContext(int *stream_idx,
                                      AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type, int extra_frame_buffer_num)
{
  int ret, stream_index;
  AVStream *st;
  AVCodec *dec = NULL;
  AVDictionary *opts = NULL;

  ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
  if (ret < 0) {
    printf("Could not find %s stream \n",av_get_media_type_string(type));
    return ret;
  } else {
    stream_index = ret;
    st = fmt_ctx->streams[stream_index];

    /* find decoder for the stream */

    std::string bm_dec_name =  "";
    bm_find_decoder_name(st->codecpar->codec_id,bm_dec_name);
    if (bm_dec_name.empty())
      decoder = avcodec_find_decoder(st->codecpar->codec_id);
    else
    {
      decoder = avcodec_find_decoder_by_name(bm_dec_name.c_str());
      /* if HW decoder not found try SW */
      if (!decoder)
      {
        decoder = avcodec_find_decoder(st->codecpar->codec_id);
      }
    }
    if (!decoder) {
      printf("Failed to find %s codec\n",
             av_get_media_type_string(type));
      return AVERROR(EINVAL);
    }

    /* Allocate a codec context for the decoder */
    *dec_ctx = avcodec_alloc_context3(decoder);
    if (!*dec_ctx) {
      printf("Failed to allocate the %s codec context\n",
             av_get_media_type_string(type));
      return AVERROR(ENOMEM);
    }

    /* Copy codec parameters from input stream to output codec context */
    if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
      printf("Failed to copy %s codec parameters to decoder context\n",
             av_get_media_type_string(type));
      return ret;
    }
    codecparamater = st->codecpar;

#ifndef SOC_MODE
    //pcie mode
    av_dict_set_int(&opts, "pcie_board_id", 0, 0);
    /*do not copy data back to x86, default is copy back*/
    av_dict_set_int(&opts, "pcie_no_copyback", 0, 0);
#endif

    /* Init the decoders, with or without reference counting */
    av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
    if(extra_frame_buffer_num >5)
      av_dict_set_int(&opts, "extra_frame_buffer_num", extra_frame_buffer_num, 0);  // if we use dma_buffer mode
    if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
      printf("Failed to open %s codec\n",
             av_get_media_type_string(type));
      return ret;
    }
    *stream_idx = stream_index;
  }

  return 0;
}

AVFrame * VideoDec_FFMPEG::grabFrame()
{
  int ret = 0;
  int decoded = pkt.size;
  bool valid = false;
  int got_frame = 0;
  while (!valid)
  {
    av_packet_unref(&pkt);
    if(av_read_frame(fmt_ctx, &pkt) < 0)
    {
      return NULL;
    }
    if (pkt.stream_index == video_stream_idx) {

      if (!frame) {
        printf("Could not allocate frame\n");
        return NULL;
      }
      /* decode video frame */
      if(refcount)
        av_frame_unref(frame);
      ret = avcodec_decode_video2(video_dec_ctx, frame, &got_frame, &pkt);
      if (ret < 0) {
        printf("Error decoding video frame (%d)\n", ret);
        continue;
      }

      if (got_frame) {
        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
        if (frame->width != width || frame->height != height ||
            frame->format != pix_fmt) {
          /* To handle this change, one could call av_image_alloc again and
           * decode the following frames into another rawvideo file. */
          printf("Error: Width, height and pixel format have to be "
                 "constant in a rawvideo file, but the width, height or "
                 "pixel format of the input video changed:\n"
                 "old: width = %d, height = %d, format = %s\n"
                 "new: width = %d, height = %d, format = %s\n",
                 width, height, av_get_pix_fmt_name((AVPixelFormat)pix_fmt),
                 frame->width, frame->height,
                 av_get_pix_fmt_name((AVPixelFormat)frame->format));

          continue;
        }
        valid = true;
      }
    }
    else
    {
      printf("###############other packet! \n ");
    }
  }

  if(valid)
    return frame;
  else
    return NULL;
}



int test_yuv_input_bmcv_resize() {

  bm_handle_t handle;
  bm_dev_request(&handle, 0);

  VideoDec_FFMPEG dec;
  int ret = dec.openDec("/home/yuan/station.avi");
  assert(ret == 0);
  AVFrame *frame = dec.grabFrame();


  bm_image input;
  ret = bm::avframe_to_bmimage(handle, frame, input);
  assert(ret == 0);

  int image_num = 1;
  bmcv_resize_image resize_attr[image_num];
  bmcv_resize_t resize_img_attr[image_num];
  for (
      int img_idx = 0;
      img_idx < image_num;
      img_idx++) {
    resize_img_attr[img_idx].
        start_x = 0;
    resize_img_attr[img_idx].
        start_y = 0;
    resize_img_attr[img_idx].
        in_width = input.width;
    resize_img_attr[img_idx].
        in_height = input.height;
    resize_img_attr[img_idx].
        out_width = 400;
    resize_img_attr[img_idx].
        out_height = 400;
  }

  for (
      int img_idx = 0;
      img_idx < image_num;
      img_idx++) {
    resize_attr[img_idx].
        resize_img_attr = &resize_img_attr[img_idx];
    resize_attr[img_idx].
        roi_num = 1;
    resize_attr[img_idx].
        stretch_fit = 0;
    //resize_attr[img_idx].
    //    interpolation = BMCV_INTER_NEAREST;
    resize_attr[img_idx].interpolation = BMCV_INTER_LINEAR;

  }

  bm_image output[image_num];

  for (
      int img_idx = 0;
      img_idx < image_num;
      img_idx++) {
    int output_data_type = DATA_TYPE_EXT_1N_BYTE;
    bm_image_create(handle,
                    600,
                    800,
                    FORMAT_BGR_PLANAR,
                    (bm_image_data_format_ext)
                        output_data_type,
                    &output[img_idx]);
  }
  bm_image_alloc_contiguous_mem(image_num, output,
                                1);
  ret = bmcv_image_resize(handle, image_num, resize_attr, &input, output);
  assert(ret == 0);

  bm::bm_imag_imwrite("bmcv-image-resize.jpg", &output[0]);

  bm_image_free_contiguous_mem(image_num, output);

  for (
      int i = 0;
      i < image_num;
      i++) {
    bm_image_destroy(input);
    bm_image_destroy(output[i]);
  }

  bm_dev_free(handle);
}


void test_rgb_bmcv_resiz(char *filepath, int stretch_fit)
{
  int ret = 0;
  bm_handle_t handle;
  bm_dev_request(&handle, 0);

  int strides[4]={FFALIGN(1280,64), 0};

  cv::Mat img=imread(filepath,cv::IMREAD_AVFRAME);
  bm_image input;
  ret = cv::bmcv::toBMI(img, &input, false);
  assert(ret == 0);

  int image_num = 1;
  bmcv_resize_image resize_attr[image_num];
  bmcv_resize_t resize_img_attr[image_num];
  for (
      int img_idx = 0;
      img_idx < image_num;
      img_idx++) {
    resize_img_attr[img_idx].
        start_x = 0;
    resize_img_attr[img_idx].
        start_y = 0;
    resize_img_attr[img_idx].
        in_width = input.width;
    resize_img_attr[img_idx].
        in_height = input.height;
    resize_img_attr[img_idx].
        out_width = 800;
    resize_img_attr[img_idx].
        out_height = 800;
  }

  for (
      int img_idx = 0;
      img_idx < image_num;
      img_idx++) {
    resize_attr[img_idx].
        resize_img_attr = &resize_img_attr[img_idx];
    resize_attr[img_idx].
        roi_num = 1;
    resize_attr[img_idx].
        stretch_fit = stretch_fit;
    resize_attr[img_idx].interpolation = BMCV_INTER_NEAREST;
    //resize_attr[img_idx].interpolation = BMCV_INTER_LINEAR;

  }

  bm_image output[image_num];

  for (
      int img_idx = 0;
      img_idx < image_num;
      img_idx++) {
    int output_data_type = DATA_TYPE_EXT_1N_BYTE;
    bm_image_create(handle,
                    800,
                    800,
                    FORMAT_BGR_PLANAR,
                    (bm_image_data_format_ext)
                        output_data_type,
                    &output[img_idx]);
  }
  bm_image_alloc_contiguous_mem(image_num, output,
                                1);
  ret = bmcv_image_resize(handle, image_num, resize_attr, &input, output);
  assert(ret == 0);

  std::string ss=cv::format("bmcv-image-resize-%d.jpg", stretch_fit);

  bm::bm_imag_imwrite(ss, &output[0]);

  ss=cv::format("bmcv-image-resize-%d.bin", stretch_fit);
  bm::dump_bmimage(ss, &output[0]);

  bm_image_free_contiguous_mem(image_num, output);

  for (
      int i = 0;
      i < image_num;
      i++) {
    bm_image_destroy(input);
    bm_image_destroy(output[i]);
  }

  bm_dev_free(handle);


}



int main(int argc, char *argv[])
{
  char *filepath = "/home/yuan/dog.jpg";
  if (argc < 1) {
      filepath = argv[1];
  }

  test_rgb_bmcv_resiz(filepath, 1);
  test_rgb_bmcv_resiz(filepath, 0);
  return 0;

}