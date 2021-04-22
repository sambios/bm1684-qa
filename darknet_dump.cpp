//
// Created by yuan on 4/22/21.
//

#ifndef DARKNET_DARKNET_CLASSIFIER_H
#define DARKNET_DARKNET_CLASSIFIER_H
#include <iostream>

#include "darknet.h"

class DarknetClassifier {
    std::string m_model_cfg;
    std::string m_model_weight;
    network *m_network;
public:
    DarknetClassifier(){

    }

    ~DarknetClassifier() {

    }

    int load_model(const std::string& cfg, const std::string& weight){
        m_network = load_network((char*)cfg.c_str(), (char*)weight.c_str(), 0);
        set_batch_network(m_network, 1);

        return 0;
    }

    int forward(){
        float *input_data = new float[416*416*3];
        memset(input_data, 0, 416*416*3);
        network_predict(m_network, input_data);
        int yolo_index = 0;
        for( int i = 0;i < m_network->n; ++i) {
            layer l = m_network->layers[i];
            if (l.type == YOLO) {
                printf("Yolo%d n = %d, c=%d, h=%d, w=%d\n", i, l.batch, l.c, l.h, l.w);
                {
                    std::string file = "Yolo";
                    file += std::to_string(yolo_index) + ".data";
                    FILE *fp = fopen(file.c_str(), "wb");
                    int size = l.batch*l.c*l.h*l.w;
                    fwrite(l.output, 1, size * sizeof(float), fp);
                    fclose(fp);
                    yolo_index++;
                }
            }
        }
    }


};



#endif //DARKNET_DARKNET_CLASSIFIER_H
