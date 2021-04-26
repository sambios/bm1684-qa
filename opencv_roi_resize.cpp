//
// Created by yuan on 4/23/21.
//

#include "bmlib_runtime.h"
#include "bmcv_api_ext.h"
#include "opencv2/opencv.hpp"
#include "bm_wrapper.hpp"

int main(int argc, char *argv[]) {
    bm_handle_t bm_handle;
    bmlib_log_set_level(BMLIB_LOG_VERBOSE);
    auto ret = bm_dev_request(&bm_handle, 0);
    assert(BM_SUCCESS == ret);
    cv::Mat image1 = cv::imread("/home/yuan/car.jpg");
    if (image1.empty()) {
        std::cout << "read image error!\n" << std::endl;
        return -1;
    }

    cv::Size roi_size(127,127);
    cv::Mat img_roi, img_reiszed;
    img_reiszed.create(64, 64, image1.type());
    //roi
    for(int i=0;i < image1.cols - roi_size.width;++i)
    {
        for (int j = 0; j < image1.rows-roi_size.height; ++j) {

            if (j%32==0) {
                cv::Rect rect(cv::Size(i, j), roi_size);
                img_roi = image1(rect);
                cv::bmcv::resize(img_roi, img_reiszed);
                cv::imwrite("img_resized.jpg", img_reiszed);
            }
        }

    }

    bm_dev_free(bm_handle);

}