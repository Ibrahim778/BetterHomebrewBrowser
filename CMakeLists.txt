cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE "$ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cmake/psp2-snc-toolchain.cmake")
project(BetterHomebrewBrowser)

include(VitaDevelopmentSuite)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(bhbb_dl)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_compile_options(
        -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -O3 -g -noex -D_DEBUG=1 -DSCE_PAF_TOOL_PRX=1
    )
else()
    add_compile_options(
        -D_SCE_TARGET_OS_PSP2=1 -Xdiag=0 -Xquit=2 -O3 -noex
    )
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/utils.cpp
    src/tex_pool.cpp
    src/event.cpp
    src/paf_runtime.cpp
    src/settings.cpp
    src/downloader.cpp
    src/fread_line.c

    src/pages/page.cpp
    src/pages/app_viewer.cpp
    src/pages/text_page.cpp
    src/pages/app_browser.cpp
    src/pages/image_viewer.cpp

    src/db/source.cpp
    src/db/vitadb.cpp
    src/db/vhbd.cpp
    src/db/cbpsdb.cpp
    src/db/psphbb.cpp

    common/bhbb_dl.h
    common/dialog.h
    common/dialog.cpp
    
    include/utils.h
    include/bhbb_plugin.h
    include/bhbb_locale.h
    include/bhbb_settings.h
    include/json.h
    include/tex_pool.h
    include/event.h
    include/settings.h
    include/downloader.h
    include/csv.h

    include/pages/page.h
    include/pages/app_browser.h
    include/pages/app_viewer.h
    include/pages/text_page.h
    include/pages/image_viewer.h

    include/db/source.h
    include/db/vitadb.h
    include/db/vhbd.h
    include/db/cbpsdb.h
    include/db/psphbb.h
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${VDSUITE_LIBRARY_DIRECTORIES}
)
  
target_link_options(${PROJECT_NAME} PRIVATE    
    --no-standard-libraries -Map=${CMAKE_BINARY_DIR}/mapfile -sn-full-map
)

add_dependencies(${PROJECT_NAME}
    bhbb_dl
)

target_link_libraries(${PROJECT_NAME}
    SceLibKernel_stub
    SceThreadmgr_stub
	SceDbg_stub
    SceAppMgrUser_stub
    SceSysmodule_stub
    SceAppUtil_stub
    SceShellSvc_stub

    # Net stubs
    SceHttp_stub
    SceNet_stub
    SceNetCtl_stub
    SceSsl_stub
    
	# PAF UI libs
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
    SceAppSettings_stub
    SceCommonGuiDialog_stub
    
    SceVshBridge_stub
    taihenUnsafe_stub
    ScePerf_stub
    SceFiber_stub
    PafFileExt
    curl_stub
    snc
)

# Create our list for all locale .xml files
file(GLOB localeXml "${CMAKE_SOURCE_DIR}/resource/locale/*.xml")
list(LENGTH localeXml localeCount)
math(EXPR lastLocaleIdx "${localeCount} - 1" OUTPUT_FORMAT DECIMAL)

# Create our list for generated rcs files
set(rcsFiles)

# Loop through the xml files and generate an rcs file and add to the deps list
foreach(i RANGE 0 ${lastLocaleIdx})
    list(GET localeXml ${i} _xml)
    
    get_filename_component(localeName ${_xml} NAME_WE)
    set(rcsOutPath ${CMAKE_SOURCE_DIR}/resource/locale/${localeName}.rcs)
    message("Locale added - ${localeName}.rcs")
    
    set(rcsOutputs ${rcsOutPath})

    if(${i} EQUAL 1) # Add the bhbb_locale.h output, (only needed for 1 file)
        list(APPEND rcsOutputs ${CMAKE_SOURCE_DIR}/include/bhbb_locale.h)
    endif()
    
    add_custom_command(
        OUTPUT ${rcsOutPath} 
        COMMAND $ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cxml/acdc.exe -c -i ${_xml} -s $ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cxml/rcs.cxmldef -o ${rcsOutPath} -x ${CMAKE_SOURCE_DIR}/include/bhbb_locale.h
        # COMMAND python $ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cxml/appinfo/rcs_compiler.py -o ${rcsOutPath} ${_xml} 
        DEPENDS ${_xml}
    )

    list(APPEND rcsFiles ${rcsOutPath})
endforeach()

# Generate RCO file
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bhbb_plugin.rco ${CMAKE_SOURCE_DIR}/include/bhbb_plugin.h
    DEPENDS ${CMAKE_SOURCE_DIR}/resource/bhbb_plugin.xml ${CMAKE_SOURCE_DIR}/resource/file/bhbb_settings.xml ${rcsFiles}
    COMMAND $ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cxml/acdc.exe -c -i ${CMAKE_SOURCE_DIR}/resource/bhbb_plugin.xml -s $ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cxml/rco.cxmldef -r ${CMAKE_BINARY_DIR}/bhbb_plugin.rcd -o ${CMAKE_BINARY_DIR}/bhbb_plugin.rco -x ${CMAKE_SOURCE_DIR}/include/bhbb_plugin.h
    #COMMAND python $ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cxml/appinfo/appinfo_compiler.py -o ${CMAKE_BINARY_DIR}/bhbb_plugin.rco ${CMAKE_SOURCE_DIR}/resource/bhbb_plugin.xml # -x ${CMAKE_SOURCE_DIR}/include/bhbb_plugin.h
)

# Generate the headers for the plugin settings
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/bhbb_settings.h
    COMMAND py $ENV{SCE_PSP2_SDK_DIR}/host_tools/build/cxml/genhashheaders.py ${CMAKE_SOURCE_DIR}/resource/file/bhbb_settings.xml ${CMAKE_SOURCE_DIR}/include/bhbb_settings.h
    DEPENDS ${CMAKE_SOURCE_DIR}/resource/file/bhbb_settings.xml
)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    # Generate the RCD file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/bhbb_plugin.rcd
        DEPENDS ${CMAKE_SOURCE_DIR}/resource/bhbb_plugin.xml 
        COMMAND $ENV{SCE_PSP2_SDK_DIR}/host_tools/build/bin/psp2rcd.exe -i ${CMAKE_SOURCE_DIR}/resource/bhbb_plugin.xml -o ${CMAKE_BINARY_DIR}/bhbb_plugin.rcd
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/common
  ${VDSUITE_USER_INCLUDE_DIRECTORIES}
  ${VDSUITE_STANDARD_INCLUDE_DIRECTORIES}
  $ENV{SCE_PSP2_SDK_DIR}/target/include
  $ENV{SCE_PSP2_SDK_DIR}/target/include_common
)

VDSuiteCreateSfo(${PROJECT_NAME}.sfo param.sfx)
VDSuiteSignElf(${PROJECT_NAME}.self $<TARGET_FILE:${PROJECT_NAME}> BOOT_PARAM bootparam.yml PROGRAM_AUTHORITY_ID 2200000000000001)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    VDSuitePackage(${PROJECT_NAME}.vpk
    FILES
      ${PROJECT_NAME}.self eboot.bin
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.sfo sce_sys/param.sfo
      ${CMAKE_BINARY_DIR}/bhbb_plugin.rco resource/bhbb_plugin.rco
      resource/texture/bg.png sce_sys/pic0.png
      ${CMAKE_BINARY_DIR}/bhbb_dl/bhbb_dl.suprx module/bhbb_dl.suprx
      sce_sys sce_sys
      module module
      ${CMAKE_BINARY_DIR}/bhbb_plugin.rcd resource/bhbb_plugin.rcd
  )  
else()
    VDSuitePackage(${PROJECT_NAME}.vpk
    FILES
        ${PROJECT_NAME}.self eboot.bin
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.sfo sce_sys/param.sfo
        ${CMAKE_BINARY_DIR}/bhbb_plugin.rco resource/bhbb_plugin.rco
        resource/texture/bg.png sce_sys/pic0.png
        ${CMAKE_BINARY_DIR}/bhbb_dl/bhbb_dl.suprx module/bhbb_dl.suprx
        sce_sys sce_sys
        module module
    )
endif()

add_custom_target(${PROJECT_NAME}.elf.target
  DEPENDS ${PROJECT_NAME}.elf
)

add_custom_command(
  OUTPUT ${PROJECT_NAME}.elf
  DEPENDS ${PROJECT_NAME}.self
  COMMAND "$ENV{SCE_ROOT_DIR}/PSP2/Tools/Publishing Tools/bin/ext/unfself.exe" ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.self ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
  COMMENT "Making ELF file"
)
