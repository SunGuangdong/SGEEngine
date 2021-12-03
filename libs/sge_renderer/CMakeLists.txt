# Project sge_renderer
cmake_minimum_required(VERSION 3.5)

project(sge_renderer)

add_dir_rec_2(SOURCES_SGE_REND "./src/sge_renderer/renderer" 3)

add_dir_rec_2(SOURCES_SGE_D3D11 "./src/sge_renderer/d3d11" 3)
if(NOT SGE_REND_API STREQUAL "Direct3D11")
	# If the rendering API is not D3D11, add them to the project but do not compile them.
	# We want them to be visible in the IDE but not compilable.
	set_source_files_properties(${SOURCES_SGE_D3D11} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

add_dir_rec_2(SOURCES_SGE_GL "./src/sge_renderer/gl" 3)
if(NOT SGE_REND_API STREQUAL "OpenGL")
	# If the rendering API is not OpenGL, add them to the project but do not compile them.
	# We want them to be visible in the IDE but not compilable.
	set_source_files_properties(${SOURCES_SGE_GL} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

add_library(sge_renderer STATIC 
	${SOURCES_SGE_REND} 
	${SOURCES_SGE_D3D11}
	${SOURCES_SGE_GL}
)

if(SGE_REND_API STREQUAL "OpenGL" AND NOT EMSCRIPTEN)
	target_include_directories(sge_renderer PRIVATE "../../libs_ext/glew")
endif()

target_include_directories(sge_renderer PUBLIC "./src")
target_include_directories(sge_renderer PUBLIC "../../libs_ext/stb")

target_link_libraries(sge_renderer PUBLIC sge_utils)
target_link_libraries(sge_renderer PUBLIC sge_log)
target_link_libraries(sge_renderer PUBLIC HLSLParser)
target_link_libraries(sge_renderer PUBLIC sge_codepreproc)
if(SGE_REND_API STREQUAL "OpenGL" AND NOT EMSCRIPTEN)
	# on Emscripten for OpenGL do not use our own glew, it has its own.
	target_link_libraries(sge_renderer PUBLIC glew)

	if(NOT WIN32)
		target_link_libraries(sge_renderer PUBLIC GL)
		target_link_libraries(sge_renderer PUBLIC GLU)
	else()
		target_link_libraries(sge_renderer PUBLIC OpenGL32)
	endif()

endif()

if(SGE_REND_API STREQUAL "Direct3D11")
	target_link_libraries(sge_renderer PUBLIC d3d11.lib)
	target_link_libraries(sge_renderer PUBLIC d3d10.lib)
	target_link_libraries(sge_renderer PUBLIC d3dcompiler.lib)
	target_link_libraries(sge_renderer PUBLIC dxguid.lib)
endif()

sgePromoteWarningsOnTarget(sge_renderer)
