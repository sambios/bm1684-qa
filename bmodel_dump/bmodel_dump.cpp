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
            float scale = net_info->input_scales[i];
            int iv = in_value *scale;
            int8_t v = std::max(std::min(iv, -127), 127);
            memset(p_sys_data, v, tensor_bytes);
        }

        // System -> Device
        bm_memcpy_s2d_partial(bm_handle_, input_tensor_[i].device_mem, p_sys_data, tensor_bytes);
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
      auto tensor_ptr = output_tensor_ + i;
      int tensor_bytesize = bmrt_tensor_bytesize(tensor_ptr);
      int tensor_count = bmrt_shape_count(&tensor_ptr->shape);
      int8_t  *p_sys_data = new int8_t[tensor_bytesize];
      bm_memcpy_d2s_partial(bm_handle_, p_sys_data, output_tensor_[i].device_mem, tensor_bytesize);
      if (net_info->output_dtypes[i] == BM_FLOAT32) {
          std::string output_fname = cv::format("output_%s.data", net_info->output_names[i]);
          FILE *fp = fopen(output_fname.c_str(), "wb+");
          fwrite(p_sys_data, 1, tensor_bytesize, fp);
          fclose(fp);
      }else if (net_info->output_dtypes[i] == BM_INT8) {
          float_t *fp32 = new float[tensor_count];
          for(int j = 0;j < tensor_count; ++j) {
              fp32[j] = p_sys_data[j] * net_info->output_scales[i];
          }
          std::string output_fname = cv::format("output_%s.data", net_info->output_names[i]);
          FILE *fp = fopen(output_fname.c_str(), "wb+");
          fwrite(fp32, 1, tensor_count * sizeof(float), fp);
          fclose(fp);
          delete []fp32;
      }else{
          assert(0);
      }

      delete [] p_sys_data;
  }
}

