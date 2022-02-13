#pragma once

#if defined(__EMSCRIPTEN__)
	#define SGE_LOG_API
#else
	#if WIN32
		#ifdef SGE_LOG_BUILDING_DLL
			#define SGE_LOG_API __declspec(dllexport)
		#else
			#define SGE_LOG_API __declspec(dllimport)
		#endif
	#else
		#ifdef SGE_LOG_BUILDING_DLL
			#define SGE_LOG_API __attribute__((visibility("default")))
		#else
			#define SGE_LOG_API
		#endif
	#endif
#endif
