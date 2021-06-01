//
// Created by yuan on 5/31/21.
//

#include "opencv2/opencv.hpp"
#include <fstream>

#define USE_FFMPEG 1
#define USE_OPENCV 1
#define USE_BMCV 1

#include "engine.h"
#include "cvwrapper.h"
#include "tensor.h"


int decode_jpeg_test(sail::Handle handle, std::string filepath)
{
    std::ifstream filestr;
    filestr.open(filepath, std::ios::binary);
    if (!filestr.is_open()) {
        return -1;
    }

    bmlib_log_set_level(BMLIB_LOG_TRACE);

    std::filebuf* pbuf = filestr.rdbuf();
    unsigned long size = pbuf->pubseekoff(0, std::ios::end, std::ios::in);
    char* buffer = new char[size];
    pbuf->pubseekpos(0, std::ios::in);
    pbuf->sgetn((char*)buffer, size);
    bm_handle_t bm_handle = handle.data();
    bm_image image1;
    int ret = bmcv_image_jpeg_dec(bm_handle, (void**)&buffer, &size, 1, &image1);
    if (BM_SUCCESS !=ret) {
        printf("bmcv_image_jpeg_dec error=%d\n", ret);
        delete []buffer;
        return ret;
    }

    bm_image_destroy(image1);
    delete [] buffer;
    return 0;
}

int main(int argc, char *argv[])
{
    std::string filepath = "images/similar_0.jpg";
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
        if (image.format() == FORMAT_YUV420P ||
            image.format() == FORMAT_NV12) {
            sail::BMImage image2 = bmcv.yuv2bgr(image);
        }

        bmcv.imwrite("sail_test_1.jpg", image);
    }

    if(0) {
        decode_jpeg_test(handle, filepath);
    }
    return 0;
}