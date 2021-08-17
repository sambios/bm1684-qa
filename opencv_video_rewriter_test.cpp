//
// Created by yuan on 5/24/21.
//
#include "opencv2/opencv.hpp"
#include "utils.hpp"
#include <thread>

int main(int argc, char *argv[])
{
    const char *keys ={
            "{f | /home/yuan/yanxi-1080p-2M.mp4   | test image file path }"
    };

    cv::CommandLineParser parser(argc, argv, keys);
    const std::string filepath = parser.get<std::string>("f");
    cv::Mat img;
    cv::VideoCapture cap(filepath, cv::CAP_ANY, 0);
    //av_log_set_level(AV_LOG_DEBUG);
    //int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    //int fourcc = cv::VideoWriter::fourcc('H', '2', '6', '4');
    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    //int fourcc = cv::VideoWriter::fourcc('h', 'e', 'v', '1'); //ok
    cv::VideoWriter writer;
    std::string enc_params = "max_qp=44:bitrate=1000";
    writer.open("test2.mp4", fourcc, 25, cv::Size(1920,1080), enc_params);
    //writer.open("test2.mp4", fourcc, 25, cv::Size(1920,1080));
    writer.set(cv::VIDEOWRITER_PROP_QUALITY, 0.5);
    int num = 0;
    TimeStamp ts;
    while(num < 50) {
        bool ok = cap.read(img);
        if (!ok) {
            std::cout << "read error!" << "\n";
            break;
        }
        ts.save("writer.write");
        writer.write(img);
        ts.save("writer.write");
        num++;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        img.release();
    }
    std::cout << num << "\n";
    ts.show_summary("writer.write");
    return 0;
}