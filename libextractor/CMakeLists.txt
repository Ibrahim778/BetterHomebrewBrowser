cmake_minimum_required(VERSION 3.19)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

add_subdirectory(usage)
project(libextractor)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

add_executable(${PROJECT_NAME}
    src/main.c
    src/minizip/ioapi.c
    src/minizip/unzip.c
    src/minizip/unzip.c
    src/minizip/zip.c
    src/zip.cpp
    src/zip.h
    src/libc2paf.c
)

target_link_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(${PROJECT_NAME}
    SceLibKernel_stub
    ScePaf_stub
    z
    SceKernelThreadMgr_stub
    SceIofilemgr_stub

  )

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME} CONFIG ${PROJECT_NAME}.yml UNSAFE)
vita_create_stubs(stubs ${PROJECT_NAME} ${PROJECT_NAME}.yml)