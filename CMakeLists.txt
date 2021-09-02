cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(BetterHomebrewBrowser)

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
    src/utils.cpp
    src/main.hpp
    src/paf_internal.hpp
    src/paf.hpp
    src/paf.cpp
    src/utils.hpp
    src/audiomgr.cpp
    src/audiomgr.hpp
    src/pagemgr.cpp
    src/pagemgr.hpp
    src/common.hpp
    src/eventhandler.cpp
    src/eventhandler.hpp
    src/network.cpp
    src/network.hpp
    src/parser.cpp
    src/parser.hpp
    src/csv.c
    src/csv.h
    src/fread_csv_line.c
    src/split.c
    src/timemgr.cpp
    src/timemgr.hpp
    src/configmgr.cpp
    src/configmgr.hpp
    src/settingsmgr.cpp
    src/Archives.cpp
    src/Archives.hpp
)

target_link_libraries(${PROJECT_NAME}
    SceKernel_stub
    SceThreadmgr_stub
    SceDbg_stub
    SceAppMgrUser_stub
    SceAppMgr_stub
    SceVshBridge_stub
    ScePower_stub
    SceHttp_stub
    SceNet_stub
    SceNetCtl_stub
    SceSsl_stub
    SceSysmodule_stub
    SceAppUtil_stub
    SceDisplay_stub
    SceShellSvc_stub
    ScePafCommon_stub_weak
    ScePafGraphics_stub_weak
    ScePafLowLayer_stub_weak
    ScePafMisc_stub_weak
    ScePafResource_stub_weak
    SceJson_stub
    ScePafStdc_stub_weak
    ScePafThread_stub_weak
    ScePafTopLevel_stub_weak
    ScePafWidget_stub_weak
    ShellAudio
    ScePsp2Compat_stub_weak
    SceRtcUser_stub
)

VDSuiteCreateSfo(${PROJECT_NAME}.sfo ${PROJECT_NAME}.yml)

VDSuitePackage(${PROJECT_NAME}.vpk
  FILES
    $<TARGET_FILE:${PROJECT_NAME}> eboot.bin
    ${PROJECT_NAME}.sfo sce_sys/param.sfo
    resource/vhbb_plugin.rco resource/vhbb_plugin.rco
    resource/src/texture/bg.png sce_sys/pic0.png
)
