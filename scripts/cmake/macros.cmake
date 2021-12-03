# dir_list
# Returns a list of subdirectories for a given directory
macro(dir_list retval curdir)
	file(GLOB sub-dir "${curdir}/*")
	set(list_of_dirs "")
	foreach(dir ${sub-dir})
		if(IS_DIRECTORY ${dir})
			get_filename_component(DIRNAME ${dir} NAME)
			set(list_of_dirs ${list_of_dirs} ${DIRNAME})
		endif()
	endforeach()
	set(${retval} ${list_of_dirs})
endmacro()

# trim_front_words
# Trims X times the front word from a string separated with "/"
# and removes the front "/" characters after that
# (used for filters for visual studio)
macro(trim_front_words source out num_filter_trims)
	set(result "${source}")
	set(counter 0)
	while(${counter} LESS ${num_filter_trims})
		MATH(EXPR counter "${counter} + 1")
		#removes everything at the front up to a "/" character
		string(REGEX REPLACE "^([^/]+)" "" result "${result}")
		#removes all consecutive "/" characters from the front
		string(REGEX REPLACE "^(/+)" "" result "${result}")
	endwhile()
	set(${out} ${result})
endmacro()

# add_dir_2
# Adds all files from a directory to a list of sources and generates
# filters according to their location (accepts relative paths only)
# Also this macro trims X times the front word from the filter string for visual studio filters.
macro(add_dir_2 place dir num_filter_trims)
	if("${dir}" STREQUAL "")
		SET(REALDIR "./")
	else()
		SET(REALDIR ${dir})
	endif()

	FILE(GLOB stuff "${REALDIR}/*.cpp" "${REALDIR}/*.cc" "${REALDIR}/*.c" "${REALDIR}/*.h" "${REALDIR}/*.hpp" "${REALDIR}/*.inl" "${REALDIR}/*.hlsl")
	SET(${place} ${${place}} ${stuff})
	trim_front_words(${REALDIR} REALDIR "${num_filter_trims}")
	# replacing forward slashes with back slashes so filters can be generated (back slash used in parsing...)
	STRING(REPLACE "/" "\\" FILTERS "${REALDIR}")
	SOURCE_GROUP("${FILTERS}" FILES ${stuff})
endmacro()

# add_dir_rec
# Calls add_dir_rec_2 with a default argument for num trims = 0
macro(add_dir_rec place DIR)
	add_dir_rec_2(${place} ${DIR} "0")
endmacro()

# add_dir_rec_2
# Adds all files from a directory traversing it recursively to a list of sources
# and generates filters according to their location (accepts relative paths only).
# Also this macro trims X times the front word from the filter string for visual studio filters.
macro(add_dir_rec_2 place DIR num_filter_trims)
	add_dir_2(${place} "${DIR}" "${num_filter_trims}")
	dir_list(subs "${DIR}")
	FOREACH(subdir ${subs})
		add_dir_rec_2(${place} "${DIR}/${subdir}" "${num_filter_trims}")
	ENDFOREACH()
endmacro()