
# Generates a target that aseembles the game plugin specfied by @target_name
# and the game engine libraries in a single dirrectory.

# Assembling ideally could be done in the ${target_name} target, however in Visual Studio
# if a project (exe/dll/lib) is already built, Visual Studio will not run the post build step.
# This results in a very tedious behavior where if a dependancy of the game is changed, for example sge_engine,
# but the game isn't, tne post build step will not get executed, thus the sge_engine will not get copied
# and we will be debugging some old version of sge_engine.
# ... SO ...
# To work around this an additional target is make named ${target_name}_AssembleAndDebug
# By being an "Utility Target" Visual Studio will execute its post build step everytime.
#
# So for the best debugging experience set your start-up project in Visual Studio to be
# ${target_name}_AssembleAndDebug and just hit F5 (or Ctrl+F5) and debug happily!
macro(sge_generate_assemble_and_debug_target_for_game target_name target_output_dir)
if(NOT EMSCRIPTEN)
	add_custom_target(${target_name}_AssembleAndDebug ALL DEPENDS ${target_name})

	add_dependencies(${target_name}_AssembleAndDebug ${target_name})
	
	
	add_custom_command(TARGET ${target_name}_AssembleAndDebug PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove ${target_output_dir}/$<TARGET_FILE:${target_name}>
	)
	
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${target_output_dir}
	)
	
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${target_name}> ${target_output_dir}/
	)

	# sge_editor
if(NOT EMSCRIPTEN)
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sge_editor> ${target_output_dir}/
	)
endif()
	
	# sge_player
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sge_player> ${target_output_dir}/
	)
	
	# sge_core
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sge_core> ${target_output_dir}/
	)
	
	#sge_core shaders
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
					   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/libs/sge_core/core_shaders/ ${target_output_dir}/core_shaders)
	
	# sge_engine
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sge_engine> ${target_output_dir}/
	)
	
	# sge_log
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:sge_log> ${target_output_dir}/
	)
	
	# SDL2
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2> ${target_output_dir}/
	)
	
	# mdlconvlib for import 3D models.
if(NOT EMSCRIPTEN)
	if(TARGET mdlconvlib)
		add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:mdlconvlib> ${target_output_dir}/
		)
	endif()
endif()
	
	# Transfer the engine assets.
	add_custom_command(TARGET ${target_name}_AssembleAndDebug POST_BUILD
					   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/libs/sge_engine/assets/ ${target_output_dir}/assets)
					   
	# Visual Studio Debugging
	set_target_properties(${target_name}_AssembleAndDebug PROPERTIES 
		VS_DEBUGGER_WORKING_DIRECTORY "${target_output_dir}")
		
	set_target_properties(${target_name}_AssembleAndDebug PROPERTIES 
		VS_DEBUGGER_COMMAND "${target_output_dir}/$<TARGET_FILE_NAME:sge_editor>")
endif()
endmacro()