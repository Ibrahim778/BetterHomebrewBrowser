cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(BetterHomebrewBrowser)

include(VitaDevelopmentSuite)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(bhbb_dl)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${VDSUITE_USER_INCLUDE_DIRECTORIES}
  ${VDSUITE_STANDARD_INCLUDE_DIRECTORIES}
)

link_directories(
  ${VDSUITE_LIBRARY_DIRECTORIES}
)

add_compile_options(
    -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -O3
)

add_link_options(
  --no-standard-libraries
  -Map -sn-full-map
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
    src/bgdl.c
    src/bgdl.h
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
    SceLibKernel_stub
    SceThreadmgr_stub
	SceIofilemgr_stub
	SceProcessmgr_stub
	ScePower_stub
    SceDbg_stub
    SceAppMgrUser_stub
    SceAppMgr_stub
    SceVshBridge_stub
    SceHttp_stub
    SceNet_stub
    SceNetCtl_stub
    SceSsl_stub
    SceSysmodule_stub
    SceAppUtil_stub
    SceDisplay_stub
    SceShellSvc_stub
	ScePafStdc_stub
    ScePafThread_stub
    ScePafTopLevel_stub
    ScePafWidget_stub
    ScePafCommon_stub
    ScePafGraphics_stub
    ScePafMisc_stub
    ScePafResource_stub
    SceJson_stub
    ShellAudio
    SceBgAppUtil_stub
    taihenUnsafe_stub
    taihen_stub
    ScePsp2Compat_stub_weak
    SceRtcUser_stub
)


VDSuiteCreateSfo(${PROJECT_NAME}.sfo param.sfx)
VDSuiteSignElf(${PROJECT_NAME}.self $<TARGET_FILE:${PROJECT_NAME}> BOOT_PARAM bootparam.yml)

VDSuitePackage(${PROJECT_NAME}.vpk
  FILES
    ${PROJECT_NAME}.self eboot.bin
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.sfo sce_sys/param.sfo
    resource/bhbb_plugin.rco resource/bhbb_plugin.rco
    resource/src/texture/bg.png sce_sys/pic0.png
    ${CMAKE_BINARY_DIR}/bhbb_dl/bhbb_dl.sfo sce_sys/param2.sfo
    ${CMAKE_BINARY_DIR}/bhbb_dl/bhbb_dl.self eboot2.bin
    sce_sys sce_sys
)
