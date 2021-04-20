cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 14)
set(TOP ${CMAKE_SOURCE_DIR})

if (NOT DEFINED TARGET_ARCH)
    set(TARGET_ARCH x86-pcie)
endif()

#set(TARGET_ARCH "x86-pcie")
#set(TARGET_ARCH "arm-pcie")
#set(TARGET_ARCH "soc")

# User need to set this before compiling.
if (DEFINED ENV{REL_TOP})
    set(BMNNSDK2_TOP $ENV{REL_TOP})
else()
    set(BMNNSDK2_TOP $ENV{HOME}/bmnnsdk2/bmnnsdk2-bm1684_v2.3.2)
endif()

set(bmnnsdk_inc_dirs ${BMNNSDK2_TOP}/include
        ${BMNNSDK2_TOP}/include/bmruntime
        ${BMNNSDK2_TOP}/include/bmlib
        ${BMNNSDK2_TOP}/include/third_party/boost/include
        ${BMNNSDK2_TOP}/include/opencv/opencv4
        ${BMNNSDK2_TOP}/include/ffmpeg
        CACHE INTERNAL "")

set(bmnnsdk_link_libs bmrt bmlib bmcv
        opencv_core opencv_imgproc opencv_imgcodecs opencv_videoio opencv_dnn
        avfilter avformat avcodec avutil swresample swscale bmvideo bmjpuapi bmjpulite
        boost_system boost_filesystem protobuf gflags glog pthread
        CACHE INTERNAL "")

if (${TARGET_ARCH} STREQUAL "x86-pcie")
    set(bmnnsdk_link_dirs ${BMNNSDK2_TOP}/lib/bmnn/pcie
            ${BMNNSDK2_TOP}/lib/opencv/x86
            ${BMNNSDK2_TOP}/lib/thirdparty/x86
            ${BMNNSDK2_TOP}/lib/opencv/x86
            ${BMNNSDK2_TOP}/lib/ffmpeg/x86
            ${BMNNSDK2_TOP}/lib/decode/x86
            CACHE INTERNAL "")


elseif (${TARGET_ARCH} STREQUAL "arm-pcie")
    set(bmnnsdk_link_dirs ${BMNNSDK2_TOP}/lib/bmnn/arm_pcie
            ${BMNNSDK2_TOP}/lib/opencv/arm_pcie
            ${BMNNSDK2_TOP}/lib/thirdparty/soc
            ${BMNNSDK2_TOP}/lib/opencv/arm_pcie
            ${BMNNSDK2_TOP}/lib/ffmpeg/arm_pcie
            ${BMNNSDK2_TOP}/lib/decode/arm_pcie
            CACHE INTERNAL "")
else()
    set(bmnnsdk_link_dirs ${BMNNSDK2_TOP}/lib/bmnn/soc
            ${BMNNSDK2_TOP}/lib/opencv/soc
            ${BMNNSDK2_TOP}/lib/thirdparty/soc
            ${BMNNSDK2_TOP}/lib/opencv/soc
            ${BMNNSDK2_TOP}/lib/ffmpeg/soc
            ${BMNNSDK2_TOP}/lib/decode/soc
            CACHE INTERNAL "")
endif ()

include_directories(${bmnnsdk_inc_dirs} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Concurrent_INCLUDE_DIRS}
        ${Qt5Test_INCLUDE_DIRS})
link_directories(${bmnnsdk_link_dirs})

set(bm_link_libs bmrt bmlib)

add_definitions(-DBM_VPP_ENABLE=1)

#find_package(OpenCV)

include_directories(${bmnnsdk_inc_dirs})
link_directories(${bmnnsdk_link_dirs})

add_executable(bmodel_dump bmodel_dump/main.cpp bmodel_dump/bmodel_dump.cpp)
target_link_libraries(bmodel_dump ${bmnnsdk_link_libs})

add_executable(data_compare data_compare/data_compare.cpp)
target_link_libraries(data_compare ${bmnnsdk_link_libs})