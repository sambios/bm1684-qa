//
// Created by yuan on 4/20/21.
//
#include "opencv2/opencv.hpp"
#include "bmlib_runtime.h"
#include "bmcv_api_ext.h"

#define TEST_SOC 0

static void upload(bm_handle_t handle, cv::Mat &m, bm_image *image)
{
    uchar *p = m.data;
    bm_device_mem_t mem[4];

    if (p == NULL) return;  // m.data==NULL means there is not virtual memory, thus no sync up required.
#if !TEST_SOC
    if (m.avOK()) return;   // if it is avOK, the data is from vpu/jpu and always working on physical memory.
#endif

    memset(mem, 0, sizeof(mem));
    bm_image_get_device_mem(*image, mem);
    for (int i = 0; i < 4; i++) {
        if (mem[i].size > 0) {
#if TEST_SOC
            bm_mem_flush_device_mem(handle, &mem[i]);
#else
            bm_memcpy_s2d(handle, mem[i], p);
            p += mem[i].size;
#endif
        }
    }
}

static int toBMI(cv::Mat &m, bm_image *image, bool update=true) {
    if (!m.u) {printf("Memory allocated by user, no device memory assigned. Not support BMCV!\n"); return BM_NOT_SUPPORTED;}
    bm_status_t ret;
    bm_handle_t handle = m.u->hid ? m.u->hid : cv::bmcv::getCard();

    CV_Assert(m.rows > 0 && m.cols > 0);

    int step[1] = { (int)m.step[0] };
    ret = bm_image_create(handle, m.rows, m.cols, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, image, step);
    if (ret != BM_SUCCESS) return ret;

    uint off = m.data - m.datastart;
    uint len = m.rows * m.step[0];
    bm_device_mem_t mem = bm_mem_from_device(m.u->addr + off, len);

    ret = bm_image_attach(*image, &mem);
    if (update) upload(handle, m, image);
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cout << "USAGE:" << std::endl;
        std::cout << "  " << argv[0] << " <gray image path>" << std::endl;
        exit(1);
    }
    bm_image bmimg1;
    printf("image: %s\n", argv[1]);
    cv::Mat input = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
    printf("input.type = %s\n", cv::typeToString(input.type()).c_str());
    printf("input.u.addr=%u\n", input.u->addr);

    //toBMI(input, &bmimg1, true);
    int flag = (input.channels() == 1? CV_8UC1 : CV_8UC3);
    cv::Mat xx = cv::Mat::zeros(input.rows, input.cols, flag);
    cv::bmcv::toBMI(xx, &bmimg1, true);
    
    bm_image_write_to_bmp(bmimg1, "test1.bmp");
    bm_image_destroy(bmimg1);

}
