#ifndef SSD_HPP
#define SSD_HPP

#include <string>
#include <opencv2/opencv.hpp>
#include "bmruntime_interface.h"
#include "utils.hpp"

class BModelDump {
public:
  BModelDump(void *bmrt, bm_handle_t bm_handle);
  ~BModelDump();
  void set_param(int netIdx, int stageIdx){
      net_idx_ = netIdx;
      stage_idx_ = stageIdx;
  }
  void preForward(int in_value=1);
  void forward();
  void postForward();

private:
  // runtime helper
  const char **net_names_;
  void *p_bmrt_;
  bm_handle_t bm_handle_;
  int stage_idx_;
  int net_idx_;

  // input & output buffers
  int output_num_;
  int input_num_;
  bm_tensor_t *input_tensor_;
  bm_tensor_t *output_tensor_;
};





#endif /* SSD_HPP */
