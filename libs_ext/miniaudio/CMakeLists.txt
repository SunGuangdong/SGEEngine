cmake_minimum_required(VERSION 3.5)

project(miniaudio)

add_library(miniaudio STATIC miniaudio_impl.cpp)
target_include_directories(miniaudio PUBLIC "./")