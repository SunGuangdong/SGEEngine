#pragma once

#include "sge_utils/sge_utils.h"
#include <string>
#include <vector>

namespace sge {

//////////////////////////////////////////////////////////////////////////////////////////////
// Base64 encoding/decoding based on :
// https://www.base64encode.org/ and http://en.wikipedia.org/wiki/Base64
//
// !!! All string operatarions DO NOT take into account the NULL TERMINATOR !!!
//////////////////////////////////////////////////////////////////////////////////////////////

// returns the needed string size for that particular data size(nullterminator isn't included)
size_t base64_get_encoded_strlen(const size_t dataSize);

// returns large enough buffer size that can hold the decoded string. numCharacters MUST be multiple of 4
size_t base64_get_decoded_datasize_approx(const size_t numCharacters);

// returns the exact buffer size that can hold the decoded string. numCharacters MUST be multiple of 4
size_t base64_get_decoded_datasize_exact(const char* const encodedString, size_t numCharacters);

// checks if the input string is a correctly encoded base64 string.
bool base64_is_correctly_encoded(const char* const encodedString, size_t stringLength);

// Encodes the data. encodedString MUST be preallocated!
void base64_encode(const void* data, size_t datasize, char* encodedString);

// Decodes base64 string data. decodedData MUST be preallocated.
// stringLength MUST be multiple of 4, not including the null terminatior.
// Works with inplace decoding.
void base64_decode(const char* encodedString, size_t stringLength, void* decodedData);


std::string base64Encode(const void* data, size_t datasize);
bool base64Decode(const char* encodedString, size_t stringLength, std::vector<char>& outDecodedData);

} // namespace sge
