//
// Created by yuan on 4/19/22.
//

#include <iostream>
#include <fstream>
#include <cstring>
#include <assert.h>
#include <opencv2/opencv.hpp>
#include "bmcv_api_ext.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "USAGE:" << std::endl;
    std::cout << "  " << argv[0] << " <gray image path>" << std::endl;
    exit(1);
  }
  bmlib_log_set_level(BMLIB_LOG_TRACE);

  int ret = 0;
  bm_handle_t handle;
  bm_dev_request(&handle, 0);

  int strides[4]={FFALIGN(1280,64), 0};

  cv::Mat img=imread(argv[1],cv::IMREAD_AVFRAME);
  bm_image input_img;
  ret = cv::bmcv::toBMI(img, &input_img, false);
  assert(ret == 0);

  //bm_image image1;//1280x1706
  //ret= bm_image_create(handle, input_img.height, input_img.width,FORMAT_BGR_PLANAR,DATA_TYPE_EXT_1N_BYTE, &image1);
  //assert(0 == ret);
  //ret = bm_image_alloc_dev_mem_heap_mask(image1, 2);
  //assert(0 == ret);
  //ret = bmcv_image_vpp_convert(handle, 1, input_img, &image1);
  //assert(0 == ret);

  bmcv_padding_atrr_t padding_attr; memset(&padding_attr, 0, sizeof(padding_attr));
  padding_attr.dst_crop_sty = 0;
  padding_attr.dst_crop_stx = 0;
  padding_attr.padding_b = 114;
  padding_attr.padding_g = 114;
  padding_attr.padding_r = 114;
  padding_attr.if_memset = 1;
  padding_attr.dst_crop_h = 658;
  padding_attr.dst_crop_w = 1280;

  bm_image output_img;
  ret= bm_image_create(handle, 720, 1280,FORMAT_BGR_PLANAR,DATA_TYPE_EXT_1N_BYTE, &output_img);
  assert(0 == ret);
  //ret = bm_image_alloc_dev_mem_heap_mask(output_img, 2);
  //assert(0 == ret);

  bmcv_rect_t crop_rect = {254,770, 2126, 1094};
  //ret = bmcv_image_vpp_convert(handle, 1, input_img, &output_img, &crop_rect);
  //assert(0 == ret);

  ret = bmcv_image_vpp_convert_padding(handle, 1, input_img, &output_img, &padding_attr, &crop_rect);
  assert(0 == ret);

  cv::Mat output_mat;
  cv::bmcv::toMAT(&output_img, output_mat);
  cv::imwrite("output.jpeg", output_mat);

  bm_dev_free(handle);
  return 0;
}