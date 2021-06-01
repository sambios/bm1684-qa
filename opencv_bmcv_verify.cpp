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
    cv::Mat input = cv::imread(argv[1]);
    printf("input.type = %s\n", cv::typeToString(input.type()).c_str());
    //Dump Data;
    if (0){
        FILE *fp = fopen("opencv.file", "wb");
        //for (int r = 0; r < input.rows; r++)
        //{
        //    fwrite(reinterpret_cast<const char*>(input.ptr(r)), 1, input.cols*input.elemSize(), fp);
        //}
        int size = input.rows * input.cols * input.elemSize();
        fwrite((char*)input.data, 1, size, fp);
        fclose(fp);
    }
    cv::bmcv::dumpMat(input, "opencv.file");
    cv::imwrite("opencv.file.bmp", input);

#if 1
    int ret = 0;
    bm_image bmimg1;
    decode_jpeg(handle, argv[1], bmimg1);
    bm_image out1;
    bm_image_create(handle, bmimg1.height, bmimg1.width, FORMAT_BGR_PACKED, DATA_TYPE_EXT_1N_BYTE, &out1);
    ret = bmcv_image_vpp_convert(handle, 1, bmimg1, &out1);
    CV_Assert(ret == 0);
    //bm::save_bmimage("bmcv.file", &out1);
    cv::bmcv::dumpBMImage(&out1, "bmcv.file");
    bm_image_write_to_bmp(out1, "bmcv.file.bmp");
    bm_image_destroy(bmimg1);
    bm_image_destroy(out1);
#endif
    return 0;
}