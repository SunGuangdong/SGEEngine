cmake_minimum_required(VERSION 3.0)

#############################################################################
# FBXSDK_TARGET - used to import the external prebuild FBX SDK libraries
# and correctly link them to our tergets.
# The tricky one is IMPORTED_LOCATION_NOOPT as it is just release withuot
# any optimizations
#############################################################################
if(SGE_FBX_SDK_DIR STREQUAL "")
	message(WARNING "FBX SDK directory is not specified. Importing files will be done with Assimp")
else()
	# Create an imported target for FBX SDK binaries.
	add_library(FBXSDK_TARGET STATIC IMPORTED)
	add_library(FBXSDK_XML_TARGET STATIC IMPORTED)
	add_library(FBXSDK_ZLIB_TARGET STATIC IMPORTED)
	if(WIN32)

		set(SGE_FBX_SDK_VS_LIB_DIR "")
		IF(EXISTS ${SGE_FBX_SDK_DIR}/lib/vs2017)
			set(SGE_FBX_SDK_VS_LIB_DIR vs2017)
		elseif (EXISTS ${SGE_FBX_SDK_DIR}/lib/vs2019)
			set(SGE_FBX_SDK_VS_LIB_DIR vs2019)
		endif()
		
		if(SGE_FBX_SDK_VS_LIB_DIR STREQUAL "")
			message(FATAL_ERROR "We failed to locate the FBX SDK link lib directory. Please report an issue and/or modify this file to fix it")
		endif()
		
		# Windows platform.
		set_target_properties(FBXSDK_TARGET PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${SGE_FBX_SDK_DIR}/include"
			
			IMPORTED_LOCATION_RELEASE        "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/libfbxsdk-md.lib"
			IMPORTED_LOCATION_RELWITHDEBINFO "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/libfbxsdk-md.lib"
			IMPORTED_LOCATION_NOOPT          "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/libfbxsdk-md.lib"
			IMPORTED_LOCATION_DEBUG          "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/debug/libfbxsdk-md.lib")
			
		set_target_properties(FBXSDK_XML_TARGET PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${SGE_FBX_SDK_DIR}/include"
			
			IMPORTED_LOCATION_RELEASE        "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/libxml2-md.lib"
			IMPORTED_LOCATION_RELWITHDEBINFO "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/libxml2-md.lib"
			IMPORTED_LOCATION_NOOPT          "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/libxml2-md.lib"
			IMPORTED_LOCATION_DEBUG          "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/debug/libxml2-md.lib")
			
		set_target_properties(FBXSDK_ZLIB_TARGET PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${SGE_FBX_SDK_DIR}/include"
			
			IMPORTED_LOCATION_RELEASE        "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/zlib-md.lib"
			IMPORTED_LOCATION_RELWITHDEBINFO "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/zlib-md.lib"
			IMPORTED_LOCATION_NOOPT          "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/release/zlib-md.lib"
			IMPORTED_LOCATION_DEBUG          "${SGE_FBX_SDK_DIR}/lib/${SGE_FBX_SDK_VS_LIB_DIR}/x64/debug/zlib-md.lib")
	else()
		# Linux platform.
		set_target_properties(FBXSDK_TARGET PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SGE_FBX_SDK_DIR}/include")
		
		# IMPORTED_LOCATION_<CONFIG> only works on Windows with Visual Studio. For everything else we have to use IMPORTED_LOCATION
		string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWERCASE)
		if(CMAKE_BUILD_TYPE_LOWERCASE STREQUAL "debug")
			set_target_properties(FBXSDK_TARGET PROPERTIES IMPORTED_LOCATION "${SGE_FBX_SDK_DIR}/lib/gcc/x64/release/libfbxsdk.a")
		else()
			set_target_properties(FBXSDK_TARGET PROPERTIES IMPORTED_LOCATION "${SGE_FBX_SDK_DIR}/lib/gcc/x64/debug/libfbxsdk.a")
		endif()
	endif()
endif()

#############################################################################
# MDLCONVLIB_SRC
#############################################################################
add_dir_rec_2(MDLCONVLIB_SRC "./src" 3)
add_library(mdlconvlib SHARED ${MDLCONVLIB_SRC})

# If FBX SDK is available mark it as such and link with it.
if(NOT SGE_FBX_SDK_DIR STREQUAL "")
	# SGE_FBX_SDK_AVAILABLE specifies to the code that FBX SDK is avilable.
	target_compile_definitions(mdlconvlib PRIVATE SGE_FBX_SDK_AVAILABLE=1)
	
	if(WIN32)
		target_link_libraries(mdlconvlib FBXSDK_TARGET FBXSDK_XML_TARGET FBXSDK_ZLIB_TARGET)
	else()
		target_link_libraries(mdlconvlib FBXSDK_TARGET)
			
		# On Linux FBX SDK still depends on zlib and libxml2 however they do not come pre-built like on Windows.
		# Use the ones alredy installed on the system.

		# To install ZLIB you'd probably need something like: sudo apt install zlib1g-dev
		# To install LibXml2 you'd probably need something like: sudo apt-get install libxml2-dev
		find_package(ZLIB REQUIRED) 
		target_link_libraries(mdlconvlib ZLIB::ZLIB)
		
		find_package(LibXml2 REQUIRED)
		target_link_libraries(mdlconvlib LibXml2::LibXml2)
	endif()
endif()

target_compile_definitions(mdlconvlib PRIVATE SGE_MDLCONVLIB_BUILDING_DLL=1)
target_include_directories(mdlconvlib PUBLIC "./src")
target_link_libraries(mdlconvlib sge_core)
target_link_libraries(mdlconvlib assimp)

# Add pthread. 
if(NOT WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	target_link_libraries(mdlconvlib ${CMAKE_PTHREAD_LIBS})	
endif()

sgePromoteWarningsOnTarget(mdlconvlib)




