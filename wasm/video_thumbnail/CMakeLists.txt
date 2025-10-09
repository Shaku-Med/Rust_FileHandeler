cmake_minimum_required(VERSION 3.20)
project(video_thumbnail_wasm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(video_thumbnail STATIC
  src/thumbnail.cpp
  src/bindings.cpp
)

target_include_directories(video_thumbnail PUBLIC include)

if(EMSCRIPTEN)
  target_compile_options(video_thumbnail PRIVATE -fno-exceptions)
  target_link_options(video_thumbnail PRIVATE
    --bind
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sENVIRONMENT=web,worker,node
    -sALLOW_MEMORY_GROWTH=1
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,FS,HEAPU8
  )
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libswscale libavutil)
pkg_check_modules(TURBOJPEG REQUIRED IMPORTED_TARGET libturbojpeg)

target_link_libraries(video_thumbnail PRIVATE PkgConfig::FFMPEG PkgConfig::TURBOJPEG)
