cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")

project(libScePafPreload.suprx)

include(VitaDevelopmentSuite)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)


include_directories(
    ${CMAKE_SOURCE_DIR}
    ${VDSUITE_USER_INCLUDE_DIRECTORIES}
    ${VDSUITE_STANDARD_INCLUDE_DIRECTORIES}
    $ENV{SCE_PSP2_SDK_DIR}/target/include
    $ENV{SCE_PSP2_SDK_DIR}/target/include_common
)

add_link_options(
    --no-required-files
)

link_directories(
    ${VDSUITE_LIBRARY_DIRECTORIES}
)

add_compile_options(
    -D_SCE_TARGET_OS_PSP2
)

add_library(${PROJECT_NAME} SHARED
    src/main.c
)

target_link_libraries(${PROJECT_NAME}
    SceLibKernel_stub
    SceAppMgr_stub
    SceSysmodule_stub
)

VDSuiteSignElf(${PROJECT_NAME} $<TARGET_FILE:${PROJECT_NAME}>)