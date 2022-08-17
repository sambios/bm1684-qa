//
// Created by yuan on 6/2/21.
//
#include <iostream>
#include <fstream>
#include <cstring>
#include <assert.h>
#include "bmcv_api_ext.h"

#define N 8
static void bmimage_new_test(bm_handle_t bmhandle)
{
  bm_image *resize_bmcv_ = nullptr;

  if(resize_bmcv_ == nullptr)
  {
    resize_bmcv_ = new bm_image[N];
  }

  for(int i = 0; i < N; ++i)
  {
    bm_status_t status = bm_image_create(bmhandle, 640, 640,
                                         FORMAT_BGR_PLANAR, DATA_TYPE_EXT_1N_BYTE, &resize_bmcv_[i]);
    if(BM_SUCCESS != status)
    {
      std::cout << "create bm image error" << std::endl;
      exit(0);
    }
  }

  auto ret = bm_image_alloc_contiguous_mem(N, resize_bmcv_);
  assert(BM_SUCCESS == ret);
  bm_image_free_contiguous_mem(N, resize_bmcv_);
  for(int i = 0; i < N; ++i) {
    bm_image_destroy(resize_bmcv_[i]);
  }

  delete [] resize_bmcv_;

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

    bmimage_new_test(handle);

    return 0;

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
        // this is important.
        memset(&img, 0, sizeof(img));
        printf("loop %d\n", i);
    }

    delete [] buffer;
    bm_dev_free(handle);
    return 0;
}