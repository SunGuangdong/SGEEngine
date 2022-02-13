#pragma once

#include "sge_log_api.h"

#include <string>
#include <vector>

#include "sge_utils/types.h"

namespace sge {

struct SGE_LOG_API Log : public NoCopy {
	enum MessageType : int {
		/// Just a message that something has been done.
		messageType_log,
		/// A message that something important or anticipated has been done. Usually a normal message that we want to outline ot the user.
		messageType_check,
		/// A warrning that something might be worng or the user needs to pay attention.
		messageType_warning,
		/// An message that singnal to the user that something failed or couldn't be done.
		messageType_error,
	};

	struct Message {
		Message(MessageType type, std::string msg)
		    : type(type)
		    , message(std::move(msg))
		{
		}

		MessageType type;
		std::string message;
	};

	void write(const char* format, ...);
	void writeCheck(const char* format, ...);
	void writeError(const char* format, ...);
	void writeWarning(const char* format, ...);

	const std::vector<Message>& getMessages() const { return m_messages; }

  private:
	std::string m_logFilename;
	std::vector<Message> m_messages;
};

#if defined(SGE_USE_DEBUG)
    // https://stackoverflow.com/questions/5588855/standard-alternative-to-gccs-va-args-trick
	#define sgeLogInfo(_str_msg_, ...)                        \
		{                                                     \
			sge::getLog()->write((_str_msg_), ##__VA_ARGS__); \
		}
	#define sgeLogCheck(_str_msg_, ...)                            \
		{                                                          \
			sge::getLog()->writeCheck((_str_msg_), ##__VA_ARGS__); \
		}
	#define sgeLogError(_str_msg_, ...)                            \
		{                                                          \
			sge::getLog()->writeError((_str_msg_), ##__VA_ARGS__); \
		}
	#define sgeLogWarn(_str_msg_, ...)                               \
		{                                                            \
			sge::getLog()->writeWarning((_str_msg_), ##__VA_ARGS__); \
		}
#else
	#define sgeLogInfo(_str_msg_, ...) \
		{                              \
		}
	#define sgeLogCheck(_str_msg_, ...) \
		{                               \
		}
	#define sgeLogError(_str_msg_, ...) \
		{                               \
		}
	#define sgeLogWarn(_str_msg_, ...) \
		{                              \
		}
#endif

/// The global of the current module(dll, exe, ect.). However we could "borrow" another modules "ICore" and use theirs.
SGE_LOG_API Log* getLog();
SGE_LOG_API void setLog(Log* global);

} // namespace sge
