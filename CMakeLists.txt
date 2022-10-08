cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(BetterHomebrewBrowser)

include(VitaDevelopmentSuite)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(bhbb_dl)
add_subdirectory(libScePafPreload)

link_directories(
  ${VDSUITE_LIBRARY_DIRECTORIES}
)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_compile_options(
        -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -Od -g -noex
    )
else()
    add_compile_options(
        -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -O3 -noex
    )
endif()

add_custom_target(file_toucher 
        COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_link_options(
  --no-standard-libraries -Map=${CMAKE_BINARY_DIR}/mapfile -sn-full-map
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/utils.cpp
    src/bgdl.c
    src/network.cpp
    src/csv.c
    src/split.c
    src/settings.cpp
    src/downloader.cpp
    src/db.cpp
    src/pages/page.cpp
    src/pages/text_page.cpp
    src/pages/apps_page.cpp
    src/pages/multi_page_app_list.cpp
    src/json.cpp
    src/dialog.cpp
    src/xthrow.cpp
    src/paf_runtime.c

    common/bhbb_dl.h
    
    include/main.h
    include/utils.h
    include/network.h
    include/csv.h
    include/settings.h
    include/downloader.h
    include/db.h
    include/pages/page.h
    include/pages/text_page.h
    include/pages/apps_page.h
    include/pages/multi_page_app_list.h
    include/json.hpp
    include/dialog.h
)

target_link_libraries(${PROJECT_NAME}
    SceLibKernel_stub
    SceThreadmgr_stub
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
    SceDeflt
    SceAppSettings_stub
    SceAppSettings2_stub
    SceJson_stub
    SceCommonGuiDialog_stub
    SceModulemgr_stub
    SceVshBridge_stub
    taihenUnsafe_stub
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/common
  ${VDSUITE_USER_INCLUDE_DIRECTORIES}
  ${VDSUITE_STANDARD_INCLUDE_DIRECTORIES}
  $ENV{SCE_PSP2_SDK_DIR}/target/include
  $ENV{SCE_PSP2_SDK_DIR}/target/include_common
)

VDSuiteCreateSfo(${PROJECT_NAME}.sfo param.sfx)
VDSuiteSignElf(${PROJECT_NAME}.self $<TARGET_FILE:${PROJECT_NAME}> BOOT_PARAM bootparam.yml)

add_dependencies(${PROJECT_NAME}
    bhbb_dl.suprx
    libScePafPreload.suprx
    file_toucher    
)

VDSuitePackage(${PROJECT_NAME}.vpk
  FILES
    ${PROJECT_NAME}.self eboot.bin
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.sfo sce_sys/param.sfo
    resource/bhbb_plugin.rco resource/bhbb_plugin.rco
    resource/rco/src/texture/bg.png sce_sys/pic0.png
    ${CMAKE_BINARY_DIR}/libScePafPreload/libScePafPreload.suprx module/libScePafPreload.suprx
    ${CMAKE_BINARY_DIR}/bhbb_dl/bhbb_dl.suprx module/bhbb_dl.suprx
    module/libextractor.suprx module/libextractor.suprx
    sce_sys sce_sys
)
