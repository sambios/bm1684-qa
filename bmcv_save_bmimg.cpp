//
// Created by yuan on 4/30/21.
//

#include "bmcv_api_ext.h"
#include "opencv2/opencv.hpp"

void save_to_file(const std::string& file, void* p, int size) {
    FILE *fp = fopen(file.c_str(), "wb");
    fwrite(p,1, size, fp);
    fclose(fp);
}


int main(int argc, char *argv[])
{
    std::string file_path = "/home/yuan/car.jpg";
    int dev_id = 0;
    bm_handle_t bmHandle;
    bm_dev_request(&bmHandle, dev_id);
    cv::Mat img1 = cv::imread(file_path, cv::IMREAD_COLOR, dev_id);
    bm_image bmimg1;
    cv::bmcv::toBMI(img1, &bmimg1, true);


    bm_image_format_info_t fmt_info;
    bm_image_get_format_info(&bmimg1, &fmt_info);
    printf("bmimg1 format info:format = %d, data_type=%d, plane_num=%d, stride[0]=%d\n",
            fmt_info.image_format,
            fmt_info.data_type,
            fmt_info.plane_nb,
            fmt_info.stride[0]);

    bm_image bmimg_rgb_planner;
    bm_image_create(bmHandle, bmimg1.height, bmimg1.width, FORMAT_RGB_PLANAR,
            DATA_TYPE_EXT_1N_BYTE, &bmimg_rgb_planner);

    bmcv_image_vpp_convert(bmHandle, 1, bmimg1, &bmimg_rgb_planner);
    bm_image_get_format_info(&bmimg_rgb_planner, &fmt_info);

    printf("bmimg2 format info:format = %d, data_type=%d, plane_num=%d, stride[0]=%d\n",
           fmt_info.image_format,
           fmt_info.data_type,
           fmt_info.plane_nb,
           fmt_info.stride[0]);

    for(int i = 0; i< fmt_info.plane_nb; ++i) {
        int size = fmt_info.height* fmt_info.stride[i];
        void *pSystemData = new int8_t[size];
        bm_memcpy_d2s_partial(bmHandle, pSystemData, fmt_info.plane_data[i], size);
        //Save Data
        std::string ofn = cv::format("plane-%d.dat", i);
        save_to_file(ofn, pSystemData, size);
        delete[]pSystemData;
    }

    bm_image_destroy(bmimg1);
    bm_dev_free(bmHandle);
}