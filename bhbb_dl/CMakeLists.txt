cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(bhbb_dl)
include(VitaDevelopmentSuite)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${VDSUITE_USER_INCLUDE_DIRECTORIES}
  ${VDSUITE_STANDARD_INCLUDE_DIRECTORIES}
)

link_directories(
  ${VDSUITE_LIBRARY_DIRECTORIES}
)

add_compile_options(
    -D_SCE_TARGET_OS_PSP2=1 --no-standard-libraries -Map -sn-full-map
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/main.hpp
    src/Archives.cpp
    src/Archives.hpp
    src/bhbb_dl.h
    src/list.cpp
    src/list.hpp
    src/net.cpp
    src/net.hpp
    src/promote.c
    src/head_bin.h
    src/sha1.c
    src/sha1.h
    src/notifmgr.cpp
    src/notifmgr.hpp
)

target_link_libraries(${PROJECT_NAME}
    SceKernel_stub
    SceThreadmgr_stub
    SceDbg_stub
    SceDisplay_stub
    SceNotificationUtilProgress_stub_weak
    ScePromoterUtil_stub_weak
    SceNotificationUtil_stub_weak
    SceNotificationUtilBgApp_stub_weak
    c_stub
    SceHttp_stub
    SceNet_stub
    SceNetCtl_stub
    SceAppMgr_stub
    ScePsp2Compat_stub_weak
    SceSysmodule_stub
    SceSsl_stub
)

VDSuiteCreateSfo(bhbb_dl.sfo bhbb_dl.sfx)
VDSuiteSignElf(${PROJECT_NAME}.self $<TARGET_FILE:${PROJECT_NAME}> BOOT_PARAM bootparam.yml)