cmake_minimum_required(VERSION 3.14)

project(CharacterGame)

add_dir_rec_2(TEMPLATE_SOURCES "./src" 2)
if(EMSCRIPTEN)
	add_executable(${PROJECT_NAME} ${TEMPLATE_SOURCES})
	set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
else()
	add_library(${PROJECT_NAME} SHARED ${TEMPLATE_SOURCES})
	set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".gll")
endif()
sgePromoteWarningsOnTarget(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC sge_core)
target_link_libraries(${PROJECT_NAME} PUBLIC sge_engine)
target_link_libraries(${PROJECT_NAME} PUBLIC sge_renderer)
target_link_libraries(${PROJECT_NAME} PUBLIC sge_utils)

if(EMSCRIPTEN)
	target_link_libraries(${PROJECT_NAME} PUBLIC sge_player)
	set(PRELOADFILE " ${PRELOADFILE} --preload-file ${CMAKE_SOURCE_DIR}/output/${PROJECT_NAME}/assets/@/home/game/assets/")
	set(PRELOADFILE " ${PRELOADFILE} --preload-file ${CMAKE_SOURCE_DIR}/output/${PROJECT_NAME}/appdata/@/home/game/appdata/")
	set(PRELOADFILE " ${PRELOADFILE} --preload-file ${CMAKE_SOURCE_DIR}/output/${PROJECT_NAME}/core_shaders/@/home/game/core_shaders/")
	#set(PRELOADFILE " --preload-file ${CMAKE_SOURCE_DIR}/output/${PROJECT_NAME}/editorSettings.json@/home/game/editorSettings.json")
	message(${PRELOADFILE})
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${PRELOADFILE}")
endif()

sge_generate_assemble_and_debug_target_for_game(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/output/${PROJECT_NAME})

