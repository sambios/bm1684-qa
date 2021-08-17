//
// Created by yuan on 7/1/21.
//
#include "opencv2/opencv.hpp"
#include <fstream>

#define USE_FFMPEG 1
#define USE_OPENCV 1
#define USE_BMCV 1

#include "engine.h"
#include "cvwrapper.h"
#include "tensor.h"
int main(int argc, char *argv[])
{
    std::string filepath = "rtsp://admin:hk123456@11.73.11.99/test";
    sail::Handle handle = sail::Handle(0);
    //imread
    cv::Mat img1 = cv::imread(filepath);
    //sail read
    if (true) {

        sail::Decoder decoder = sail::Decoder(filepath, false, 0);
        sail::BMImage image;
        int ret = decoder.read(handle, image);
        if (ret != 0) {
            std::cout << "read error!" << std::endl;
            return -1;
        }

        sail::Bmcv bmcv = sail::Bmcv(handle);
        printf("format = %d\n", image.format());
        sail::BMImage image2;
        if (image.format() == FORMAT_YUV420P ||
            image.format() == FORMAT_NV12) {
            image = bmcv.yuv2bgr(image);

        }

        sail::BMImage resized_img = bmcv.resize(image, 200, 200);
        sail::Tensor *t1 = new sail::Tensor(handle, {1, 3, resized_img.height(), resized_img.width()}, BM_UINT8, true, false);
        bmcv.bm_image_to_tensor(image, *t1);
        t1->sync_d2s();



    }

    return 0;
}