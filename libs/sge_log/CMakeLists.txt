#
# sge_log
#

cmake_minimum_required(VERSION 3.5)

project(sge_log)

add_dir_rec_2(SOURCES_SGE_LOG "./src" 3)

add_library(sge_log SHARED ${SOURCES_SGE_LOG})
target_compile_definitions(sge_log PRIVATE SGE_LOG_BUILDING_DLL=1)
target_include_directories(sge_log PUBLIC "./src")
target_link_libraries(sge_log PUBLIC sge_utils)


sgePromoteWarningsOnTarget(${PROJECT_NAME})
