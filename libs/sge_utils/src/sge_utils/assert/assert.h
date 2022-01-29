#pragma once

#if defined(__GNUC__)
	#include <signal.h>
#endif


/// A function that fires up the UI when a breakpoint is hit.
/// Not in the namespace because it is used in a macro and the macro could do strange things.
int sge_assertAskDisable(const char* const file, const int line, const char* expr);

namespace sge {

#if defined(SGE_USE_DEBUG)
	#if defined(__EMSCRIPTEN__)
	    // Emscripten:
	    // Does nothing, because Emscripten disables asm.js vailation if we use __builtin_debugtrap()
		#define SGE_BREAK() void(0)
	#elif defined(_WIN32)
	    // Windows:
		#define SGE_BREAK() __debugbreak()
	#elif defined(__clang__)
	    // Clang:
		#define SGE_BREAK() __builtin_debugtrap()
	#elif __GNUC__
	    // GCC:
		#define SGE_BREAK() raise(SIGTRAP)
	#else
		#define SGE_BREAK() void(0)
		#warning SGE_BREAK is not defined for that platform.
	#endif
#else
	#define SGE_BREAK() void(0)
#endif

#if defined(SGE_USE_DEBUG)
	#define sgeAssert(__exp)                                                         \
		do {                                                                         \
			static int isBrakPointDisabled = 0;                                      \
			if (!isBrakPointDisabled && (false == bool(__exp))) {                    \
				int responseCode = sge_assertAskDisable(__FILE__, __LINE__, #__exp); \
				isBrakPointDisabled = !isBrakPointDisabled && responseCode != 0;     \
				if (responseCode != 2) {                                             \
					SGE_BREAK();                                                     \
				}                                                                    \
			}                                                                        \
		} while (false)
	#ifdef __clang__
		#define if_checked(expr) if ((expr))
	#else
		#define if_checked(expr) if ((expr) ? true : (SGE_BREAK(), false))
	#endif
#else
	#define sgeAssert(__exp) \
		{                    \
		}
	#define if_checked(expr) if (expr)
#endif

#define sgeAssertFalse(msg) sgeAssert(false && msg)

} // namespace sge
