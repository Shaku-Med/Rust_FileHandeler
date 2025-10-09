cmake_minimum_required(VERSION 3.20)
project(video_thumbnail_wasm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_FUNCTIONS='[\"_malloc\", \"_free\"]'")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]'")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s MODULARIZE=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORT_NAME='VideoThumbnailModule'")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)

include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS})

if(EMSCRIPTEN)
    set(FFMPEG_LIBRARIES
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libavformat.a
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libavcodec.a
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libavutil.a
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libswscale.a
    )
endif()

add_library(video_thumbnail_wasm SHARED
    src/video_thumbnail_generator.cpp
    src/ffmpeg_wrapper.cpp
    src/image_processor.cpp
    src/memory_manager.cpp
    src/emscripten_bindings.cpp
)

target_link_libraries(video_thumbnail_wasm ${FFMPEG_LIBRARIES})

if(EMSCRIPTEN)
    target_link_libraries(video_thumbnail_wasm pthread)
    set_target_properties(video_thumbnail_wasm PROPERTIES
        OUTPUT_NAME "video_thumbnail_wasm"
        SUFFIX ".js"
    )
endif()