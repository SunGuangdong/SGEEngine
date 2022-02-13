#include "VSDebugger.h"

#ifdef WIN32
	#include "sge_utils/text/format.h"

	#define NOMINMAX
	#include <Windows.h>
#endif

namespace sge {

#ifdef WIN32
bool startVisualStudioDebugging()
{
	// Start Visual Studio JIT. It will open a dialogue to ask the user
	// to select how he is going to debug the application.
	STARTUPINFOA si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));


	// Get process ID and create the command line
	DWORD pid = GetCurrentProcessId();

	std::string cmdVSJit = string_format("vsjitdebugger.exe -p %llu", pid);
	if (!CreateProcessA(NULL, cmdVSJit.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		return false;
	}

	// The block below is copied from the internet I have no idea if it is really needed.
	{
		// Close debugger process handles to eliminate resource leak.
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	return true;
}

bool isVSDebuggerPresent()
{
	return IsDebuggerPresent() != FALSE;
}
#else // #ifdef WIN32

bool startVisualStudioDebugging()
{
	return false;
}

bool isVSDebuggerPresent()
{
	return false;
}

#endif // #ifdef WIN32

} // namespace sge
