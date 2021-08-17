//
// Created by yuan on 6/24/21.
//

#include "opencv2/opencv.hpp"
#include "utils.hpp"
#include <thread>

int main(int argc, char *argv[])
{
    TimeStamp ts;
    int flag = cv::IMREAD_AVFRAME;

    for(int i = 0; i < 1000; i++) {
        {
            cv::SophonDevice device(0);
            cv::Mat image1(device);
            ts.save("1680x1050");
            image1 = cv::imread("/home/yuan/bitmain/feng-huo-xing-kong/images/1680x1050.jpg", flag);
            ts.save("1680x1050");
        }

        {
            cv::SophonDevice device(0);
            cv::Mat image1(device);
            ts.save("1024x680");
            image1 = cv::imread("/home/yuan/bitmain/feng-huo-xing-kong/images/1024x680.jpg", flag);
            ts.save("1024x680");
        }

        {
            cv::SophonDevice device(0);
            cv::Mat image1(device);
            ts.save("650x427");
            image1 = cv::imread("/home/yuan/bitmain/feng-huo-xing-kong/images/650x427.jpg", flag);
            ts.save("650x427");
        }

        {
            cv::SophonDevice device(0);
            cv::Mat image1(device);
            ts.save("440x300");
            image1 = cv::imread("/home/yuan/bitmain/feng-huo-xing-kong/images/440x300.jpg", flag);
            ts.save("440x300");
        }

        {
            cv::SophonDevice device(0);
            cv::Mat image1(device);
            ts.save("160x117");
            image1 = cv::imread("/home/yuan/bitmain/feng-huo-xing-kong/images/160x117.jpg", flag);
            ts.save("160x117");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (1){
            cv::SophonDevice device(0);
            cv::Mat image1(device);
            ts.save("13x30");
            image1 = cv::imread("/home/yuan/bitmain/feng-huo-xing-kong/images/13x30.jpg", flag);
            ts.save("13x30");
        }
    }


    cv::Mat m;
    cv::transpose()

    ts.show_timeline();
    ts.show_summary("opencv imread");

}