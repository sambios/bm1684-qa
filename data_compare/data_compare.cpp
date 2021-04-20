//
// Created by yuan on 4/20/21.
//

#include <iostream>
#include <assert.h>
#include "opencv2/opencv.hpp"

class DataCompare {
    FILE *fp1;
    FILE *fp2;
public:
    DataCompare(const char *f1, const char *f2){
        fp1 = fopen(f1, "rb");
        fp2 = fopen(f2, "rb");
    }

    ~DataCompare()
    {
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
    }

    float distance(bool print_detail)
    {
        fseek(fp1, 0, SEEK_END);
        int size_byte = ftell(fp1);
        fseek(fp1, 0, SEEK_SET);

        float *f1 = new float[size_byte];
        float *f2 = new float[size_byte];

        int len = fread(f1, 1, size_byte, fp1);
        assert(len == size_byte);
        len = fread(f2, 1, size_byte, fp2);
        assert(len == size_byte);

        int size_float = size_byte / sizeof(float);
        cv::Mat a(1, size_float, CV_32F, f1);
        cv::Mat b(1, size_float, CV_32F, f2);

        double dist = cv::norm(a, b, cv::NORM_INF);
        printf("dist = %f\n", dist);
        if (print_detail) {
            for (int i = 0; i < size_float; ++i) {
                if (i % 8 == 0) printf("\n");
                printf("%8f\t", f1[i] - f2[i]);
            }
        }

        delete []f1;
        delete []f2;

        return 0;
    }
};


int main(int argc, char *argv[])
{
    if (argc <= 3) {
        std::cout << "USAGE:" << std::endl;
        std::cout << "  " << argv[0] << " <file1 path> <file2 path> <1:print_detail>" << std::endl;
        exit(1);
    }

    //DataCompare cmp("./output_Yolo0.data", "/home/yuan/tmp/ToResolve3/yolos/Yolo0.data");
    DataCompare cmp(argv[1], argv[2]);
    bool print_detail = false;
    if (argc > 3) print_detail = std::stoi(argv[3]);

    cmp.distance(print_detail != 0);
}

