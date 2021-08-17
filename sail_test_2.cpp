//
// Created by yuan on 6/8/21.
//

#include <iostream>
#include <string>
#include <vector>
#include "engine.h"
#include <ctime>
#include <unistd.h>
#include "common.h"
using namespace std;

int main() {
    string bmodel_path = "/home/yuan/bitmain/rui-si-te/issue3/yolov5l_aqzy_i1280_int8.bmodel";
    //string img_path = "/home/linaro/RstDetectBtdl/data/2021_06_04/1014gz1.jpg";
    int input_size = 1280*1280*3*1;
    float *input = new float[input_size];
    for(int ii=0; ii< input_size; ii++){
        input[ii] = 1.0;
        printf(".");
    }
    int tpu_id = 0;
    sail::Engine engine(tpu_id);
    int ret = engine.load(bmodel_path);
    cout<<"ret "<<ret<<endl;
    auto graph_name = engine.get_graph_names().front();
    auto input_name = engine.get_input_names(graph_name).front();
    auto input_shape = engine.get_input_shape(graph_name, input_name);
    auto in_dtype = engine.get_input_dtype(graph_name, input_name);
    auto output_name0 = engine.get_output_names(graph_name)[0];
    auto output_shape0 = engine.get_output_shape(graph_name, output_name0);
    auto output_name1 = engine.get_output_names(graph_name)[1];
    auto output_shape1 = engine.get_output_shape(graph_name, output_name1);
    auto output_name2 = engine.get_output_names(graph_name)[2];
    auto output_shape2 = engine.get_output_shape(graph_name, output_name2);
    auto out_dtype = engine.get_output_dtype(graph_name, output_name0);
    sail::Handle handle = engine.get_handle();
    cout<<"input_shape size "<<input_shape.size()<<input_shape[0]<<input_shape[1]<<input_shape[2]<<input_shape[3]<<endl;
    cout<<"in_dtype "<<in_dtype<<endl;
    cout<<"out_dtype "<<out_dtype<<endl;
    //sail::Bmcv bmcv(handle);
    sail::Tensor in(handle, input_shape, in_dtype, true, true);
    sail::Tensor out0(handle, output_shape0, out_dtype, true, true);
    sail::Tensor out1(handle, output_shape1, out_dtype, true, true);
    sail::Tensor out2(handle, output_shape2, out_dtype, true, true);
    int out_size = 1;
    for(auto it : output_shape0) out_size = out_size*it;
    float* output0 = (float *) reinterpret_cast<float*>(out1.sys_data());
    cout<<"out_size0 "<<out_size<<" "<<output0[0]<<" "<<output0[1]<<endl;
    std::map<string, sail::Tensor*> input_tensors = {{input_name, &in}};
    std::map<string, sail::Tensor*> output_tensors = {{output_name0, &out0}, {output_name1, &out1}, {output_name2, &out2}};
    //float* output = new float[out_size];
    cout<<"out_size "<<out_size<<" "<<endl;
    engine.set_io_mode(graph_name, sail::SYSIO);
    float input_scale = engine.get_input_scale(graph_name, input_name);
#if 0
    in.scale_from(input, input_scale, input_size);
    char* inp = (char*) in.sys_data();
    cout<<"inp "<<(int)(*inp)<<endl;
    cout<<"input_scale "<<input_scale<<endl;
    cout<<"graph_name "<<graph_name<<endl;
    cout<<"output_name0 "<<output_name0<<endl;
    cout<<"output_name1 "<<output_name1<<endl;
    cout<<"output_name2 "<<output_name2<<endl;
    engine.process(graph_name, input_tensors, output_tensors);
    float* output1 = (float *) reinterpret_cast<float*>(out1.sys_data());
    cout<<"out_size1 "<<out_size<<" "<<output1[0]<<" "<<output1[1]<<endl;
    //float* target = reinterpret_cast<float*>(out0.sys_data());
    //memcpy(output, target, out_size * sizeof(float));
    //engine.scale_output_tensor(graph_name, output_name0, output);


#endif
    //for(int ii=0;ii<input_size;ii++) input[ii] = 1.0;
    in.scale_from(input, input_scale, input_size);
    char *inp = (char*) in.sys_data();
    cout<<"inp1 "<<(uint32_t)(*inp)<<endl;
    engine.process(graph_name, input_tensors, output_tensors);
    if (!out1.own_sys_data()) {
        out1.sync_d2s();
    }

    std::vector<int> out_shape = out0.shape();
    int size_shape  = std::accumulate(out_shape.begin(), out_shape.end(),
                                      1, std::multiplies<int>());

    float output_scale = engine.get_output_scale(graph_name, output_name0);
    float *output_data = new float[size_shape];
    out0.scale_to(output_data, output_scale, size_shape);
    bm::save_to_file("sail_test2_output0.data", output_data, size_shape*sizeof(float));
    delete []output_data;

    //float* output2 = (float *) reinterpret_cast<float*>(out1.sys_data());
    //cout<<"out_size2 "<<out_size<<" "<<output2[0]<<" "<<output2[1]<<endl;

    //bm::save_to_file("sail_test2_output_fp32.data", output2, )
}













