//
// Created by yuan on 9/1/21.
//

#include "bmlib_runtime.h"
#include <cstring>

int main(int argc, char *argv[])
{
    bm_handle_t handle;
    bm_dev_request(&handle, 0);


    bm_mem_desc_t mem;
    memset(&mem, 0, sizeof(mem));
    bm_malloc_device_byte(handle, &mem, 100);
    bm_free_device(handle, mem);

    bm_dev_free(handle);
}