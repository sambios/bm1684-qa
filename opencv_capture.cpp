//
// Created by yuan on 1/11/22.
//

#include "opencv2/opencv.hpp"
int main(int argc, char *argv[])
{
    std::string filepath = "rtsp://admin:hk123456@11.73.12.20";
    cv::VideoCapture cap(filepath, cv::CAP_ANY, 0);
    int num = 0;
    while(num < 50) {
        cv::Mat img;
        bool ok = cap.read(img);
        if (!ok) {
            std::cout << "read error!" << "\n";
            break;
        }
        num++;
        std::cout << "capture " << num << "\n";
    }

    cap.release();
}