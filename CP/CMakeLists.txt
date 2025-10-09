cmake_minimum_required(VERSION 3.22)
project(cp_thumb LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

option(CP_BUILD_WASM "Build with Emscripten for WebAssembly" ON)

if(CP_BUILD_WASM)
  if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "CP_BUILD_WASM=ON requires Emscripten toolchain")
  endif()
  add_compile_definitions(CP_WASM)
  set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
endif()

add_library(cp_thumb STATIC
  src/thumbnail.cpp
  src/encode.cpp
  src/api.c
)

target_include_directories(cp_thumb PUBLIC include)

if(CP_BUILD_WASM)
  target_compile_options(cp_thumb PRIVATE 
    -sUSE_PTHREADS=0 
    -fno-exceptions
  )
  set_target_properties(cp_thumb PROPERTIES LINK_FLAGS "
    -sALLOW_MEMORY_GROWTH=1 \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sENVIRONMENT=web,worker \
    -sEXPORTED_FUNCTIONS=['_cp_init','_cp_free','_cp_thumbnail_rgba','_cp_thumbnail_png','_cp_thumbnail_jpeg','_cp_malloc','_cp_free_buf'] \
    -sEXPORTED_RUNTIME_METHODS=['cwrap','getValue','setValue','HEAPU8'] \
  ")
endif()

find_package(PkgConfig REQUIRED)
if(NOT EMSCRIPTEN)
  pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
  pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
  pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
  pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
  target_link_libraries(cp_thumb PRIVATE 
    PkgConfig::AVFORMAT PkgConfig::AVCODEC PkgConfig::AVUTIL PkgConfig::SWSCALE)
else()
  target_link_options(cp_thumb PRIVATE 
    -sUSE_FFMPEG=1)
endif()
