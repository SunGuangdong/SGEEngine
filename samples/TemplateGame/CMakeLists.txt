cmake_minimum_required(VERSION 3.14)

project(TemplateGame)

add_dir_rec_2(TEMPLATE_SOURCES "./src" 2)
add_library(${PROJECT_NAME} SHARED ${TEMPLATE_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".gll")
sgePromoteWarningsOnTarget(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC sge_core)
target_link_libraries(${PROJECT_NAME} PUBLIC sge_engine)
target_link_libraries(${PROJECT_NAME} PUBLIC sge_renderer)
target_link_libraries(${PROJECT_NAME} PUBLIC sge_utils)

sge_generate_assemble_and_debug_target_for_game(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/output/${PROJECT_NAME} ON)

