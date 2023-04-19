//
// Created by yuan on 2023/4/19.
//
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include "bmcv_api_ext.h"
#include "opencv2/opencv.hpp"

static void save_to_file(const std::string& file, void* p, int size) {
    FILE *fp = fopen(file.c_str(), "wb");
    fwrite(p,1, size, fp);
    fclose(fp);
}

static int read_from_file(const std::string& filepath, uint8_t **p_buffer, int *p_size)
{
    std::ifstream filestr;
    filestr.open(filepath, std::ios::binary);
    if (!filestr.is_open()) {
        std::cout << "ERROR:file not exist" << std::endl;
        return -1;
    }

    std::filebuf* pbuf = filestr.rdbuf();
    unsigned long size = pbuf->pubseekoff(0, std::ios::end, std::ios::in);
    uint8_t * buffer = new uint8_t [size];
    pbuf->pubseekpos(0, std::ios::in);
    pbuf->sgetn((char*)buffer, size);

    *p_buffer = buffer;
    *p_size = size;
    return 0;
}

int main(int argc, char *argv[])
{
    std::string file_path = "/home/yuan/tmp/zidane.jpg";
    int dev_id = 0;
    int ret = 0;
    bm_handle_t bmHandle;
    int image_w, image_h;

    /* check file exists */
    if (access(file_path.c_str(), 0) == -1) {
        printf("File not exist!");
        return -1;
    }
    bm_dev_request(&bmHandle, dev_id);
    bmlib_log_set_level(BMLIB_LOG_VERBOSE);
    cv::Mat img1 = cv::imread(file_path, cv::IMREAD_COLOR, dev_id);

    /* convert cv::Mat to bm_image */
    bm_image bmimg1;
    cv::bmcv::toBMI(img1, &bmimg1, true);
    image_w = bmimg1.width;
    image_h = bmimg1.height;

    /* save rgb data to file */
    bm_image_format_info_t fmt_info;
    bm_image_get_format_info(&bmimg1, &fmt_info);
    printf("bmimg1 format info:format = %d, data_type=%d, plane_num=%d, stride[0]=%d\n",
           fmt_info.image_format,
           fmt_info.data_type,
           fmt_info.plane_nb,
           fmt_info.stride[0]);
    // convert target RGB_PLANER format
    bm_image bmimg_rgb_planner;
    bm_image_create(bmHandle, bmimg1.height, bmimg1.width, FORMAT_RGB_PLANAR,
                    DATA_TYPE_EXT_1N_BYTE, &bmimg_rgb_planner);

    ret = bmcv_image_vpp_convert(bmHandle, 1, bmimg1, &bmimg_rgb_planner);
    assert(BM_SUCCESS == ret);
    ret = bm_image_get_format_info(&bmimg_rgb_planner, &fmt_info);
    assert(BM_SUCCESS == ret);

    printf("bmimg2 format info:format = %d, data_type=%d, plane_num=%d, stride[0]=%d\n",
           fmt_info.image_format,
           fmt_info.data_type,
           fmt_info.plane_nb,
           fmt_info.stride[0]);

    for(int i = 0; i< fmt_info.plane_nb; ++i) {
        int size = fmt_info.height* fmt_info.stride[i] * 3;
        void *pSystemData = new int8_t[size];
        bm_memcpy_d2s_partial(bmHandle, pSystemData, fmt_info.plane_data[i], size);
        //Save Data
        std::string ofn = cv::format("plane-%d.dat", i);
        save_to_file(ofn, pSystemData, size);
        delete[]pSystemData;
    }

    bm_image_destroy(bmimg_rgb_planner);
    bm_image_destroy(bmimg1);

    /*read rgb planer data from file
     * 1. read data from file
     * 2. create bm_image
     * 3. transfer data to bm_image
     * */
    file_path = "plane-0.dat";
    uint8_t *rgb_data=NULL;
    int rgb_data_size=0;

    ret = read_from_file(file_path, &rgb_data, &rgb_data_size);
    if (ret < 0){
        return -1;
    }

    memset(&bmimg_rgb_planner, 0, sizeof(bmimg_rgb_planner));
    bm_image_create(bmHandle, image_h, image_w, FORMAT_RGB_PLANAR,
                    DATA_TYPE_EXT_1N_BYTE, &bmimg_rgb_planner);
    ret = bm_image_alloc_dev_mem_heap_mask(bmimg_rgb_planner, 3);
    assert(BM_SUCCESS == ret);

    uint8_t* buffers[3]={0};
    buffers[0] = rgb_data;
    ret = bm_image_copy_host_to_device(bmimg_rgb_planner, (void**)buffers);
    assert(BM_SUCCESS == ret);
    ret = bm_image_write_to_bmp(bmimg_rgb_planner, "rgb_data.bmp");
    assert(BM_SUCCESS == ret);
    bm_image_destroy(bmimg_rgb_planner);

    /* rgb data -> cv::Mat -> bm_image */
    cv::Mat cv_img1_r = cv::Mat(image_h, image_w, CV_8UC1, rgb_data);
    cv::Mat cv_img1_g = cv::Mat(image_h, image_w, CV_8UC1, rgb_data + image_w*image_h);
    cv::Mat cv_img1_b = cv::Mat(image_h, image_w, CV_8UC1, rgb_data + image_w*image_h*2);

    cv::Mat cv_mats[]={cv_img1_b, cv_img1_g, cv_img1_r};
    cv::Mat cv_img1, cv_img2;
    cv::merge(cv_mats, 3, cv_img1);
    // create device memory
    cv_img1.copyTo(cv_img2);
    cv::bmcv::toBMI(cv_img2, &bmimg_rgb_planner, true);
    cv::imwrite("cv_img2.jpg", cv_img2);

    bm_image_destroy(bmimg_rgb_planner);
    free(rgb_data);
    bm_dev_free(bmHandle);
}