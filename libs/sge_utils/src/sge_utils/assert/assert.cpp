#include "sge_utils/assert/assert.h"
#include "sge_utils/text/format.h"
#include <string>

#ifdef WIN32
	#include <Windows.h>
#endif

namespace sge {

int assertAskDisableInternal(const char* const file, const int line, const char* expr)
{
	const std::string title = string_format(
	    "%s %d triggered an assert, Do you want to Disable it?\nNo - Debug\nYes - Disable and Debug\nCancel - Disable "
	    "and Ignore",
	    file,
	    line);

#ifdef WIN32
	int code = MessageBoxA(GetActiveWindow(), expr, title.c_str(), MB_YESNOCANCEL);
	if (code == IDNO) {
		// Don't disable the breakpoint and debug.
		return 0;
	}
	if (code == IDYES) {
		// Disable the breakpoint but debug now.
		return 1;
	}

	// Disable the breakpoint and do not debug.
	return 2;
#else
	return false;
#endif
}

} // namespace sge

int sge_assertAskDisable(const char* const file, const int line, const char* expr)
{
	return sge::assertAskDisableInternal(file, line, expr);
}
