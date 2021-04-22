//
// Created by yuan on 4/22/21.
//

#include "classifier.h"

#define USE_BMCV 0

Classifier::Classifier(const std::string &bmodel) {
    init_status_ = false;
    bmlib_log_set_level(BMLIB_LOG_VERBOSE);
    auto ret = bm_dev_request(&bm_handle_, 0);
    assert(BM_SUCCESS == ret);

    p_bmrt_ = bmrt_create(bm_handle_);
    assert(bmrt_load_bmodel(p_bmrt_, bmodel.c_str()));

    bmrt_get_network_names(p_bmrt_, &net_names_);
    std::cout << "> Load model " << net_names_[0] << " successfully" << std::endl;

    /* more info pelase refer to bm_net_info_t in bmdef.h */
    auto net_info = bmrt_get_network_info(p_bmrt_, net_names_[0]);
    bm_image_data_format_ext data_type = DATA_TYPE_EXT_1N_BYTE_SIGNED;

    /* TODO: get class number from net_info */

    /* get fp32/int8 type, the thresholds may be different */
    if (BM_FLOAT32 == net_info->input_dtypes[0]) {
        threshold_ = 0.5;
        int8_flag_ = false;
        std::cout << "fp32 model" << std::endl;
        data_type = DATA_TYPE_EXT_FLOAT32;
    } else {
        threshold_ = 0.5;
        int8_flag_ = true;
        std::cout << "int8 model" << std::endl;
    }

    bmrt_print_network_info(net_info);

    /*
     * only one input shape supported in the pre-built model
     * you can get stage_num from net_info
     */
    auto &input_shape = net_info->stages[0].input_shapes[0];
    /* malloc input and output system memory for preprocess data */
    int count = bmrt_shape_count(&input_shape);

    output_num_ = net_info->output_num;

    for (int i = 0; i < output_num_; i++) {
        auto &output_shape = net_info->stages[0].output_shapes[i];
        count = bmrt_shape_count(&output_shape);
        if (int8_flag_) {
            float *out = new float[count];
            outputs_.push_back(out);
        } else {
            float *out = new float[count];
            outputs_.push_back(out);
        }
        output_sizes_.push_back(output_shape.dims[1]);
        class_num_ = output_shape.dims[1];
    }

    batch_size_ = input_shape.dims[0];
    num_channels_ = input_shape.dims[1];
    net_h_ = input_shape.dims[2];
    net_w_ = input_shape.dims[3];
    input_shape_ = {4, {batch_size_, 3, net_h_, net_w_}};

    output_scales_ = net_info->output_scales[0];

    float input_scale_ = 1.0 / 255;
    if (int8_flag_) {
        input_scale_ *= net_info->input_scales[0];
    }

    convert_attr_.alpha_0 = input_scale_;
    convert_attr_.beta_0 = 0;

    convert_attr_.alpha_1 = input_scale_;
    convert_attr_.beta_1 = 0;

    convert_attr_.alpha_2 = input_scale_;
    convert_attr_.beta_2 = 0;

    scaled_inputs_ = new bm_image[batch_size_];
    /* create bm_image - used for border processing */
    ret = bm_image_create_batch(bm_handle_, net_h_, net_w_, FORMAT_RGB_PLANAR, data_type,
                                scaled_inputs_, batch_size_);

    std::cout << "Classifier model net init finish!" << std::endl;

    if (BM_SUCCESS != ret) {
        std::cerr << "ERROR: bm_image_create_batch failed" << std::endl;
        return;
    }
    processed_imgs_.resize(4);
    bm_image_create(bm_handle_, net_h_, net_w_, FORMAT_RGB_PLANAR, DATA_TYPE_EXT_1N_BYTE, &processed_imgs_[0],
                    NULL);
    init_status_ = true;
}

Classifier::~Classifier() {
    bm_image_destroy_batch(scaled_inputs_, batch_size_);

    if (scaled_inputs_) {
        delete[]scaled_inputs_;
    }

    for (size_t i = 0; i < outputs_.size(); i++) {
        delete[] reinterpret_cast<float *>(outputs_[i]);
    }
    free(net_names_);


    for (size_t i = 0; i < processed_imgs_.size(); i++) {
        bm_image_destroy(processed_imgs_[i]);
    }
}

void Classifier::preprocess(bm_image &in, bm_image &out) {

    bmcv_rect_t crop_rect = {0, 0, in.width, in.height};
    bmcv_image_vpp_convert(bm_handle_, 1, in, &out, &crop_rect);
}

long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

void Classifier::preForward(std::vector<cv::Mat> &images) {
#if 0
    images_.clear();
    for (size_t i = 0; i < images.size(); i++) {

#if USE_BMCV
        bm_image tmp1;
        bm_image tmp2;
        // cvMat -> bmimage
        cv::bmcv::toBMI(images[i], &tmp1, true);
        // reisze to (64,64)
        bm_image_create(bm_handle_, net_h_, net_w_, FORMAT_RGB_PLANAR, tmp1.data_type, &tmp2);
        auto ret = bmcv_image_vpp_convert(bm_handle_, 1, tmp1, &tmp2);
        assert(BM_SUCCESS == ret);
        ret = bmcv_image_gaussian_blur(bm_handle_, tmp2, processed_imgs_[i], 3, 3, 0, 0);
        assert(BM_SUCCESS == ret);
        bm_image_destroy(tmp1);
        bm_image_destroy(tmp2);
#elif USE_OPENCV2
        cv::Mat img1 = cv::Mat::zeros(cv::Size(64,64), CV_8UC3);
        cv::Mat img2;
        cv::bmcv::resize(images[0], img1);
        cv::GaussianBlur(img1, img2, cv::Size(3,3), 0, 0);
        // FORMAT_RGB_PACKED -> FORMAT_RGB_PLANER
        bm_image bmimg;
        cv::bmcv::toBMI(img2, &bmimg, true);
        preprocess(bmimg, processed_imgs_[i]);
        bm_image_destroy(bmimg);
#else

        bm_image bmimg;
        cv::Mat temp = cv::Mat::zeros(cv::Size(64,64), CV_8UC3);;
        cv::bmcv::uploadMat(images[i]);
        if (1)
        {
            cv::bmcv::resize(images[i], temp, true, BMCV_INTER_NEAREST);
            cv::bmcv::downloadMat(temp);
            cv::GaussianBlur(temp, temp, cv::Size(3, 3), 0, 0);
            cv::bmcv::uploadMat(temp);
        }
        else
        {
            temp = images[i];
        }

        bm_image_from_mat(bm_handle_, temp, bmimg);
        preprocess(bmimg, processed_imgs_[i]);
        bm_image_destroy(bmimg);
#endif
        images_.push_back(images[i]);

        //long current_time = getSystemTime();
        //std::string timetamp = std::to_string(current_time);
        //std::string timetamp_cv = timetamp + "_cv.jpg";
        //timetamp = timetamp + ".jpg";

        //imwrite("output_cv.jpg", images[i]);
        bm_image_write_to_bmp(processed_imgs_[i], "output.jpg");
    }

    bmcv_image_convert_to(bm_handle_, batch_size_, convert_attr_, &processed_imgs_[0], scaled_inputs_);
#endif
    if (images.size() == 0)
    {
        return;
    }
    cv::Mat temp;
    if (1)
    {
        temp = cv::Mat::zeros(net_h_, net_w_, images[0].type());
    }
    for (size_t i = 0; i < images.size(); i++)
    {
        bm_image bmimg;

        //long current_time = getSystemTime();
        //string timetamp = to_string(current_time);
        //string timetamp_cv = timetamp + "_cv.jpg";
        //timetamp = timetamp + ".jpg";

        //imwrite(timetamp_cv.c_str(), images[i]);
        //cout << "input size==============" << images[i].size() << timetamp_cv.c_str() << endl;

        cv::bmcv::uploadMat(images[i]);
        if (1)
        {
            cv::bmcv::resize(images[i], temp, true, BMCV_INTER_NEAREST);
            cv::bmcv::downloadMat(temp);
            cv::GaussianBlur(temp, temp, cv::Size(3, 3), 0, 0);
            cv::bmcv::uploadMat(temp);
        }
        else
        {
            temp = images[i];
        }

        bm_image_from_mat(bm_handle_, temp, bmimg);
        preprocess(bmimg, processed_imgs_[i]);
        bm_image_destroy(bmimg);

        // long current_time = getSystemTime();
        // string timetamp = to_string(current_time);
        // string timetamp_cv = timetamp + "_cv.jpg";
        // timetamp = timetamp + ".jpg";

        // imwrite(timetamp_cv.c_str(), images[i]);
        // bm_image_write_to_bmp(processed_imgs_[i], timetamp.c_str());
    }
    bmcv_image_convert_to(bm_handle_, batch_size_, convert_attr_, &processed_imgs_[0], scaled_inputs_);
}

bool Classifier::forward() {
    bool res = bm_inference(p_bmrt_, scaled_inputs_, outputs_, input_shape_,
                            reinterpret_cast<const char *>(net_names_[0]));
    if (!res) {
        std::cout << "ERROR : inference failed!!" << std::endl;
        return false;
    }
    return true;
}

std::vector<int> Classifier::sort_indexes_e(std::vector<float> &v) {
    std::vector<int> idx(v.size());
    iota(idx.begin(), idx.end(), 0);
    sort(idx.begin(), idx.end(),
         [&v](int i1, int i2) { return v[i1] > v[i2]; });
    return idx;
}

std::vector<classifier_result> Classifier::postForward() {
    std::vector<classifier_result> results;

    for (int batch_cnt = 0; batch_cnt < batch_size_; batch_cnt++) {
        std::vector<float *> blobs;

        for (int j = 0; j < output_num_; j++) {
            blobs.push_back(reinterpret_cast<float *>(outputs_[j]) + output_sizes_[j] * batch_cnt);
        }

        classifier_result result;
        std::vector<float> output;
        std::vector<float> sort;
        for (int c = 0; c < class_num_; c++) {
            output.push_back(blobs[0][c]);
        }
        std::vector<int> sort_index = sort_indexes_e(output);
        for (int i = 0; i < sort_index.size(); i++) {
            sort.push_back(output[sort_index[i]]);
        }
        result.scene = sort_index;
        result.conf = sort;
        results.push_back(result);
    }
    return results;
}

bool Classifier::Run(std::vector<cv::Mat> &input_images, std::vector<classifier_result> &results) {

    if (!input_images.size()) {
        std::cout << "ERROR : input 0 image!!" << std::endl;
        return false;
    }

    preForward(input_images);
    bool forward_status;
    forward_status = forward();

    if (forward_status == false) {
        return false;
    }

    std::vector<classifier_result> outputs;
    outputs = postForward();

    classifier_result result;
    for (int j = 0; j < outputs.size(); j++) {
        result.scene = outputs[j].scene;
        result.conf = outputs[j].conf;
    }
    results.push_back(result);

    return true;
}


bool Classifier::Run(cv::Mat &image, classifier_result &result) {
    input_images_.clear();
    if (image.type() == CV_8UC1) {
        cv::Mat bgr;
        cv::bmcv::toMAT(image, bgr);
        input_images_.push_back(bgr);
    } else if (image.type() == CV_8UC3) {
        input_images_.push_back(image);
    }

    preForward(input_images_);

    bool forward_status;
    forward_status = forward();

    if (forward_status == false) {
        return false;
    }

    std::vector<classifier_result> outputs;
    outputs = postForward();

    result.scene = outputs[0].scene;
    result.conf = outputs[0].conf;

    return true;
}

int main(int argc, char *argv[])
{
    Classifier predict("../issue2/compilation.bmodel");
    cv::Mat img1 = cv::imread("../images/test2.jpg");
    classifier_result result;
    predict.Run(img1, result);

    int n = result.scene.size();
    for(int i = 0;i <n; ++i) {
        printf("scene=%d, conf=%f\n", result.scene[i], result.conf[i]);
    }

    return 0;
}


