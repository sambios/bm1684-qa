//
// Created by yuan on 7/8/21.
//

#include "engine.h"
#include "tensor.h"


bool inference(
        const std::string& bmodel_path,
        int                tpu_id) {
    // init Engine
    sail::Engine engine(tpu_id);
    sail::Handle handle = engine.get_handle();

    // load bmodel without builtin input and output tensors
    engine.load(bmodel_path);
    // get model info
    // only one model loaded for this engine
    // only one input tensor and only one output tensor in this graph
    auto graph_name = engine.get_graph_names().front();
    auto input_names = engine.get_input_names(graph_name);
    auto output_names = engine.get_output_names(graph_name);

    std::vector<int> input_shape0 = {4, 32, 3, 256, 455};
    std::vector<int> input_shape1 = {4, 1, 30, 5};
    std::vector<int> input_shape2 = {4, 8, 3, 256, 455};
    std::vector<std::vector<int>> shapes;
    shapes.push_back(input_shape0);
    shapes.push_back(input_shape1);
    shapes.push_back(input_shape2);

    std::map<std::string, sail::Tensor*> input_tensors;
    std::map<std::string, sail::Tensor*> output_tensors;


    for(int i = 0;i <  input_names.size(); ++i) {
        sail::Tensor *tensor= new sail::Tensor(handle, shapes[i], BM_INT8, true, true);
        input_tensors[input_names[i]] = tensor;
    }

    for(int i = 0; i< output_names.size(); ++i) {
        output_tensors[output_names[i]] = new sail::Tensor(handle, {4, 80}, BM_INT8, true, true);
    }


    engine.process(graph_name, input_tensors, output_tensors);

    return 0;
}

int main(int argc, char *argv[])
{
   //inference("/home/yuan/tmp/4batch.bmodel", 0);
   bm_handle_t bmHandle;
   int ret;
   bm_dev_request(&bmHandle,  0);
   std::vector<bm_device_mem_t> vct_mem;
   for(int i = 0; i < 1000000000; ++i) {
       bm_device_mem_t mem;
       ret = bm_malloc_device_byte_heap_mask(bmHandle, &mem, 4, 1000000);
       if (ret != 0) {
           printf("malloc failed\n");
           break;
       }
       vct_mem.push_back(mem);
   }

   getchar();





}