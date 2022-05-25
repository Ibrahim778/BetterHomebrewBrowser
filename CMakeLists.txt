cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(BetterHomebrewBrowser)

include(VitaDevelopmentSuite)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(bhbb_dl)
add_subdirectory(libScePafPreload)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${VDSUITE_USER_INCLUDE_DIRECTORIES}
  ${VDSUITE_STANDARD_INCLUDE_DIRECTORIES}
  $ENV{SCE_PSP2_SDK_DIR}/target/include
  $ENV{SCE_PSP2_SDK_DIR}/target/include_common
)

link_directories(
  ${VDSUITE_LIBRARY_DIRECTORIES}
)

add_compile_options(
    -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -Od
)

add_link_options(
  --no-standard-libraries
  -Map -sn-full-map
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/utils.cpp
    src/paf.cpp
    src/bgdl.c
    src/network.cpp
    src/parser.cpp
    src/csv.c
    src/split.c
    src/settingsmgr.cpp
    src/downloader.cpp
    src/db.cpp
    src/pages/page.cpp
    src/pages/text_page.cpp
    src/pages/home_page.cpp
)

target_link_libraries(${PROJECT_NAME}
    SceLibKernel_stub
    SceThreadmgr_stub
	SceProcessmgr_stub
	SceDbg_stub
    SceAppMgrUser_stub
    SceHttp_stub
    SceNet_stub
    SceNetCtl_stub
    SceSsl_stub
    SceSysmodule_stub
    SceAppUtil_stub
    SceShellSvc_stub
	ScePafStdc_stub
    ScePafThread_stub
    ScePafTopLevel_stub
    ScePafWidget_stub
    ScePafCommon_stub
    ScePafGraphics_stub
    ScePafResource_stub
    ScePafMisc_stub
    ShellAudio
    SceRtcUser_stub
    SceIpmi_stub
    SceShellUtil_stub
    SceCommonDialog_stub
    SceDeflt
)


VDSuiteCreateSfo(${PROJECT_NAME}.sfo param.sfx)
VDSuiteSignElf(${PROJECT_NAME}.self $<TARGET_FILE:${PROJECT_NAME}> BOOT_PARAM bootparam.yml)

VDSuitePackage(${PROJECT_NAME}.vpk
  FILES
    ${PROJECT_NAME}.self eboot.bin
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.sfo sce_sys/param.sfo
    resource/bhbb_plugin.rco resource/bhbb_plugin.rco
    resource/rco/src/texture/bg.png sce_sys/pic0.png
    resource/settings.xml resource/settings.xml
    ${CMAKE_BINARY_DIR}/libScePafPreload/libScePafPreload.suprx module/libScePafPreload.suprx
    ${CMAKE_BINARY_DIR}/bhbb_dl/bhbb_dl.sfo sce_sys/param2.sfo
    ${CMAKE_BINARY_DIR}/bhbb_dl/bhbb_dl.self eboot2.bin
    sce_sys sce_sys
)
