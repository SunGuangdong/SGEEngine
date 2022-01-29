#pragma once

#include <cstdio>
#include <vector>

#include "sge_utils/sge_utils.h"

#include "IStream.h"

namespace sge {
//
// struct FileTime
//{
//	int year = 0;
//	int day;
//};

//-------------------------------------------------------------------------
// FileReadStream
//-------------------------------------------------------------------------
class FileReadStream : public IReadStream {
  public:
	FileReadStream()
	    : maxBufferSize(0)
	    , file(nullptr)
	    , buffer(nullptr)
	    , bufferSize(0)
	    , bufferFileOffset(0)
	    , pointer(0) {}

	~FileReadStream() { close(); }

	// Just an errorless shortcut to Open method
	FileReadStream(const char* const filename, const size_t bufsz = 0)
	    : FileReadStream() {
		open(filename, bufsz);
	}

	// Attemplts to open a file for reading.
	// @filename: A file name.
	// @bufferSize: If non-zero the data will be bufferred in one
	// chuck with lenghth bufferSize. 0 means no buffering.
	int open(const char* const filename, const size_t bufsz = 0);

	// Closes the file and reverts the object to its inital state
	void close();

	bool isOpened() const;

	// Attempts to read data equal in size to numBytes
	// @retval: the amount of data actually read
	size_t read(void* destination, size_t numBytes2read) override;

	// Attempts to read full file contests.
	// Retuns false on failure.
	static bool readFile(const char* const filename, std::vector<char>& data);
	static bool readTextFile(const char* const filename, std::string& outText);

	static sint64 getFileModTime(const char* const filename);

	size_t pointerOffset() override;
	void seek(SeekOrigin origin, size_t bytes) override;
	size_t remainingBytes();

  private:
	size_t maxBufferSize;
	FILE* file;

	unsigned char* buffer;
	size_t bufferSize;       // size of the buffer <= maxBufferSize
	size_t pointer;          // the pointer in the buffer
	size_t bufferFileOffset; // buffers location in the file
};

//-------------------------------------------------------------------------
// FileWriteStream
//-------------------------------------------------------------------------
class FileWriteStream : public IWriteStream {
  public:
	FileWriteStream()
	    : m_file(nullptr) {}

	~FileWriteStream() { close(); }

	bool open(const char* const filename);
	void close();
	size_t write(const char* data, size_t numBytes) override;

  private:
	FILE* m_file;
};

} // namespace sge
