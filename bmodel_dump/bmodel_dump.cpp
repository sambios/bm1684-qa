#include "bmodel_dump.hpp"

BModelDump::BModelDump(void *bmrt, bm_handle_t bm_handle) {
    p_bmrt_ = bmrt;
    bm_handle_ = bm_handle;
    bm_status_t status = BM_SUCCESS;

    // as of a simple example, assume the model file contains one BModelDump network only
    bmrt_get_network_names(p_bmrt_, &net_names_);
    std::cout << "> get model " << net_names_[0] << " successfully" << std::endl;
}

BModelDump::~BModelDump() {
    for (int i = 0; i < input_num_; ++i) {
        bm_free_device(bm_handle_, input_tensor_[i].device_mem);
    }
    for (int i = 0; i < output_num_; ++i) {
        bm_free_device(bm_handle_, output_tensor_[i].device_mem);
    }

    free(net_names_);
}


void BModelDump::preForward(int in_value) {
    auto net_info = bmrt_get_network_info(p_bmrt_, net_names_[0]);

    input_num_ = net_info->input_num;
    input_tensor_ = new bm_tensor_t[input_num_];
    for (int i = 0; i < input_num_; ++i) {
        auto &input_shape = net_info->stages[0].input_shapes[i];
        bmrt_tensor(&input_tensor_[i], p_bmrt_, net_info->input_dtypes[i], input_shape);
        int tensor_bytes = bmrt_tensor_bytesize(&input_tensor_[i]);
        int shape_count = bmrt_shape_count(&input_shape);
        int8_t *p_sys_data = new int8_t[tensor_bytes];
        memset(p_sys_data, 0, tensor_bytes);
        if (input_tensor_[i].dtype == BM_FLOAT32) {
            float *p = (float*)p_sys_data;
            for(int j = 0; j < shape_count;j++) {
                p[j] = in_value;
            }
        }else if (input_tensor_[i].dtype == BM_INT8) {
            memset(p_sys_data, in_value, tensor_bytes);
        }

        // System -> Device
        bm_memcpy_s2d(bm_handle_, input_tensor_[i].device_mem, p_sys_data);
        std::string fname = cv::format("input_%s.data", net_info->input_names[i]);
        FILE *fp = fopen(fname.c_str(), "wb");
        fwrite(p_sys_data, 1, tensor_bytes, fp);
        fclose(fp);
    }

    output_num_ = net_info->output_num;
    output_tensor_ = new bm_tensor_t[output_num_];
    for (int i = 0; i < output_num_; ++i) {
        auto &output_shape = net_info->stages[0].output_shapes[i];
        bmrt_tensor(&output_tensor_[i], p_bmrt_, net_info->output_dtypes[i], output_shape);
    }
}

void BModelDump::forward() {
  bm_status_t status = BM_SUCCESS;

  bool ret = bmrt_launch_tensor_ex(p_bmrt_, net_names_[0], input_tensor_,
                                   input_num_, output_tensor_, output_num_, true, false);
  if (!ret) {
    std::cout << "ERROR: Failed to launch network" << net_names_[0] << "inference" << std::endl;
  }

  status = bm_thread_sync(bm_handle_);
}

void BModelDump::postForward() {
  auto net_info = bmrt_get_network_info(p_bmrt_, net_names_[0]);
  for(int i = 0; i < output_num_; ++i) {
      int tensor_bytesize = bmrt_tensor_bytesize(output_tensor_ + i);
      int8_t  *p_sys_data = new int8_t[tensor_bytesize];
      bm_memcpy_d2s_partial(bm_handle_, p_sys_data, output_tensor_[i].device_mem, tensor_bytesize);

      std::string output_fname = cv::format("output_%s.data", net_info->output_names[i]);
      FILE *fp = fopen(output_fname.c_str(), "wb+");
      fwrite(p_sys_data, 1, tensor_bytesize, fp);
      fclose(fp);

      delete [] p_sys_data;
  }
}

