//
// Created by yuan on 5/8/21.
//
#include <stdlib.h>
#include <vector>
#include <assert.h>
#include <iostream>
#include "bmruntime_interface.h"

using Feature = std::vector<float>;
class FeatureMatch {
    bm_handle_t m_handle;
    int m_dev_id;
    void *m_bmrt;

public:
    FeatureMatch(int dev_id):m_dev_id(dev_id) {
        auto ret = bm_dev_request(&m_handle, dev_id);
        assert(BM_SUCCESS == ret);
        m_bmrt = bmrt_create(m_handle);
        if (!bmrt_load_bmodel(m_bmrt, "./topk_attr")) {
            std::cout << "load bmodel failed!" << std::endl;
            exit(0);
        }
    }

    ~FeatureMatch(){

    }

    int init(int db_size=10000) {
        // init db
        int db_size_bytes = db_size * 512 * sizeof(float);
        float *db_data = new float[db_size_bytes];
        for(int i = 0;i < db_size;++i) {
            for(int j= 0; j < 512; j++) {
                db_data[i] = 0.0001 * (rand() % 10000);
            }
        }



        return 0;
    }

    int topk(std::vector<Feature> feature_list, std::vector<int> &ids)
    {

    }
};



int main(int argc, char *argv[])
{
    //人脸特征值为512个fp32
    //输入【N,512】人脸数据
    //输出【N,k】

    int db_size = 200000; //20W底库
    int q_size = 4; //每次查询的批次





    return 0;
}