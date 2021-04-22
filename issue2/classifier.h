#pragma once
#include <stdlib.h>
#include <fstream>
#include <cmath>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <sys/timeb.h>
#include <assert.h>
#include "bmcv_api_ext.h"

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "bmruntime_interface.h"

#define USE_OPENCV 1
#include "bm_wrapper.hpp"

struct classifier_result {
    std::vector<float> conf;
    std::vector<int> scene;
};

class Classifier
{
public:
    Classifier(const std::string& bmodel);
    ~Classifier();
    bool Run(std::vector<cv::Mat>& input_images,std::vector<classifier_result>& results);
    bool Run(cv::Mat& input_image, classifier_result& result);
    bool init_status_ ;

private:
    void preprocess(bm_image& in, bm_image& out);
    void preForward(std::vector<cv::Mat>& images);
    bool forward();
    std::vector<classifier_result> postForward();
    std::vector<int> sort_indexes_e(std::vector<float> &v);

    float PI = std::acos(-1);
    /* handle of low level device */
    bm_handle_t bm_handle_;

    /* runtime helper */
    const char **net_names_;
    void *p_bmrt_;

    /* network input shape */
    int batch_size_;
    int num_channels_;
    int class_num_;

    float threshold_;
    int net_h_;
    int net_w_;

    float input_scale_;
    float output_scales_;
    std::vector<void*> outputs_;
    bool int8_flag_;
    int output_num_;

    // linear transformation arguments of BMCV
    bmcv_convert_to_attr convert_attr_;
    bm_shape_t input_shape_;
    bm_image* scaled_inputs_;
    std::vector<cv::Mat> images_;
    std::vector<int> output_sizes_;
    std::vector<bm_image> processed_imgs_;
    std::vector<cv::Mat> input_images_;

};
