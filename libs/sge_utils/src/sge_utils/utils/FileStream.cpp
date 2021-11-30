//#ifdef WIN32
//#define NOMINMAX
//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//#endif


#if defined(SGE_USE_DEBUG) && defined(_WIN32)
//#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib") // https://docs.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathfindfilenamea
#endif

#include <sys/stat.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "sge_utils/utils/common.h"

#include "FileStream.h"

namespace sge {

int FileReadStream::open(const char* const filename, const size_t bufsz /*= 0*/) {
	close();

	if (filename == nullptr) {
		return 0;
	}

	sge_fopen(&file, filename, "rb");

	if (!file) {
		return 0;
	}

#if defined(SGE_USE_DEBUG) && defined(_WIN32)
	{
		// Check for case sensitivity and fire a warning,
		// as this might produce errors on non-Windows builds.
		WIN32_FIND_DATAA wfd;
		HANDLE hFind = ::FindFirstFileA(filename, &wfd);
		if (hFind != INVALID_HANDLE_VALUE) {
			::FindClose(hFind);
			const char* const foundPathName = ::PathFindFileNameA(filename);
			if (0 != strcmp(wfd.cFileName, foundPathName)) {
				sgeAssert(false && "File case mismatch, the file will be opened on Windows, but will fail on other platforms!");
			}
		}
	}
#endif

	if (bufsz != 0) {
		maxBufferSize = bufsz;
		buffer = new unsigned char[maxBufferSize];
		pointer = 0;
		bufferFileOffset = 0;

		bufferSize = fread(buffer, 1, maxBufferSize, file);
	}

	return 1;
}

// Closes the file and reverts the object to its inital state
void FileReadStream::close() {
	sge_fclose_safe(file);
	file = nullptr;

	delete[] buffer;
	buffer = nullptr;

	maxBufferSize = 0;
	bufferSize = 0;
	pointer = 0;
	bufferFileOffset = 0;
}

bool FileReadStream::isOpened() const {
	return file != nullptr;
}

// Attempts to read data equal in size to numBytes
// @retval: the amount of data actually read
size_t FileReadStream::read(void* destination, size_t numBytes2read) {
	if (maxBufferSize == 0) {
		return fread(destination, 1, numBytes2read, file);
	}

	char* writeLocation = (char*)destination;
	const size_t requestedSize = numBytes2read;
	while (numBytes2read) {
		if (pointer == bufferSize) {
			bufferSize = fread(buffer, 1, maxBufferSize, file);
			if (bufferSize == 0)
				break;

			pointer = 0;
			bufferFileOffset += bufferSize;
		}

		// copy the data from the buffer and move the pointer
		const size_t numBytes2Copy = ((numBytes2read + pointer) <= bufferSize) ? numBytes2read : bufferSize - pointer;

		//[TODO]
		for (size_t t = 0; t < numBytes2Copy; ++t) {
			writeLocation[t] = buffer[pointer + t];
		}

		numBytes2read -= numBytes2Copy;
		pointer += numBytes2Copy;
		writeLocation += numBytes2Copy;
	}

	return requestedSize - numBytes2read;
}

size_t FileReadStream::pointerOffset() {
	if (maxBufferSize == 0) {
		return ftell(file);
	}

	return bufferFileOffset + pointer;
}

void FileReadStream::seek(SeekOrigin origin, size_t bytes) {
	if (maxBufferSize == 0) {
		int fseekResult = 1;

		if (origin == SeekOrigin::Begining) {
			fseekResult = fseek(file, long(bytes), SEEK_SET);
		} else if (origin == SeekOrigin::Current) {
			fseekResult = fseek(file, long(bytes), SEEK_CUR);
		} else if (origin == SeekOrigin::End) {
			fseekResult = fseek(file, long(bytes), SEEK_END);
		} else {
			// unknown origin type
			sgeAssert(false);
			return;
		}

		sgeAssert(fseekResult == 0);
		return;
	}

	size_t seekOffset = 0;
	if (origin == SeekOrigin::Begining)
		seekOffset = bytes;
	else if (origin == SeekOrigin::Current)
		seekOffset = bufferFileOffset + pointer + bytes;
	else if (origin == SeekOrigin::End) {
		fseek(file, long(bytes), SEEK_END);
		seekOffset = size_t(ftell(file));
		fseek(file, long(bufferFileOffset), SEEK_SET);
	} else {
		// unknown origin type
		sgeAssert(false);
	}

	const bool isSeekLocBuffered = (seekOffset >= bufferFileOffset) && (seekOffset <= bufferFileOffset + bufferSize);

	if (isSeekLocBuffered) {
		pointer = seekOffset - bufferFileOffset;
		return;
	}

	fseek(file, long(seekOffset), SEEK_SET);

	bufferSize = fread(buffer, 1, maxBufferSize, file);
	pointer = 0;
	bufferFileOffset = seekOffset;

	return;
}

size_t FileReadStream::remainingBytes() {
	const size_t currOffset = pointerOffset();
	seek(SeekOrigin::End, 0);
	const size_t remainingBytes = pointerOffset() - currOffset;
	seek(SeekOrigin::Begining, currOffset);
	return remainingBytes;
}

bool FileReadStream::readFile(const char* const filename, std::vector<char>& data) {
	FileReadStream frs;
	if (frs.open(filename) == 0) {
		return false;
	}

	const size_t fileSizeBytes = frs.remainingBytes();

	const size_t offset = data.size();
	data.resize(data.size() + fileSizeBytes);

	if (frs.read(data.data() + offset, fileSizeBytes) != fileSizeBytes) {
		data.resize(offset);
		return false;
	}

	return true;
}

bool FileReadStream::readTextFile(const char* const filename, std::string& outText) {
	std::vector<char> data;
	if (FileReadStream::readFile(filename, data) == false) {
		return false;
	}

	outText.reserve(data.size());

	for (char c : data) {
		if (c == '\0') {
			break;
		}

		outText.push_back(c);
	}

	return true;
}

sint64 FileReadStream::getFileModTime(const char* const filename) {
	// https://linux.die.net/man/2/stat
	struct stat result; // Yep "struct", as there is a function with the same name...
	if (stat(filename, &result) == 0) {
		sint64 t = result.st_mtime;
		return t;
	}

	return 0; // TODO: is this really the invalid time?
}

//-------------------------------------------------------------------------
// FileWriteStream
//-------------------------------------------------------------------------

bool FileWriteStream::open(const char* const filename) {
	close();

	if (!filename) {
		return false;
	}

	sge_fopen(&m_file, filename, "wb");

	if (!m_file || ferror(m_file)) {
		return false;
		// sgeLogWarn("fopen failed with errno = %d\n", errno);
	}

	return m_file != nullptr;
}

void FileWriteStream::close() {
	sge_fclose_safe(m_file);
	m_file = nullptr;
}

size_t FileWriteStream::write(const char* data, size_t numBytes) {
	if (m_file) {
		return fwrite(data, 1, numBytes, m_file);
	}

	return 0;
}

} // namespace sge
