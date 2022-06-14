cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(bhbb_dl)

include(VitaDevelopmentSuite)
enable_language(EMD)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

link_directories(
  ${VDSUITE_LIBRARY_DIRECTORIES}
)

add_compile_options(
    -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -O3 -D__SNC__
)

add_link_options(
  --prx-no-runtime-support
  -Map -sn-full-map
)

add_library(${PROJECT_NAME} MODULE
    src/main.cpp
    src/offsets.c
    ${PROJECT_NAME}.emd
)

target_link_libraries(${PROJECT_NAME}
    SceKernel_stub
    ScePafStdc_stub
    ScePafThread_stub
    ScePafTopLevel_stub
    ScePafWidget_stub
    ScePafCommon_stub
    ScePafGraphics_stub
    ScePafResource_stub
    ScePafMisc_stub
    SceSysmodule_stub
    SceCes
    taihen_stub
    QuickMenuReborn_stub_weak
    SceAppMgrUser_stub
    taihenUnsafe_stub
    SceAppMgr_Stub
    SceNotificationUtil_stub_weak
    SceShellUtil_stub
    extractor_stub_weak
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/bhbb_dl/include
    ${CMAKE_SOURCE_DIR}/common
    ${VDSUITE_USER_INCLUDE_DIRECTORIES}
    ${VDSUITE_STANDARD_INCLUDE_DIRECTORIES}
    $ENV{SCE_PSP2_SDK_DIR}/target/include
    $ENV{SCE_PSP2_SDK_DIR}/target/include_common
)

VDSuiteSignElf(${PROJECT_NAME}.suprx $<TARGET_FILE:${PROJECT_NAME}>)