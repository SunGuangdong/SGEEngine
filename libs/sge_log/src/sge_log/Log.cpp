#include "Log.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/strings.h"
#include <stdarg.h>

namespace sge {

void Log::write(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string buffer;
	string_format(buffer, format, args);
	va_end(args);

	m_messages.emplace_back(Message(messageType_log, buffer));
}

void Log::writeCheck(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string buffer;
	string_format(buffer, format, args);
	va_end(args);

	m_messages.emplace_back(Message(messageType_check, buffer));
}

void Log::writeError(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string buffer;
	string_format(buffer, format, args);
	va_end(args);

	m_messages.emplace_back(Message(messageType_error, buffer));
}

void Log::writeWarning(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string buffer;
	string_format(buffer, format, args);
	va_end(args);

	m_messages.emplace_back(Message(messageType_warning, buffer));
}

Log g_moduleLocalLog;
Log* g_pWorkingLog = &g_moduleLocalLog;

Log* getLog() {
	return g_pWorkingLog;
}

void setLog(Log* newLog) {
	sgeAssert(newLog);
	g_pWorkingLog = newLog;
}

} // namespace sge
