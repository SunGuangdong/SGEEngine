#####################################################
# Project SGE
#####################################################

cmake_minimum_required(VERSION 3.5)

project(sge_audio)

add_dir_rec_2(SOURCES_SGE_ENGINE "./src/sge_audio/" 3)

add_library(sge_audio STATIC
	${SOURCES_SGE_ENGINE}
)

target_include_directories(sge_audio PUBLIC "./src")
target_include_directories(sge_audio PUBLIC "../../libs_ext/stb")
target_include_directories(sge_audio PUBLIC "../../libs_ext/miniaudio")

target_link_libraries(sge_audio PUBLIC sge_utils)

if(NOT EMSCRIPTEN)
	#Do not link with our SDL2, Emscripten has it's own.
	target_link_libraries(sge_audio PUBLIC SDL2main SDL2)
endif()

sgePromoteWarningsOnTarget(${PROJECT_NAME})
