project(Demo)

cmake_minimum_required(VERSION 3.14)

add_dir_rec_2(DEMO_SOURCES "./src" 2)
if(EMSCRIPTEN)
	add_executable(Demo ${DEMO_SOURCES})
	set_target_properties(Demo PROPERTIES SUFFIX ".html")
else()
	add_library(Demo SHARED ${DEMO_SOURCES})
	set_target_properties(Demo PROPERTIES SUFFIX ".gll")
endif()
sgePromoteWarningsOnTarget(Demo)

target_link_libraries(Demo PUBLIC sge_core)
target_link_libraries(Demo PUBLIC sge_engine)
target_link_libraries(Demo PUBLIC sge_renderer)
target_link_libraries(Demo PUBLIC sge_utils)

if(EMSCRIPTEN)
	target_link_libraries(Demo PUBLIC sge_player)
	set(PRELOADFILE " ${PRELOADFILE} --preload-file ${CMAKE_SOURCE_DIR}/output/Demo/assets/@/home/game/assets/")
	set(PRELOADFILE " ${PRELOADFILE} --preload-file ${CMAKE_SOURCE_DIR}/output/Demo/appdata/@/home/game/appdata/")
	set(PRELOADFILE " ${PRELOADFILE} --preload-file ${CMAKE_SOURCE_DIR}/output/Demo/core_shaders/@/home/game/core_shaders/")
	#set(PRELOADFILE " --preload-file ${CMAKE_SOURCE_DIR}/output/Demo/editorSettings.json@/home/game/editorSettings.json")
	message(${PRELOADFILE})
	set_target_properties(Demo PROPERTIES LINK_FLAGS "${PRELOADFILE}")
endif()

sge_generate_assemble_and_debug_target_for_game(Demo ${CMAKE_SOURCE_DIR}/output/Demo ON)

