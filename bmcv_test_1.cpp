//
// Created by yuan on 6/2/21.
//
#include <iostream>
#include <fstream>
#include "bmcv_api_ext.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "USAGE:" << std::endl;
        std::cout << "  " << argv[0] << " <gray image path>" << std::endl;
        exit(1);
    }
    bmlib_log_set_level(BMLIB_LOG_TRACE);

    bm_handle_t handle;
    bm_dev_request(&handle, 0);

    std::ifstream filestr;
    filestr.open(argv[1], std::ios::binary);
    if (!filestr.is_open()) {
        return -1;
    }

    std::filebuf* pbuf = filestr.rdbuf();
    unsigned long size = pbuf->pubseekoff(0, std::ios::end, std::ios::in);
    char* buffer = new char[size];
    pbuf->pubseekpos(0, std::ios::in);
    pbuf->sgetn((char*)buffer, size);

    bm_image img;
    int i = 0;
    while(i++ < 10000) {
        int ret = bmcv_image_jpeg_dec(handle, (void **) &buffer, &size, 1, &img);
        if (BM_SUCCESS != ret) {
            printf("bmcv_image_jpeg_dec error=%d\n", ret);
            delete[]buffer;
            return ret;
        }

        bm_image_destroy(img);
        printf("loop %d\n", i);
    }

    delete [] buffer;
    bm_dev_free(handle);
    return 0;
}