cmake_minimum_required(VERSION 3.5)
project(sge_audio)

add_dir_rec_2(SOURCES_SGE_AUDIO "./src/sge_audio/" 3)
add_library(sge_audio STATIC ${SOURCES_SGE_AUDIO})

target_include_directories(sge_audio PUBLIC "./src")
target_link_libraries(sge_audio PUBLIC sge_utils miniaudio)

sgePromoteWarningsOnTarget(sge_audio)
