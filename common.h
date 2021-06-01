//
// Created by yuan on 5/31/21.
//

#ifndef BM1684_QA_COMMON_H
#define BM1684_QA_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include "bmcv_api_ext.h"

namespace bm {
    static void save_to_file(const std::string& filepath, void *p, size_t size)
    {
        FILE *fp = fopen(filepath.c_str(), "wb");
        fwrite(p,1, size, fp);
        fclose(fp);
    }

    static void save_bmimage(const std::string& filepath, bm_image *img)
    {
        FILE *fp = fopen(filepath.c_str(), "wb");
        bm_image_format_info_t fmt_info;
        bm_image_get_format_info(img, &fmt_info);
        bm_handle_t handle = bm_image_get_handle(img);
        for(int i = 0; i< fmt_info.plane_nb; ++i) {
            int size = fmt_info.height* fmt_info.stride[i];
            void *pSystemData = new int8_t[size];
            bm_memcpy_d2s_partial(handle, pSystemData, fmt_info.plane_data[i], size);
            //Save Data
            fwrite(pSystemData, 1, size, fp);
            delete[]pSystemData;
        }

        fclose(fp);
    }

    //
    static inline bm_status_t avframe_to_bmimage(bm_handle_t bm_handle,
                                                   const AVFrame *pAVFrame,
                                                   bm_image &out, bool bToYUV420p=false, bool isNewAPI=false) {
        bm_status_t ret;
        const AVFrame &in = *pAVFrame;
        int offset = 0;
        if (isNewAPI) {
            offset = 4;
            if (in.format != AV_PIX_FMT_BMCODEC) {
                std::cout << "format don't support" << std::endl;
                return BM_NOT_SUPPORTED;
            }
        }else{
            if (in.format != AV_PIX_FMT_NV12) {
                std::cout << "format don't support" << std::endl;
                return BM_NOT_SUPPORTED;
            }
        }



        if (isNewAPI) offset = 4;

        if (in.channel_layout == 101) { /* COMPRESSED NV12 FORMAT */
            /* sanity check */
            if ((0 == in.height) || (0 == in.width) || (0 == in.linesize[4-offset]) || (0 == in.linesize[5-offset]) ||
                (0 == in.linesize[6-offset]) || (0 == in.linesize[7-offset]) || (0 == in.data[4-offset]) ||
                (0 == in.data[5-offset]) || (0 == in.data[6-offset]) || (0 == in.data[7-offset])) {
                std::cout << "bm_image_from_frame: get yuv failed!!" << std::endl;
                return BM_ERR_PARAM;
            }

            bm_image cmp_bmimg;
            ret = bm_image_create(bm_handle,
                                  in.height,
                                  in.width,
                                  FORMAT_COMPRESSED,
                                  DATA_TYPE_EXT_1N_BYTE,
                                  &cmp_bmimg);
            assert(BM_SUCCESS == ret);
            /* calculate physical address of avframe */
            bm_device_mem_t input_addr[4];
            int size = in.height * in.linesize[4-offset];
            input_addr[0] = bm_mem_from_device((unsigned long long) in.data[6-offset], size);
            size = (in.height / 2) * in.linesize[5-offset];
            input_addr[1] = bm_mem_from_device((unsigned long long) in.data[4-offset], size);
            size = in.linesize[6-offset];
            input_addr[2] = bm_mem_from_device((unsigned long long) in.data[7-offset], size);
            size = in.linesize[7-offset];
            input_addr[3] = bm_mem_from_device((unsigned long long) in.data[5-offset], size);
            ret = bm_image_attach(cmp_bmimg, input_addr);
            assert(BM_SUCCESS == ret);

            if (!bToYUV420p) {
                out = cmp_bmimg;
            }else{
                ret = bm_image_create(bm_handle,
                                      in.height,
                                      in.width,
                                      FORMAT_YUV420P,
                                      DATA_TYPE_EXT_1N_BYTE,
                                      &out);
                assert(BM_SUCCESS == ret);
                ret = bm_image_alloc_dev_mem(out, BMCV_HEAP1_ID);
                assert(BM_SUCCESS == ret);
                int ret = bmcv_image_vpp_convert(bm_handle, 1, cmp_bmimg, &out);
                assert(BM_SUCCESS == ret);
                bm_image_destroy(cmp_bmimg);
            }

        } else { /* UNCOMPRESSED NV12 FORMAT */
            /* sanity check */
            if ((0 == in.height) || (0 == in.width) || (0 == in.linesize[4-offset]) || (0 == in.linesize[5-offset]) || \
                (0 == in.data[4-offset]) || (0 == in.data[5-offset])) {
                std::cout << "bm_image_from_frame: get yuv failed!!" << std::endl;
                return BM_ERR_PARAM;
            }

            /* create bm_image with YUV-nv12 format */
            int stride[2];
            stride[0] = in.linesize[4-offset];
            stride[1] = in.linesize[5-offset];
            bm_image cmp_bmimg;
            bm_image_create(bm_handle,
                            in.height,
                            in.width,
                            FORMAT_NV12,
                            DATA_TYPE_EXT_1N_BYTE,
                            &cmp_bmimg,
                            stride);

            /* calculate physical address of yuv mat */
            bm_device_mem_t input_addr[2];
            int size = in.height * stride[0];
            input_addr[0] = bm_mem_from_device((unsigned long long) in.data[4-offset], size);
            size = in.height * stride[1];
            input_addr[1] = bm_mem_from_device((unsigned long long) in.data[5-offset], size);

            /* attach memory from mat to bm_image */
            bm_image_attach(cmp_bmimg, input_addr);

            if (!bToYUV420p) {
                out = cmp_bmimg;
            }else{
                ret = bm_image_create(bm_handle,
                                      in.height,
                                      in.width,
                                      FORMAT_YUV420P,
                                      DATA_TYPE_EXT_1N_BYTE,
                                      &out);
                assert(BM_SUCCESS == ret);
                ret = bm_image_alloc_dev_mem(out, BMCV_HEAP1_ID);
                assert(BM_SUCCESS == ret);
                int ret = bmcv_image_vpp_convert(bm_handle, 1, cmp_bmimg, &out);
                assert(BM_SUCCESS == ret);
                bm_image_destroy(cmp_bmimg);
            }
        }

        return BM_SUCCESS;
    }


}



#endif //BM1684_QA_COMMON_H
