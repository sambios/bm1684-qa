//
// Created by yuan on 5/6/21.
//

#include <opencv2/opencv.hpp>
#include <opencv2/core/bmcv.hpp>
#include "bm_wrapper.hpp"

#include "utils.hpp"
#include <bmlib/bmcv_api.h>
#include <bmlib/bmlib_runtime.h>
#include <vector>

using namespace std;

int test_mat2bmimage(int argc, char* argv[])
{
    bm_image bmimg;
    cv::Mat img = cv::imread("test_depth.bmp");
    if( argc > 1)
    {
        std::cout<< "1 channels"<< std::endl;
        img = cv::imread("test_depth.bmp", 0);
    }
    int flag = (img.channels() == 1)?CV_8UC1:CV_8UC3;
    cv::Mat xx = cv::Mat::zeros(img.rows, img.cols, flag);
    for(int i = 0; i < img.rows; ++i)
    {
        uchar *ptr_src = img.ptr<uchar>(i);
        uchar *ptr_dst = xx.ptr<uchar>(i);

        memcpy(ptr_dst, ptr_src, img.cols*img.channels());
    }
    cv::bmcv::toBMI(xx, &bmimg, true);
    bm_image_write_to_bmp(bmimg, "result.bmp");
    bm_image_destroy(bmimg);

    return 0;
}


void test_preprocess(int argc, char* argv[])
{

    std::cout<< __FILE__<< " "<< __LINE__<< std::endl;

    std::string img_path = "images/test_depth.bmp";//argv[1];
    int dev_id = 0;

    bm_handle_t bmHandle;
    bm_status_t s = bm_dev_request(&bmHandle, dev_id);
    if (s != BM_SUCCESS) {
        cout << "Initialize bm handle failed, ret = " << endl;
        exit(-1);
    }
    std::cout<< __FILE__<< " "<< __LINE__<< std::endl;
    void *p_bmrt_ = bmrt_create(bmHandle);
    if (nullptr == p_bmrt_) {
        cout << "ERROR: get handle failed!" << endl;
        exit(1);
    }
    std::cout<< __FILE__<< " "<< __LINE__<< std::endl;
    // init resize & crop & split of opencv
    bm_image resize_tmp[1];
    bm_image resize_bmcv_[1];
    bm_image linear_trans_bmcv_[1];

    std::cout<< __FILE__<< " "<< __LINE__<< std::endl;

    bm_image_create_batch(bmHandle,
                          112,  //h
                          112,  //w
                          FORMAT_GRAY,
                          DATA_TYPE_EXT_1N_BYTE,
                          resize_bmcv_,
                          1);
    std::cout<< __FILE__<< " "<< __LINE__<< std::endl;
    bm_image_create_batch (bmHandle,
                           112,
                           112,
                           FORMAT_GRAY,
                           DATA_TYPE_EXT_FLOAT32,
                           linear_trans_bmcv_,
                           1);
    std::cout<< __FILE__<< " "<< __LINE__<< std::endl;
    std::vector<cv::Mat> results_vpp_;

    std::vector<cv::Size> resize_params_;
    cv::Size resize_param (112, 112);
    resize_params_.push_back (resize_param);

    // linear transformation arguments of BMCV
    bmcv_convert_to_attr linear_trans_param_;
    float input_scale = 0.0078125;
//    float input_scale = net_info_->input_scales[0];
    linear_trans_param_.alpha_0 = input_scale;
    linear_trans_param_.beta_0 = -127.5 * input_scale;
    linear_trans_param_.alpha_1 = input_scale;
    linear_trans_param_.beta_1 = -127.5 * input_scale;
    linear_trans_param_.alpha_2 = input_scale;
    linear_trans_param_.beta_2 = -127.5 * input_scale;

    std::cout<< __FILE__<< " "<< __LINE__<< std::endl;

    cv::Mat img = cv::imread(img_path, /*cv::IMREAD_COLOR*/0, dev_id);

#if 0
    // do not crop
    cv::Rect crop_param (0,0,img.cols,img.rows);
    vector<cv::Rect> crop_params;
    crop_params.push_back (crop_param);

    // do resize && split by opencv::bmcv
    cv::bmcv::convert (img, crop_params, resize_params_, results_vpp_); // resize && split
    cv::bmcv::toBMI (results_vpp_[0] , &resize_tmp[0]); // change format to bm_image

    // change format
    bmcv_image_storage_convert (bmHandle , 1, resize_tmp, resize_bmcv_);
#else
    bm_image image1;
    cv::bmcv::toBMI(img, &image1);
    auto ret = bmcv_image_vpp_convert(bmHandle, 1, image1, resize_bmcv_);
    CV_Assert(BM_SUCCESS == ret);
#endif
    // do linear transform
    ret = bmcv_image_convert_to (bmHandle, 1, linear_trans_param_, resize_bmcv_, linear_trans_bmcv_);
    CV_Assert(BM_SUCCESS == ret);

    bm_image_format_info_t fmt_info_0;
    bm_image_format_info_t fmt_info_1;
    bm_image_get_format_info(&(resize_bmcv_[0]), &fmt_info_0);
    bm_image_get_format_info(&(linear_trans_bmcv_[0]), &fmt_info_1);

    for(int i = 0; i < fmt_info_0.plane_nb; ++i)
    {
        int size = fmt_info_0.height * fmt_info_0.stride[i];
        uint8_t *pSystemData = new uint8_t[size];
        bm_memcpy_d2s_partial(bmHandle, pSystemData, fmt_info_0.plane_data[i], size);
        std::cout<< "fmt_info_0:"<< i<< std::endl;
        uint8_t *pu8 = (uint8_t *)pSystemData;
        for(int i = 0; i < 5; ++i)
        {
            std::cout<< (int)pu8[i]<< " ";
        }
        std::cout<< std::endl;

        delete [] (int8_t *)pSystemData;
    }

    for(int i = 0; i < fmt_info_1.plane_nb; ++i)
    {
        int size = fmt_info_1.height * fmt_info_1.stride[i];
        void *pSystemData = new int8_t[size];
        bm_memcpy_d2s_partial(bmHandle, pSystemData, fmt_info_1.plane_data[i], size);
        std::cout<< "fmt_info_1:"<< i<< std::endl;
        float *pFloat = (float*)pSystemData;
        for(int i = 0; i < 5; ++i)
        {
            std::cout<< pFloat[i]<< " ";
        }
        std::cout<< std::endl;

        delete [] (int8_t *)pSystemData;
    }



    return ;
}


int main(int argc, char* argv[])
{
    test_preprocess(argc, argv);

    return 0;
}
