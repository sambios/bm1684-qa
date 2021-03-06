cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 14)
set(TOP ${CMAKE_SOURCE_DIR})

if (NOT DEFINED TARGET_ARCH)
    set(TARGET_ARCH x86)
endif()

option(USE_BMNNSDK2 "Use bmnnsdk2 as sdk root" ON)
option(USE_DEV_OPENCV "Use Develop opencv" OFF)

# User need to set this before compiling.
if (USE_BMNNSDK2)
    if (DEFINED ENV{REL_TOP})
        set(BMNNSDK2_TOP $ENV{REL_TOP})
    else ()
        set(BMNNSDK2_TOP $ENV{HOME}/bmnnsdk2/bmnnsdk2-latest)
    endif ()

    set(bmnnsdk_inc_dirs ${BMNNSDK2_TOP}/include
            ${BMNNSDK2_TOP}/include/bmruntime
            ${BMNNSDK2_TOP}/include/bmlang
            ${BMNNSDK2_TOP}/include/bmlib
            ${BMNNSDK2_TOP}/include/third_party/boost/include
            ${BMNNSDK2_TOP}/include/sail
            )

    set(opencv_inc_dirs ${BMNNSDK2_TOP}/include/opencv/opencv4)
    set(ffmpeg_inc_dirs ${BMNNSDK2_TOP}/include/ffmpeg)


    if (${TARGET_ARCH} STREQUAL "x86")
        set(bmnnsdk_link_dirs ${BMNNSDK2_TOP}/lib/bmnn/pcie
                ${BMNNSDK2_TOP}/lib/bmlang
                ${BMNNSDK2_TOP}/lib/bmcompiler
                ${BMNNSDK2_TOP}/lib/thirdparty/x86
                ${BMNNSDK2_TOP}/lib/sail/pcie
                )
        set(opencv_link_dirs ${BMNNSDK2_TOP}/lib/opencv/x86)
        set(ffmpeg_link_dirs ${BMNNSDK2_TOP}/lib/ffmpeg/x86
                ${BMNNSDK2_TOP}/lib/decode/x86
                )
    elseif (${TARGET_ARCH} STREQUAL "aarch64")
        set(bmnnsdk_link_dirs ${BMNNSDK2_TOP}/lib/bmnn/arm_pcie
                ${BMNNSDK2_TOP}/lib/thirdparty/soc
		   )
        set(opencv_link_dirs ${BMNNSDK2_TOP}/lib/opencv/arm_pcie)
        set(ffmpeg_link_dirs ${BMNNSDK2_TOP}/lib/ffmpeg/arm_pcie
                ${BMNNSDK2_TOP}/lib/decode/arm_pcie
                )
    elseif (${TARGET_ARCH} STREQUAL "mips64")
        set(bmnnsdk_link_dirs ${BMNNSDK2_TOP}/lib/bmnn/mips64
                ${BMNNSDK2_TOP}/lib/thirdparty/mips64
                )
        set(opencv_link_dirs ${BMNNSDK2_TOP}/lib/opencv/mips64)
        set(ffmpeg_link_dirs ${BMNNSDK2_TOP}/lib/ffmpeg/mips64
                ${BMNNSDK2_TOP}/lib/decode/mips64
                )
    endif ()
endif(USE_BMNNSDK2)

if(USE_DEV_OPENCV)
    if (${TARGET_ARCH} STREQUAL "x86")
        set(MW_ROOT /home/yuan/bitmain/work/middleware-soc/install/pcie_bm1684_asic)
    elseif (${TARGET_ARCH} STREQUAL "aarch64")
        set(MW_ROOT /home/linaro/pcie_arm64_bm1684_asic)
    elseif (${TARGET_ARCH} STREQUAL "mips64")
        set(MW_ROOT /home/linaro/pcie_mips64_bm1684_asic)
    elseif (${TARGET_ARCH} STREQUAL "soc")
        set(MW_ROOT /home/linaro/soc_bm1684_asic)
    endif()

    set(opencv_inc_dirs ${MW_ROOT}/opencv/include/opencv4)
    set(ffmpeg_inc_dirs ${MW_ROOT}/ffmepg/usr/local/include)
    set(opencv_link_dirs ${MW_ROOT}/opencv/lib)
    set(ffmpeg_link_dirs ${MW_ROOT}/ffmpeg/usr/local/lib
            ${MW_ROOT}/decode/lib
            )
endif()

set(bmnnsdk_inc_dirs ${bmnnsdk_inc_dirs} ${opencv_inc_dirs} ${ffmpeg_inc_dirs})
set(bmnnsdk_link_dirs ${bmnnsdk_link_dirs} ${opencv_link_dirs} ${ffmpeg_link_dirs})


set(bmnnsdk_link_libs bmrt bmlib bmcv bmlang bmcompiler
        opencv_core opencv_imgproc opencv_imgcodecs opencv_videoio opencv_dnn
        avformat avfilter avcodec avutil avdevice  swresample swscale
        bmvideo bmjpuapi bmjpulite
        boost_system boost_filesystem protobuf gflags glog pthread)

message(STATUS "bmnnsdk_inc_dirs=${bmnnsdk_inc_dirs}")
message(STATUS "bmnnsdk_link_dirs=${bmnnsdk_link_dirs}")
message(STATUS "bmnnsdk_link_libs=${bmnnsdk_link_libs}")

include_directories(${bmnnsdk_inc_dirs})
link_directories(${bmnnsdk_link_dirs})

add_executable(bmodel_dump bmodel_dump/main.cpp bmodel_dump/bmodel_dump.cpp)
target_link_libraries(bmodel_dump ${bmnnsdk_link_libs})

add_executable(data_compare data_compare/data_compare.cpp)
target_link_libraries(data_compare ${bmnnsdk_link_libs})

add_executable(bmimage_to_gray bmimage_to_gray/bmimage_to_gray.cpp)
target_link_libraries(bmimage_to_gray ${bmnnsdk_link_libs})

add_executable(opencv_gray_issue1 opencv_gray_issue1.cpp)
target_link_libraries(opencv_gray_issue1 ${bmnnsdk_link_libs})

add_executable(issue2 issue2/classifier.cpp)
target_link_libraries(issue2 ${bmnnsdk_link_libs})

add_executable(opencv_roi_resize opencv_roi_resize.cpp)
target_link_libraries(opencv_roi_resize ${bmnnsdk_link_libs})

add_executable(bmcv_save_bmimg bmcv_save_bmimg.cpp)
target_link_libraries(bmcv_save_bmimg ${bmnnsdk_link_libs})

add_executable(chuanda-001 chuanda-001.cpp)
target_link_libraries(chuanda-001 ${bmnnsdk_link_libs})

add_executable(bmlang_topk_runtime bmlang_topk_runtime.cpp)
target_link_libraries(bmlang_topk_runtime ${bmnnsdk_link_libs})

add_executable(opencv_video_rewriter_test opencv_video_rewriter_test.cpp)
target_link_libraries(opencv_video_rewriter_test ${bmnnsdk_link_libs})

add_executable(sail_test_1 sail_test_1.cpp)
target_link_libraries(sail_test_1 sail ${bmnnsdk_link_libs})

add_executable(opencv_bmcv_verify opencv_bmcv_verify.cpp)
target_link_libraries(opencv_bmcv_verify ${bmnnsdk_link_libs})

add_executable(ffmpeg_decode ffmpeg_decode.cpp)
target_link_libraries(ffmpeg_decode ${bmnnsdk_link_libs})

add_executable(bmcv_test_1 bmcv_test_1.cpp)
target_link_libraries(bmcv_test_1 ${bmnnsdk_link_libs})

add_executable(sail_test_2 sail_test_2.cpp)
target_link_libraries(sail_test_2 sail ${bmnnsdk_link_libs})

add_executable(opencv_imread_perf opencv_imread_perf.cpp)
target_link_libraries(opencv_imread_perf ${bmnnsdk_link_libs})

add_executable(sail_test_3 sail_test_3.cpp)
target_link_libraries(sail_test_3 sail ${bmnnsdk_link_libs})

add_executable(ffmpeg_bmcv_test ffmpeg_bmcv_test.cpp)
target_link_libraries(ffmpeg_bmcv_test sail ${bmnnsdk_link_libs})

add_executable(sail_test_4 sail_test_4.cpp)
target_link_libraries(sail_test_4 sail ${bmnnsdk_link_libs})