//
// Created by yuan on 5/7/21.
//
#include <vector>
#include <string.h>
#include "bmlang.h"

#define COMPILE_DEBUG

int main(int argc, char** argv) {
    // Init bmlang
#ifdef COMPILE_DEBUG
    bmlang::init(bmlang::BOTH, "BM1684", "./topk_attr");
#else
    bmlang::init(bmlang::COMPILE_TPU, "BM1684", "./topk_attr");
#endif

    // Test topk with attr selection
    std::vector<int> people_shape = {1000000}; // number of people
    std::vector<int> attr_shape = {1000000};

    bmlang::DataType attr_dtype = bmlang::FLOAT32;
    // Declare input
    // Input for database
    bmlang::Tensor people_index("People_index", bmlang::INT32, people_shape);
    bmlang::Tensor score("Score", bmlang::FLOAT32, people_shape);
    bmlang::Tensor cap_attr("cap_attr", attr_dtype, attr_shape); // 1: have cap, 0: haven't cap, x: other attr
    bmlang::Tensor gender_attr("gender_attr", attr_dtype, attr_shape); // data, 0: male, 1: female
    bmlang::Tensor id_attr("id_attr", attr_dtype, attr_shape); // id: 0, 1, 2, ...

    // Input for user
    int db_sel_num = 16; // select database id number
    bmlang::Tensor id_sel("id_sel", attr_dtype, db_sel_num);
    bmlang::Tensor cap_sel("cap_sel", attr_dtype, 1);
    bmlang::Tensor gender_sel("gender_sel", attr_dtype, 1);

    // Declare output
    bmlang::Tensor topk_score("topk_score", bmlang::FLOAT32);
    bmlang::Tensor topk_people_index("topk_index", bmlang::INT32);

#ifdef COMPILE_DEBUG
    // data set
    int* index_data = new int [people_shape[0]];
    for (int i = 0; i < people_shape[0]; ++i) {
      index_data[i] = i;
    }
    float* score_data = new float [people_shape[0]];
    for (int i = 0; i < people_shape[0]; ++i) {
      score_data[i] = 0.0001 * i;
    }
    float* cap_data = new float [people_shape[0]];
    for (int i = 0; i < people_shape[0]; ++i) {
      if (i < people_shape[0] / 2) {
        cap_data[i] = (i + 1) % 2;
      } else {
        cap_data[i] = i % 2;
      }
    }
    float* gender_data = new float [people_shape[0]];
    for (int i = 0; i < people_shape[0]; ++i) {
      if (i < people_shape[0] / 2) {
        gender_data[i] = i % 2;
      } else {
        gender_data[i] = (i+1) % 2;
      }
    }
    float* id_data = new float [people_shape[0]];
    for (int i = 0; i < people_shape[0]; ++i) {
      id_data[i] = i % 64;
    }
    people_index.set_data((char*)index_data);
    score.set_data((char*)score_data);
    cap_attr.set_data((char*)cap_data);
    gender_attr.set_data((char*)gender_data);
    id_attr.set_data((char*)id_data);

    delete [] index_data;
    delete [] score_data;
    delete [] cap_data;
    delete [] gender_data;
    delete [] id_data;

    // user input
    float* user_id_sel = new float [db_sel_num];
    for (int i = 0; i < db_sel_num; ++i) {
      user_id_sel[i] = i;
    }
    id_sel.set_data((char*)user_id_sel);

    float user_cap_sel = 1;
    cap_sel.set_data((char*)(&user_cap_sel));

    float user_gender_sel = 0;
    gender_sel.set_data((char*)(&user_gender_sel));
#endif

    // Split id
    std::vector<bmlang::Tensor*> id_split_v;
    for (int i = 0; i < db_sel_num; ++i) {
        bmlang::Tensor* id_split = new bmlang::Tensor(attr_dtype);
        id_split_v.push_back(id_split);
    }
    bmlang::split(id_sel, id_split_v, 0, db_sel_num);

    // Get the mask that belong user's id_sel, cap_sel and gender_sel
    // Get id_sel mask
    bmlang::Tensor id_sel_mask(attr_dtype);
    for (int i = 0; i < db_sel_num; ++i) {
        if (i == 0) {
            bmlang::eq(id_attr, *id_split_v[i], id_sel_mask);
        } else {
            bmlang::Tensor id_sel_mask0(attr_dtype);
            bmlang::eq(id_attr, *id_split_v[i], id_sel_mask0);
            bmlang::max(id_sel_mask, id_sel_mask0, id_sel_mask);
        }
    }

    // Get male and have cap 0/1 mask
    bmlang::Tensor gender_mask(attr_dtype);
    bmlang::eq(gender_attr, gender_sel, gender_mask);

    bmlang::Tensor attr_mask(attr_dtype);
    bmlang::mul(id_sel_mask, gender_mask, attr_mask);

    bmlang::Tensor cap_mask(attr_dtype);
    bmlang::eq(cap_attr, cap_sel, cap_mask);

    bmlang::mul(attr_mask, cap_mask, attr_mask);

    // Select score of the person
    bmlang::Tensor select_score(bmlang::FLOAT32);
    bmlang::masked_select(score, attr_mask, select_score);

    // Select people index
    bmlang::Tensor select_index(bmlang::INT32);
    bmlang::masked_select(people_index, attr_mask, select_index);

    // Select database id
    bmlang::Tensor select_id(attr_dtype);
    bmlang::masked_select(id_attr, attr_mask, select_id);

    // Topk for each selected database ID
    std::vector<bmlang::Tensor*> topk_score_v;
    std::vector<bmlang::Tensor*> topk_people_index_v;
    for (int i = 0; i < db_sel_num; ++i) {
        // Get id mask
        bmlang::Tensor id_mask(attr_dtype);
        bmlang::eq(select_id, *(id_split_v[i]), id_mask);

        // Mask select score
        bmlang::Tensor select_id_score(bmlang::FLOAT32);
        bmlang::masked_select(select_score, id_mask, select_id_score);

        bmlang::Tensor select_index_2rd(bmlang::INT32);
        bmlang::masked_select(select_index, id_mask, select_index_2rd);

        // Get the topk of this database ID
        bmlang::Tensor topk_index(bmlang::INT32);
        bmlang::Tensor topk_res(bmlang::FLOAT32);
        bmlang::TopkParam param;
        param.axis = 0;
        param.k = 100;
        param.descending = true;
        bmlang::topk(select_id_score, topk_res, topk_index, param);

        // Select the true people index
        bmlang::Tensor select_people_index(bmlang::INT32);
        bmlang::index_select(select_index_2rd, 0, topk_index, select_people_index);

        // Expand dimension for concat
        bmlang::Tensor* p_topk_res = new bmlang::Tensor(bmlang::FLOAT32);
        bmlang::Tensor* p_people_index = new bmlang::Tensor(bmlang::INT32);

        bmlang::ExpandDimsParam expand_param;
        expand_param.axis = 0;
        bmlang::expand_dims(topk_res, *p_topk_res, expand_param);
        bmlang::expand_dims(select_people_index, *p_people_index, expand_param);

        topk_score_v.push_back(p_topk_res);
        topk_people_index_v.push_back(p_people_index);
    }

    // concat as [db_sel_num, 100]
    bmlang::concat(topk_score_v, topk_score, 0);
    bmlang::concat(topk_people_index_v, topk_people_index, 0);
#ifdef COMPILE_DEBUG
    float* topk_score_data = new float [db_sel_num * 100];
    int* topk_index_data = new int [db_sel_num * 100];
    topk_score.get_data((char*)topk_score_data);
    topk_people_index.get_data((char*)topk_index_data);
    for (int i = 0; i < db_sel_num; ++i) {
      std::cout << "### Topk result for ID " << (int)user_id_sel[i] << std::endl;
      std::cout << "Topk score:" << std::endl;
      for (int j = 0; j < 100; ++j) {
        std::cout << topk_score_data[i * 100 + j] << " ";
      }
      std::cout << std::endl;
      std::cout << "Topk people index:" << std::endl;
      for (int j = 0; j < 100; ++j) {
        std::cout << topk_index_data[i * 100 + j] << " ";
      }
      std::cout << std::endl;
    }
    delete [] user_id_sel;
    delete [] topk_score_data;
    delete [] topk_index_data;
#endif


    // Compile to optimize and CodeGen
#ifndef COMPILE_DEBUG
    bmlang::compile("TopK_attr", 2, true, true);
#else
    std::vector<bmlang::Tensor*> inp_tensor;
    std::vector<bmlang::Tensor*> ref_tensor;
    inp_tensor.push_back(&people_index);
    inp_tensor.push_back(&score);
    inp_tensor.push_back(&cap_attr);
    inp_tensor.push_back(&gender_attr);
    inp_tensor.push_back(&id_attr);
    inp_tensor.push_back(&id_sel);
    inp_tensor.push_back(&cap_sel);
    inp_tensor.push_back(&gender_sel);

    ref_tensor.push_back(&id_sel_mask);
    ref_tensor.push_back(&attr_mask);
    for (int i = 0; i < db_sel_num; ++i) {
      ref_tensor.push_back(topk_score_v[i]);
      ref_tensor.push_back(topk_people_index_v[i]);
    }

    bmlang::compile_with_check("TopK_attr", inp_tensor,ref_tensor,2, true, true);
#endif
    // Deinit bmlang
    bmlang::deinit();

    for (auto e : topk_score_v) delete e;
    for (auto e : topk_people_index_v) delete e;
    for (auto e : id_split_v) delete e;

    return 0;
}