cmake_minimum_required(VERSION 3.11)

project(compiler)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")


add_executable(compiler ${SOURCE_FILES})

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(compiler PUBLIC -g -march=native -fsanitize=address -fno-omit-frame-pointer -fpermissive -fno-inline-small-functions)
    target_link_libraries(compiler PUBLIC gvc cgraph cdt -g -fsanitize=address)
else()
    target_compile_options(compiler PUBLIC -g -march=native -O3 -fno-omit-frame-pointer -fpermissive -fno-inline-small-functions)
    target_link_libraries(compiler PUBLIC gvc cgraph cdt)
endif()

