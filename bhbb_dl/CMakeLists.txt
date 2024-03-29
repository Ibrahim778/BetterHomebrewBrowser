cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(bhbb_dl)
include(VitaDevelopmentSuite)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
enable_language(EMD)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_compile_options(
        -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -Od -g -noex -D_DEBUG=1 -DPAF_BUILD=1 -D__psp2__ -D__SNC__
        #-D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -Od -g -noex -DSCE_PAF_TOOL_PRX=1 -D_DEBUG=1 #use this if non devkit libpaf
    )
else()
    add_compile_options(
        -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -O3 -noex -DPAF_BUILD=1
    )
endif()

add_link_options(
    --prx-no-runtime-support
)

add_custom_target(${PROJECT_NAME}.elf.target
  DEPENDS ${PROJECT_NAME}.elf ${PROJECT_NAME}.suprx
)

VDSuiteCreateStubs(stubs SceLsdb.emd
  LIBRARIES
    SceLsdb_stub_weak
)

add_library(${PROJECT_NAME} MODULE
    
    include/head_bin.h
    include/main.h
    include/offsets.h
    include/promote.h
    include/sha1.h
    include/compressed_file.h
    include/zip.h
    include/installer.h
    include/tgz.h

    src/main.cpp
    src/offsets.c
    src/zip.cpp
    src/promote.c
    src/sha1.c
    src/installer.cpp
    src/compressed_file.cpp
    src/tgz.cpp

    # minizip
    src/minizip/unzip.c
    src/minizip/ioapi.c
    src/minizip/zip.c

    src/libc2paf.c

    ../common/bhbb_dl.h
    ../common/dialog.cpp
    ../common/dialog.h

    bhbb_dl.emd
)

add_dependencies(${PROJECT_NAME}
    stubs.target
)


target_link_directories(${PROJECT_NAME} PRIVATE
  ${VDSUITE_LIBRARY_DIRECTORIES}
  ${CMAKE_BINARY_DIR}/stubs
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
    ScePafWidget_stub
    ScePafThread_stub
    ScePafGraphics_stub
    ScePafTopLevel_stub
    ScePafMisc_stub
    ScePafCommon_stub
    SceAppMgrUser_stub
    SceCommonGuiDialog_stub
    SceAppMgr_stub
    SceThreadMgr_stub
    taihenUnsafe_stub
    SceModulemgr_stub
    taihen_stub
    SceSysmodule_stub
    SceIofilemgr_stub
    ScePromoterUtil_stub_weak
    SceLsdb_stub
    SceShellUtil_stub
    SceLsdb_stub_weak
    z
)
VDSuiteSignElf(${PROJECT_NAME}.suprx ${PROJECT_NAME})