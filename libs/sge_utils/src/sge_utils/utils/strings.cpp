#include <array>
#include <clocale>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include "strings.h"

int sge_snprintf(char* s, size_t n, const char* format, va_list args) {
	std::setlocale(LC_NUMERIC, "C");
	const int result = vsnprintf(s, n, format, args);
	return result;
}

int sge_snprintf(char* s, size_t n, const char* format, ...) {
	va_list args;
	va_start(args, format);
	const int result = sge_snprintf(s, n, format, args);
	va_end(args);

	return result;
}

void sge_strcpy(char* dest, size_t SZ, const char* source) {
#ifdef _MSC_VER
	strcpy_s(dest, SZ, source);
#else
	strcpy(dest, source);
#endif
}

// The caller is EXPECTED to call va_end on the va_list args
void string_format(std::string& retval, const char* const fmt_str, va_list args) {
	// [CAUTION]
	// Under Windows with msvc it is fine to call va_start once and then use the va_list multiple times.
	// However this is not the case on the other plafroms. The POSIX docs state that the va_list is undefined
	// after calling vsnprintf with it:
	//
	// From https://linux.die.net/man/3/vsprintf :
	// The functions vprintf(), vfprintf(), vsprintf(), vsnprintf() are equivalent to the functions
	// printf(), fprintf(), sprintf(), snprintf(), respectively,
	// except that they are called with a va_list instead of a variable number of arguments.
	// These functions do not call the va_end macro. Because they invoke the va_arg macro,
	// the value of ap is undefined after the call. See stdarg(3).
	// Obtain the length of the result string.
	va_list args_copy;
	va_copy(args_copy, args);

	const size_t ln = sge_snprintf(nullptr, 0, fmt_str, args);

	// [CAUTION] Write the data do the result. Allocate one more element
	// as the *sprintf function add the '\0' always. Later we will pop that element.
	retval.resize(ln + 1, 'a');

	sge_snprintf(&retval[0], retval.size() + 1, fmt_str, args_copy);
	retval.pop_back(); // remove the '\0' that was added by snprintf on the back.
	va_end(args_copy);
}

void string_format(std::string& retval, const char* const fmt_str, ...) {
	va_list args;
	va_start(args, fmt_str);
	string_format(retval, fmt_str, args);
	va_end(args);
}

std::string string_format(const char* const fmt_str, ...) {
	std::string retval;

	va_list args;
	va_start(args, fmt_str);
	string_format(retval, fmt_str, args);
	va_end(args);

	return retval;
}

bool string_endsWith(const std::string& fullString, char* const ending) {
	const int endingLength = int(strlen(ending));
	if (fullString.length() >= endingLength) {
		return (0 == fullString.compare(fullString.length() - endingLength, endingLength, ending));
	}

	return false;
}

int sge_stricmp(const char* a, const char* b) {
#if defined(WIN32)
	return _stricmp(a, b);
#else
	return strcasecmp(a, b);
#endif
}

bool isStringEmpty(const char* const str) {
	return str == nullptr || str[0] == '\0';
}
