# sge_codepreproc provides a way to preprocess file (or text) with a C-style preprocessor.
cmake_minimum_required(VERSION 3.5)

project(sge_codepreproc)

add_dir_rec_2(CODE_SOURCES "./src" 3)
add_library(sge_codepreproc STATIC ${CODE_SOURCES})
target_link_libraries(sge_codepreproc PUBLIC mcpp)
target_link_libraries(sge_codepreproc PUBLIC sge_utils)
target_include_directories(sge_codepreproc PUBLIC "./src")

sgePromoteWarningsOnTarget(sge_codepreproc)
