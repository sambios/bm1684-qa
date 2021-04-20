#include <boost/filesystem.hpp>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include "bmodel_dump.hpp"

namespace fs = boost::filesystem;
using namespace std;

int main(int argc, char **argv) {

    bm_handle_t bm_handle;
    void *p_bmrt;
    cout.setf(ios::fixed);

    if (argc != 3) {
        cout << "USAGE:" << endl;
        cout << "  " << argv[0] << "<bmodel path> <value>" << endl;
        exit(1);
    }

    std::string bmodel_file = argv[1];
    int set_value = atoi(argv[2]);

    if (!fs::exists(bmodel_file)) {
        cout << "Cannot find valid model file." << endl;
        exit(1);
    }

    //1. create device handle
    bm_status_t status = bm_dev_request(&bm_handle, 0);
    if (BM_SUCCESS != status) {
        std::cout << "ERROR: bm_dev_request failed, ret:" << status << std::endl;
        exit(-1);
    }

    //2. create inference runtime handle
    p_bmrt = bmrt_create(bm_handle);
    if (NULL == p_bmrt) {
        std::cout << "ERROR: bmrt_create failed" << std::endl;
        exit(-1);
    }

    //3. load bmodel by file
    bool flag = bmrt_load_bmodel(p_bmrt, bmodel_file.c_str());
    if (!flag) {
        std::cout << "ERROR: Load bmodel[" << bmodel_file << "] failed" << std::endl;
        exit(-1);
    }

    BModelDump net(p_bmrt, bm_handle);
    
    //4. detect process
    net.preForward(set_value);
    net.forward();
    net.postForward();

    //5. cleanup resource
    bmrt_destroy(p_bmrt);
    bm_dev_free(bm_handle);

    return 0;
}
