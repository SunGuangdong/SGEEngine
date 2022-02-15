#include "base64.h"

namespace sge {

char base64_index_of(const char c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	else if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	else if (c == '+')
		return 62;
	else if (c == '/')
		return 63;
	return c - '0' + 52;
}

char base64_is_base64_data_char(const char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '+') || (c >= '/' && c <= '9');
}

size_t base64_get_encoded_strlen(const size_t dataSize)
{
	// for every 3 bytes we assing 4 characters
	// if the number of bytes isn't multiple of 3 add 4 more characters
	return ((dataSize / 3) + (dataSize % 3 ? 1 : 0)) * 4;
}

size_t base64_get_decoded_datasize_approx(const size_t numCharacters)
{
	return (numCharacters / 4) * 3;
}

size_t base64_get_decoded_datasize_exact(const char* const encodedString, size_t numCharacters)
{
	size_t retval = ((numCharacters - 4) / 4) * 3;

	// process the last 4 character separatley, because they can contain '='
	// assume that there aren't any '='
	retval += 3;

	if (encodedString[numCharacters - 1] == '=')
		retval--;
	if (encodedString[numCharacters - 2] == '=')
		retval--;

	return retval;
}

bool base64_is_correctly_encoded(const char* const encodedString, size_t stringLength)
{
	if (stringLength % 4 != 0) {
		return false;
	}

	for (size_t t = 0; t < stringLength - 2; ++t) {
		if (!base64_is_base64_data_char(encodedString[t])) {
			return false;
		}
	}

	// the last 2 chacater could be :
	//
	// 1) <data-char><data-char>
	// 2) <data-char>=
	// 3) ==
	//
	// everything elese is invlid
	if (base64_is_base64_data_char(encodedString[stringLength - 2])) {
		// 1) and 2)
		return (base64_is_base64_data_char(encodedString[stringLength - 1]) || encodedString[stringLength - 1] == '=');
	}
	else if (encodedString[stringLength - 2] == '=') {
		// 3)
		return encodedString[stringLength - 1] == '=';
	}

	return false;
}

void base64_encode(const void* vptrData, size_t datasize, char* encodedString)
{
	const char* data = (char*)(vptrData);

	const char base64_characters[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz"
	    "0123456789+/";

	// the number of left out bytes that need speical attetion
	const size_t paddedBytes = datasize - ((datasize / 3) * 3);
	datasize -= paddedBytes;

	char encodedIndices[4];
	while (datasize) {
		encodedIndices[0] = ((data[0] & 0xfc) >> 2);
		encodedIndices[1] = ((data[0] & 0x03) << 4) + ((data[1] & 0xf0) >> 4);
		encodedIndices[2] = ((data[1] & 0x0f) << 2) + ((data[2] & 0xc0) >> 6);
		encodedIndices[3] = ((data[2] & 0x3f));

		encodedString[0] = base64_characters[encodedIndices[0]];
		encodedString[1] = base64_characters[encodedIndices[1]];
		encodedString[2] = base64_characters[encodedIndices[2]];
		encodedString[3] = base64_characters[encodedIndices[3]];

		data += 3;
		encodedString += 4;
		datasize -= 3;
	}

	// check if the data size wansn't mutiple of 3
	// if so a special care must be taken
	if (paddedBytes != 0) {
		// the maximum amout of charracter needed to be encodeed is 2 bytes
		// pretend that there are 2 bytes
		encodedIndices[0] = ((data[0] & 0xfc) >> 2);
		encodedIndices[1] = ((data[0] & 0x03) << 4) + ((data[1] & 0xf0) >> 4);
		encodedIndices[2] = ((data[1] & 0x0f) << 2);

		// add the =. they are only needed to make the size of the string multiple of 4
		// they do not carry any information
		encodedString[0] = base64_characters[encodedIndices[0]];
		encodedString[1] = base64_characters[encodedIndices[1]];
		encodedString[2] =
		    (paddedBytes == 1)
		        ? '='
		        : base64_characters[encodedIndices[2]]; // if exactly one byte is converted this must be '='
		encodedString[3] = '=';                         // this is always '='
	}
}

void base64_decode(const char* encodedString, size_t stringLength, void* vptrDecodedData)
{
	char* decodedData = (char*)(vptrDecodedData);

	// Last 4 elements will be processed separetrly because they can contain '='
	stringLength -= 4;

	char indices[4];
	while (stringLength) {
		for (char i = 0; i < 4; ++i) {
			indices[i] = base64_index_of(encodedString[i]);
		}

		decodedData[0] = (indices[0] << 2) + ((indices[1] & 0x30) >> 4);
		decodedData[1] = ((indices[1] & 0x0f) << 4) + ((indices[2] & 0x3c) >> 2);
		decodedData[2] = ((indices[2] & 0x03) << 6) + indices[3];

		encodedString += 4;
		decodedData += 3;
		stringLength -= 4;
	}

	// process the last 4 symbols
	for (char i = 0; i < 4; ++i) {
		indices[i] = base64_index_of(encodedString[i]);
	}

	//[CAUTION] last 4 symbols COULD contain '='. but it's not a must....

	// exclude non real data containing symbols
	decodedData[0] = (indices[0] << 2) + ((indices[1] & 0x30) >> 4);

	if (encodedString[2] != '=')
		decodedData[1] = ((indices[1] & 0x0f) << 4) + ((indices[2] & 0x3c) >> 2);

	if (encodedString[2] != '=' && encodedString[3] != '=')
		decodedData[2] = ((indices[2] & 0x03) << 6) + indices[3];
}


std::string base64Encode(const void* data, size_t datasize)
{
	size_t strLen = base64_get_encoded_strlen(datasize);

	std::vector<char> encodedDataRaw;
	encodedDataRaw.resize(strLen + 1, '\0');
	base64_encode(data, datasize, encodedDataRaw.data());

	sgeAssert(encodedDataRaw.back() == '\0');

	return std::string(encodedDataRaw.data());
}

bool base64Decode(const char* encodedString, size_t stringLength, std::vector<char>& outDecodedData)
{
	if (base64_is_correctly_encoded(encodedString, stringLength)) {
		const size_t decodedDataSizeBytes = base64_get_decoded_datasize_exact(encodedString, stringLength);
		outDecodedData.resize(decodedDataSizeBytes);
		base64_decode(encodedString, stringLength, outDecodedData.data());
		return true;
	}

	return false;
}

} // namespace sge
