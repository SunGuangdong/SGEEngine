#pragma once

#include <stdarg.h>
#include <string>

inline bool sge_isspace(const char ch) {
	return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

inline bool sge_isdigit(const char ch) {
	return ch >= '0' && ch <= '9';
}

inline bool sge_isalpha(const char ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

int sge_snprintf(char* s, size_t n, const char* format, va_list args);

int sge_snprintf(char* s, size_t n, const char* format, ...);

template <size_t SZ>
inline int sge_snprintf(char (&s)[SZ], const char* format, va_list args) {
	const int result = sge_snprintf(s, SZ, format, args);
	return result;
}

// template <size_t SZ>
// int sge_snprintf(char (&s)[SZ], const char* format, ...) {
//	va_list args;
//	va_start(args, format);
//	const int result = sge_snprintf(s, SZ, format, args);
//	va_end(args);
//
//	return result;
//}

template <size_t SZ>
inline void sge_strcpy(char (&dest)[SZ], const char* source) {
#ifdef _MSC_VER
	strcpy_s(dest, source);
#else
	strcpy(dest, source);
#endif
}

void sge_strcpy(char* dest, size_t SZ, const char* source);

/// Caution: The caller is EXPECTED to call va_end() on the va_list args, see the implementation for more details!
void string_format(std::string& retval, const char* const fmt_str, va_list args);
void string_format(std::string& retval, const char* const fmt_str, ...);

std::string string_format(const char* const fmt_str, ...);

bool string_endsWith(const std::string& fullString, char* const ending);

int sge_stricmp(const char* a, const char* b);

bool isStringEmpty(const char* const str);
