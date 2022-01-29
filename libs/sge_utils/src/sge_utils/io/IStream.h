#pragma once

#include <string.h>
#include <string>

namespace sge {
//[TODO] Add seeking in those streams

enum class SeekOrigin {
	Begining,
	Current,
	End,
};

class IReadStream {
  public:
	IReadStream() = default;
	virtual ~IReadStream() {}
	/// Attempts to read data equal in size to numBytes
	/// @retval: the amount of data actually read
	virtual size_t read(void* destination, size_t numBytes) = 0;
	virtual size_t pointerOffset() = 0;
	virtual void seek(SeekOrigin origin, size_t bytes) = 0;
};

class IWriteStream {
  public:
	IWriteStream() = default;
	virtual ~IWriteStream() {}
	/// Attempts to write data equal in size to numBytes
	/// @retval: the amount of data actually written
	virtual size_t write(const char* src, size_t numBytes) = 0;
};


class ReadCStringStream : public IReadStream {
  public:
	ReadCStringStream()
	    : string(nullptr)
	    , lenght(0)
	    , pointer(0) {}

	ReadCStringStream(const char* const string)
	    : string(string)
	    , lenght(strlen(string))
	    , pointer(0) {}

	// Attempts to read data equal in size to numBytes
	// @retval: the amount of data actually read
	inline size_t read(void* destination, size_t numBytes) override {
		sgeAssert(destination);
		char* chdest = (char*)destination;

		size_t bytesRead = 0;
		while ((bytesRead < numBytes) && (pointer < lenght)) {
			chdest[bytesRead] = string[pointer];
			pointer++;
			bytesRead++;
		}

		return bytesRead;
	}

	inline size_t pointerOffset() { return pointer; }

	inline void seek(SeekOrigin origin, size_t bytes) {
		switch (origin) {
			case SeekOrigin::Begining:
				pointer = bytes;
				break;
			case SeekOrigin::Current:
				pointer += bytes;
				break;
			case SeekOrigin::End:
				pointer = lenght;
				break;
			default:
				sgeAssert(false); // Should never happen.
		}
		if (pointer > lenght)
			pointer = lenght;
	}

  private:
	const char* string;
	size_t lenght;
	size_t pointer;
};


class WriteStdStringStream : public IWriteStream {
  public:
	std::string serializedString;

	// Attempts to write data equal in size to numBytes
	// @retval: the amount of data actually written
	size_t write(const char* src, size_t numBytes) final {
		serializedString.reserve(serializedString.size() + numBytes);

		const char* srcChar = (char*)src;
		size_t bytesToWrite = numBytes;
		while (bytesToWrite) {
			serializedString += *srcChar;

			srcChar++;
			bytesToWrite--;
		}

		return numBytes;
	}
};

class WriteByteStream : public IWriteStream {
  public:
	std::vector<char> serializedData;

	// Attempts to write data equal in size to numBytes
	// @retval: the amount of data actually written
	size_t write(const char* src, size_t numBytes) final {
		serializedData.reserve(serializedData.size() + numBytes);

		const char* srcChar = (char*)src;
		size_t bytesToWrite = numBytes;
		while (bytesToWrite) {
			serializedData.emplace_back(*srcChar);
			srcChar++;
			bytesToWrite--;
		}

		return numBytes;
	}
};

class ReadByteStream : public IReadStream {
  public:
	ReadByteStream() = default;

	/// @brief Creates the stream by reading the bytes form the specified vector.
	/// The input vector should not be changed while using this class. The class doesn't create it's own copy.
	/// @param vector the vector to be used for reading the data.
	ReadByteStream(const std::vector<char>& vector)
	    : data(vector.data())
	    , dataSizeBytes(vector.size())
	    , pointer(0) {}

	ReadByteStream(const char* const data, size_t numBytes)
	    : data(data)
	    , dataSizeBytes(numBytes)
	    , pointer(0) {}

	// Attempts to read data equal in size to numBytes
	// @retval: the amount of data actually read
	inline size_t read(void* destination, size_t numBytes) override {
		sgeAssert(destination);
		char* chdest = (char*)destination;

		size_t bytesRead = 0;
		while ((bytesRead < numBytes) && (pointer < dataSizeBytes)) {
			chdest[bytesRead] = data[pointer];
			pointer++;
			bytesRead++;
		}

		return bytesRead;
	}

	inline size_t pointerOffset() { return pointer; }

	inline void seek(SeekOrigin origin, size_t bytes) {
		switch (origin) {
			case SeekOrigin::Begining:
				pointer = bytes;
				break;
			case SeekOrigin::Current:
				pointer += bytes;
				break;
			case SeekOrigin::End:
				pointer = dataSizeBytes;
				break;
			default:
				sgeAssert(false); // Should never happen.
		}
		if (pointer > dataSizeBytes)
			pointer = dataSizeBytes;
	}

  private:
	const char* data = nullptr;
	size_t dataSizeBytes = 0;
	size_t pointer = 0;
};

} // namespace sge
