//
// Created by yuan on 4/20/21.
//

#include <iostream>
#include <fstream>
#include "bmcv_api_ext.h"
#include <assert.h>

int main(int argc, char *argv[])
{
    std::string file_path = "/home/yuan/car.jpg";

    std::ifstream filestr;
    filestr.open(file_path, std::ios::binary);
    std::filebuf* pbuf = filestr.rdbuf();
    unsigned long size = pbuf->pubseekoff(0, std::ios::end, std::ios::in);
    char* buffer = new char[size];
    pbuf->pubseekpos(0, std::ios::in);
    pbuf->sgetn((char*)buffer, size);

    bm_handle_t bm_handle;
    int ret = bm_dev_request(&bm_handle, 0);
    bm_image image1;
    ret = bmcv_image_jpeg_dec(bm_handle, (void**)&buffer, &size, 1, &image1);
    if (BM_SUCCESS !=ret) {
        printf("bmcv_image_jpeg_dec error=%d\n", ret);
        return -1;
    }

    bm_device_mem_t planar_mems[3];
    ret = bm_image_get_device_mem(image1, planar_mems);
    assert(BM_SUCCESS == ret);

    bm_image image2;
    bm_image_create(bm_handle, image1.height, image1.width, FORMAT_GRAY,
                    DATA_TYPE_EXT_1N_BYTE, &image2);
    bm_image_attach(image2, &planar_mems[0]);
    bm_image_write_to_bmp(image2, "test_gray.bmp");
    bm_image_destroy(image1);
    bm_dev_free(bm_handle);


}