cmake_minimum_required(VERSION 3.19)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

project(bhbb_dl)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions -nostdlib")

add_compile_options(
    -D_SCE_TARGET_OS_PSP2=1
)

add_link_options(

)

add_executable(${PROJECT_NAME}
    src/main.c
    src/offsets.c

    src/minizip/ioapi.c
    src/minizip/unzip.c
    src/minizip/unzip.c
    src/minizip/zip.c
    src/zip.cpp
    src/zip.h
    src/libc2paf.c
    src/taskmgr.c
    src/taskmgr.h
    src/notifmgr.c
    src/notifmgr.h
    src/net.c
    src/net.h
    src/promote.c
    src/promote.h
    src/sha1.c
    src/sha1.h
    src/head_bin.h

    bhbb_dl.yml
)

target_link_libraries(${PROJECT_NAME}
    SceLibKernel_stub
    ScePaf_stub
    z
    SceKernelThreadMgr_stub
    SceIofilemgr_stub
    taihen_stub
    SceSysmodule_stub
    SceNotificationUtilVDSuite_stub_weak
    ScePafVDSuite_stub
    ScePromoterUtil_stub_weak
    curl_stub_weak
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../common
    include
)

target_link_directories(${PROJECT_NAME} PRIVATE
    $ENV{VITASDK}/arm-vita-eabi/lib/vdsuite
)

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME} CONFIG ${PROJECT_NAME}.yml UNSAFE)