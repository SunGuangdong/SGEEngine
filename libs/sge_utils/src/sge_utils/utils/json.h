#pragma once

#include <array>
#include <string>
#include <vector>

#include "sge_utils/sge_utils.h"

namespace sge {

class IReadStream;
class IWriteStream;

struct JsonValueBuffer;

/////////////////////////////////////////////////////////////////////////
// Json control characters IDs
/////////////////////////////////////////////////////////////////////////
enum JID : unsigned char {
	JID_NULL = 0x00,
	JID_MAP_BEGIN = 0x7b,
	JID_MAP_END = 0x7d,
	JID_ARRAY_BEGIN = 0x5b,
	JID_ARRAY_END = 0x5d,
	JID_INT8 = 0x11,
	JID_INT16 = 0x12,
	JID_INT32 = 0x13,
	JID_INT64 = 0x14,
	JID_REAL16 = 0x18,
	JID_REAL32 = 0x19,
	JID_REAL64 = 0x1a,
	JID_UINT8 = 0x21,
	JID_UINT16 = 0x22,
	JID_STRING = 0x27,
	JID_FALSE = 0x30,
	JID_TRUE = 0x31,
	JID_TOKENDEF = 0x2b,
	JID_TOKENREF = 0x26,
	JID_TOKENUNDEF = 0x2d,
	JID_KEY_SEPARATOR = 0x3a,
	JID_VALUE_SEPARATOR = 0x2c,

	// not stored in file just a parsing helper
	JID_SOME_NUMBER,

	// Not sure if this is really necesarry
	// [ ALIASING ]
	JID_MAP = JID_MAP_BEGIN,
	JID_ARRAY = JID_ARRAY_BEGIN,
};

struct JsonExceptAccess {};

/////////////////////////////////////////////////////////////////////////
// struct JsonValue
/////////////////////////////////////////////////////////////////////////
struct JsonValue {
	void clear() {
		jid = JID_NULL;
		uniformData.clear();
		arrayValues.clear();
		members.clear();
	}

	JsonValue() {
		clear();
	}

	//[TODO] HALF
	union {
		double value_double;
		float value_float;
		char value_int8;
		unsigned char value_uint8;
		short value_int16;
		unsigned short value_uint16;
		sint32 value_int32;
		sint64 value_int64;
		uint32 value_uint32;
		uint64 value_uint64;
	};

	// general setters
	void setBool(const bool v) {
		jid = v ? JID_TRUE : JID_FALSE;
	}
	void setFloat(const float v);
	void setInt32(const sint32 v);
	void setUInt32(const uint32 v);
	void setString(const char* const str);

	// map operators
	JsonValue* setMember(const char* const name, JsonValue* value); // Returns value.
	const JsonValue* getMember(const char* const name) const;

	/// @brief Retrieves a member or throws JsonExceptAccess is missing or its type is different form @explectedType if != JID_NULL.
	const JsonValue& getMemberOrThrow(const char* const name, JID explectedType = JID_NULL) const;

	// const JsonValue& operator[](const char* const) const;

	// Array operators
	JsonValue* arrPush(JsonValue* value); // returns value
	size_t arrSize() const {
		return arrayValues.size();
	}
	const JsonValue* arrAt(const size_t index) const {
		return arrayValues[index];
	}
	const std::vector<JsonValue*>& arr() const {
		return arrayValues;
	}

	const char* GetString() const {
		return (char*)uniformData.data();
	}
	const char* GetStringOrThrow() const {
		if (jid != JID_STRING) {
			throw JsonExceptAccess();
		}
		return (char*)uniformData.data();
	}

	template <typename T>
	T getNumberAs() const {
#pragma warning(push)
#pragma warning(disable : 4800) // forcing value to bool

		if (jid == JID_INT8)
			return static_cast<T>(value_int8);
		else if (jid == JID_INT16)
			return static_cast<T>(value_int16);
		else if (jid == JID_INT32)
			return static_cast<T>(value_int32);
		else if (jid == JID_INT64)
			return static_cast<T>(value_int64);
		// else if(jid == JID_REAL16) [TODO]
		else if (jid == JID_REAL32)
			return static_cast<T>(value_float);
		else if (jid == JID_REAL64)
			return static_cast<T>(value_double);
		else if (jid == JID_UINT8)
			return static_cast<T>(value_uint8);
		else if (jid == JID_UINT16)
			return static_cast<T>(value_uint16);
		else if (jid == JID_TRUE)
			return 1;
		else if (jid == JID_FALSE)
			return 0;

#pragma warning(pop)

		// Unknown JID Type
		sgeAssert(false && "unkniwn JsonValue type");
		return (T)0;
	}

	template <typename T>
	T getNumberAsOrThrow() const {
#pragma warning(push)
#pragma warning(disable : 4800) // forcing value to bool

		if (jid == JID_INT8)
			return static_cast<T>(value_int8);
		else if (jid == JID_INT16)
			return static_cast<T>(value_int16);
		else if (jid == JID_INT32)
			return static_cast<T>(value_int32);
		else if (jid == JID_INT64)
			return static_cast<T>(value_int64);
		// else if(jid == JID_REAL16) [TODO]
		else if (jid == JID_REAL32)
			return static_cast<T>(value_float);
		else if (jid == JID_REAL64)
			return static_cast<T>(value_double);
		else if (jid == JID_UINT8)
			return static_cast<T>(value_uint8);
		else if (jid == JID_UINT16)
			return static_cast<T>(value_uint16);
		else if (jid == JID_TRUE)
			return 1;
		else if (jid == JID_FALSE)
			return 0;

#pragma warning(pop)

		throw JsonExceptAccess();
	}

	bool getAsBool() const {
		if (jid == JID_INT8)
			return !!(value_int8);
		else if (jid == JID_INT16)
			return !!(value_int16);
		else if (jid == JID_INT32)
			return !!(value_int32);
		else if (jid == JID_INT64)
			return !!(value_int64);
		// else if(jid == JID_REAL16) [TODO]
		else if (jid == JID_REAL32)
			return !!(value_float);
		else if (jid == JID_REAL64)
			return !!(value_double);
		else if (jid == JID_UINT8)
			return !!(value_uint8);
		else if (jid == JID_UINT16)
			return !!(value_uint16);
		else if (jid == JID_TRUE)
			return true;
		else if (jid == JID_FALSE)
			return false;

		// Unknown JID Type
		sgeAssert(false && "unkniwn JsonValue type");

		return false;
	}

	// Reads a json array of Numbers and stores them into arr (must be preallocated)
	template <typename T>
	bool getNumberArrayAs(T* arr, size_t numElements) const {
		if (!arr || !isArray() || (arrSize() < numElements)) {
			sgeAssert(false);
			return false;
		}

		for (size_t t = 0; t < numElements; ++t) {
			arr[t] = arrAt(t)->getNumberAs<T>();
		}

		return true;
	}

	bool isArray() const {
		return jid == JID_ARRAY_BEGIN;
	}
	bool isMap() const {
		return jid == JID_MAP_BEGIN;
	}
	bool isString() const {
		return jid == JID_STRING;
	}

	JID jid;                                // The ID of the variable type.
	std::vector<unsigned char> uniformData; // Just a data buffer, currently used for strings.
	std::vector<JsonValue*> arrayValues;
	std::vector<std::pair<std::vector<unsigned char>, JsonValue*>> members;

	static JsonValue* Clone(const JsonValue& root, JsonValueBuffer& jvb);
};

/////////////////////////////////////////////////////////////////////////
// JsonValueBuffer
/////////////////////////////////////////////////////////////////////////
struct JsonValueBuffer {
  public:
	//[TODO] ChunkSize is currently randomly hardcoded.
	JsonValueBuffer(size_t ChunkSize = 100)
	    : ChunkSize(ChunkSize)
	    , m_pointer(0) {
	}

	~JsonValueBuffer() {
		clearAllValues();
	}

	// allocates a new value
	JsonValue* GetNewValue();

	JsonValue* operator()(const std::string& str);
	JsonValue* operator()(const char* const str);
	JsonValue* operator()(const float);
	JsonValue* operator()(const int);
	JsonValue* operator()(const unsigned);
	JsonValue* operator()(const bool);
	JsonValue* operator()(const JID);

	JsonValue* operator()(const float* const arr, int size);
	JsonValue* operator()(const int* const arr, int size);

	JsonValue* operator()(const std::vector<float>& vec);
	JsonValue* operator()(const std::vector<int>& vec);
	JsonValue* operator()(const std::vector<std::string>& vec);

	// deletes all values
	void clearAllValues();

  private:
	size_t ChunkSize;
	std::vector<JsonValue*> m_valuesBuffer;
	size_t m_pointer;
};

/////////////////////////////////////////////////////////////////////////
// JsonParser
/////////////////////////////////////////////////////////////////////////
class JsonParser : protected JsonValueBuffer {
  public:
	JsonParser() {
	}
	void Clear();

	bool parse(IReadStream* instream);
	JsonValue* getRoot() {
		return root;
	}
	const JsonValue* getRoot() const {
		return root;
	}
	const char* getErrorMsg() const {
		return parsingErrorMsg;
	}

  private:
	JID getNextJID();                        // returns the JID of the next element
	bool skipSeparatorAhead(char separator); // skips to 1st non space symbol. retval is (symbol == separator)
	void skipSpacesAhead();                  // skips all spaces ahead
	void readString(std::vector<unsigned char>& str,
	                const bool procStringTokens); // if procStringTokens is true than the parser will try to convert \n \r \t symbols
	void readNumber();                            // reads a number and stores the string in (numconvert,numconvertidx)

	// reads a value form the stream
	JsonValue* parseValue(JID selfJID);

	// returns the next character
	char GetChar();

	// while trying to read tokens we may accidentaly
	// read something that belongs to other token
	// ch will be the next result form GetChar()
	void returnChar(char ch);

  private:
	IReadStream* stream; // json source, do not delete this the parser doesnt own that object
	char accumCh;        // the buffer for GetChar()/ReturnChar()

	// used for converting from string to double/float/int/ect.
	std::array<char, 32> numconvert; // If you modify the array size, please do NOT forget to update it the writer too.
	unsigned numconvertidx;
	bool numAppearsFloaty;

	JsonValue* root; // a pointer to root value
	const char* parsingErrorMsg;
};

/////////////////////////////////////////////////////////////////////////
// JsonWriter
// [NOTE][CAUTION] Make SHURE that even the PRETTY WRITER
// DOES NOT add ANY SYMBOLS AFTER THE LAST CLOSING SYMBOL
// FOR THE GIVEN VALUE (] or } for example) !!!
/////////////////////////////////////////////////////////////////////////
class JsonWriter {
  public:
	bool write(IWriteStream* wstream, const JsonValue* const root, const bool prettify = false);
	bool WriteInFile(const char* const filename, const JsonValue* const root, const bool prettify = false);

  private:
	void writeVairable(const JsonValue* const value);

	void write(const char ch, bool forceNoPretty = false);
	void writeString(const char* string,
	                 const bool procStringTokens); // set procStringTokens to true to convert \t\n\r ect to '\' + 'n' ect..

	std::vector<char> encodedData;
	std::array<char, 32> numconvert;
	IWriteStream* stream;

	bool bPretty;
	int prettyIdentation;
};


} // namespace sge
