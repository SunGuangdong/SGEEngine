#pragma once

/// Every function what we want to be acessible by the code outside of sge_core (when built as DLL/SO)
/// needs to be marked with SGE_CORE_API macro so that the compiler/linker could resolve it.
#if defined(__EMSCRIPTEN__)
	#define SGE_CORE_API
#else
	#if WIN32
		#ifdef SGE_CORE_BUILDING_DLL
			#define SGE_CORE_API __declspec(dllexport)
		#else
			#define SGE_CORE_API __declspec(dllimport)
		#endif
	#else
		#ifdef SGE_CORE_BUILDING_DLL
			#define SGE_CORE_API __attribute__((visibility("default")))
		#else
			#define SGE_CORE_API
		#endif
	#endif
#endif
