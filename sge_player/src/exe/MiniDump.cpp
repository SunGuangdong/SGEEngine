#include "MiniDump.h"

#if WIN32

    // clang-format off
// The include order here matters.
#include <Windows.h>
#include <DbgHelp.h>
#include <tchar.h> // For the _T macro.
// clang-format on

void sgeCreateMiniDump(EXCEPTION_POINTERS* pep)
{
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

	HANDLE hFile = CreateFile(_T("minidump.dmp"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)) {
		// Create the minidump file.
		MINIDUMP_EXCEPTION_INFORMATION mdei;

		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = FALSE;

		MINIDUMP_TYPE mdt = MiniDumpNormal;

		[[maybe_unused]] BOOL rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (pep != 0) ? &mdei : 0, 0, 0);

		// Close the file

		CloseHandle(hFile);
	}
}

LONG WINAPI sgeCrashHandler(EXCEPTION_POINTERS* except)
{
	sgeCreateMiniDump(except);
	#if SGE_USE_DEBUG
	return EXCEPTION_EXECUTE_HANDLER; // Try to debug.
	#else
	return EXCEPTION_CONTINUE_SEARCH; // Just continue.
	#endif
}
#endif

void sgeRegisterMiniDumpHandler()
{
#if WIN32
	SetUnhandledExceptionFilter(sgeCrashHandler);
#endif
}
