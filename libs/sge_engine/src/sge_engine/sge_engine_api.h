#pragma once

#if defined(__EMSCRIPTEN__)
	#define SGE_ENGINE_API
#else
	#if WIN32
		#ifdef SGE_ENGINE_BUILDING_DLL
			#define SGE_ENGINE_API __declspec(dllexport)
		#else
			#define SGE_ENGINE_API __declspec(dllimport)
		#endif
	#else
		#ifdef SGE_ENGINE_BUILDING_DLL
			#define SGE_ENGINE_API __attribute__((visibility("default")))
		#else
			#define SGE_ENGINE_API
		#endif
	#endif
#endif