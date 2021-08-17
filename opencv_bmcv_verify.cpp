//
// Created by yuan on 5/31/21.
//
#include "opencv2/opencv.hpp"
#include "bmlib_runtime.h"
#include "bmcv_api_ext.h"
#include <fstream>
#include "common.h"

int decode_jpeg(bm_handle_t handle, std::string filepath, bm_image &img)
{
    std::ifstream filestr;
    filestr.open(filepath, std::ios::binary);
    if (!filestr.is_open()) {
        return -1;
    }

    std::filebuf* pbuf = filestr.rdbuf();
    unsigned long size = pbuf->pubseekoff(0, std::ios::end, std::ios::in);
    char* buffer = new char[size];
    pbuf->pubseekpos(0, std::ios::in);
    pbuf->sgetn((char*)buffer, size);
    int ret = bmcv_image_jpeg_dec(handle, (void**)&buffer, &size, 1, &img);
    if (BM_SUCCESS !=ret) {
        printf("bmcv_image_jpeg_dec error=%d\n", ret);
        delete []buffer;
        return ret;
    }

    delete [] buffer;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "USAGE:" << std::endl;
        std::cout << "  " << argv[0] << " <gray image path>" << std::endl;
        exit(1);
    }
    bmlib_log_set_level(BMLIB_LOG_TRACE);

    bm_handle_t handle;
    bm_dev_request(&handle, 0);

    printf("image: %s\n", argv[1]);
    cv::Mat input0_dev0 = cv::imread(argv[1], cv::IMREAD_COLOR, 0);
    cv::Mat input1_dev0 = cv::imread(argv[1], cv::IMREAD_COLOR, 0);
    printf("input.type = %s\n", cv::typeToString(input0_dev0.type()).c_str());
    printf("input.avformat=%d\n", input0_dev0.avFormat());
#if 0
    cv::Mat input1_dev1 = cv::imread(argv[1], cv::IMREAD_COLOR, 1);
    printf("before copy input0_dev0.card=%d\n", input0_dev0.card);
    input0_dev0 = input1_dev1;
    printf("after copy input.card=%d\n", input0_dev0.card);

    cv::Mat input3;
    input1_dev0.copyTo(input3);
    input1_dev1.copyTo(input1_dev0);
    printf("after deep copy input1_dev0.card=%d\n", input1_dev0.card);
    printf("after deep copy input1_dev1.card=%d\n", input1_dev1.card);
#endif
    //Dump Data;
    bm::save_cvmat("opencv.file", input0_dev0);

    cv::bmcv::dumpMat(input0_dev0, "opencv.file2");
    cv::imwrite("opencv.file.bmp", input0_dev0);

    int ret = 0;
    bm_image bmimg1;
    decode_jpeg(handle, argv[1], bmimg1);
    bm_image out1;
    bm_image_create(handle, bmimg1.height, bmimg1.width, FORMAT_BGR_PACKED, DATA_TYPE_EXT_1N_BYTE, &out1);
    ret = bmcv_image_vpp_csc_matrix_convert(handle, 1, bmimg1, &out1, CSC_YPbPr2RGB_BT601);
    CV_Assert(ret == 0);

    // save file 1
    cv::bmcv::dumpBMImage(&out1, "bmcv.file");
    // save file 2
    bm::save_bmimage("bmcv.file2", &out1);
    // savee to bmp
    bm_image_write_to_bmp(out1, "bmcv.file.bmp");
    bm_image_destroy(bmimg1);
    bm_image_destroy(out1);
    return 0;
}