cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(bhbb_dl)
include(VitaDevelopmentSuite)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
enable_language(EMD)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    message(Debug build!)
    add_compile_options(
        -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -Od -g -noex -D_DEBUG=1 -DPAF_BUILD=1
        #-D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -Od -g -noex -DSCE_PAF_TOOL_PRX=1 -D_DEBUG=1
    )
else()
    add_compile_options(
        -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -O3 -noex -DPAF_BUILD=1
    )
endif()

add_link_options(
    --prx-no-runtime-support
)

add_library(${PROJECT_NAME} MODULE
    
    include/head_bin.h
    include/main.h
    include/netutils.h
    include/notice.h
    include/notifmgr.h
    include/offsets.h
    include/promote.h
    include/sha1.h
    include/taskmgr.h
    include/zip.h

    src/main.cpp
    src/offsets.c

    src/zip.cpp
    src/taskmgr.c
    src/notifmgr.c
    src/net.c
    src/promote.c
    src/sha1.c
    src/notice.cpp

    bhbb_dl.emd
)


target_link_directories(${PROJECT_NAME} PRIVATE
  ${VDSUITE_LIBRARY_DIRECTORIES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../common
    include
    ${VDSUITE_USER_INCLUDE_DIRECTORIES}
    ${VDSUITE_STANDARD_INCLUDE_DIRECTORIES}
    $ENV{SCE_PSP2_SDK_DIR}/target/include
    $ENV{SCE_PSP2_SDK_DIR}/target/include_common
)


target_link_libraries(${PROJECT_NAME}
    SceLibKernel_stub
    ScePafStdc_stub
    ScePafResource_stub
    ScePafMisc_stub
    SceThreadMgr_stub
    taihenUnsafe_stub
    SceModulemgr_stub
    taihen_stub
    SceSysmodule_stub
    SceNotificationUtil_stub_weak
    SceNotificationUtilProgress_stub_weak
    SceIofilemgr_stub
    ScePromoterUtil_stub_weak
    SceLsdb_stub
    ScePafThread_stub
    curl_stub_weak
    minizip
)

VDSuiteSignElf(${PROJECT_NAME}.suprx ${PROJECT_NAME})
