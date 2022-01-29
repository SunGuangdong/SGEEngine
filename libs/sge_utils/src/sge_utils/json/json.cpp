#include "sge_utils/json/json.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/io/IStream.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/text/format.h"

#include <charconv>
#include <cstring>

namespace sge {

/////////////////////////////////////////////////////////////////////////
// Common Json parsing helpers
/////////////////////////////////////////////////////////////////////////
class JsonParseError {
  public:
	JsonParseError(const char* const error = nullptr)
	    : error(error)
	{
	}

	const char* const error;
};


float cstr2float(const char* cstr)
{
#if 0
	// This is useful when we suspect that the file might be broken.
	 double value;
	 auto res = std::from_chars(cstr, cstr + strlen(cstr), value);
	 if (res.ec == std::errc()) {
	 	return float(value);
	 }
	 throw JsonParseError("cannot convert string to float");
#endif
	return float(atof(cstr));
}

int cstr2int(const char* cstr)
{
#if 0
	int value;
	auto res = std::from_chars(cstr, cstr + strlen(cstr), value);
	if (res.ec == std::errc()) {
		return value;
	}
	throw JsonParseError("cannot convert string to int");
#endif

	return atoi(cstr);
}

/////////////////////////////////////////////////////////////////////////
// struct JsonValueBuffer
/////////////////////////////////////////////////////////////////////////
JsonValue* JsonValueBuffer::GetNewValue()
{
	const bool shouldAllocateChunk = (m_valuesBuffer.empty() || m_pointer == ChunkSize);

	if (shouldAllocateChunk) {
		m_pointer = 0;
		m_valuesBuffer.push_back(new JsonValue[ChunkSize]);
	}

	return &((m_valuesBuffer.back())[m_pointer++]);
}

void JsonValueBuffer::clearAllValues()
{
	for (size_t t = 0; t < m_valuesBuffer.size(); ++t) {
		delete[] m_valuesBuffer[t];
	}

	m_valuesBuffer.clear();
}

JsonValue* JsonValueBuffer::operator()(const std::string& str)
{
	JsonValue* retval = GetNewValue();
	retval->setString(str.c_str());
	return retval;
}

JsonValue* JsonValueBuffer::operator()(const char* const str)
{
	JsonValue* retval = GetNewValue();
	retval->setString(str);
	return retval;
}

JsonValue* JsonValueBuffer::operator()(const float s)
{
	JsonValue* retval = GetNewValue();
	retval->setFloat(s);
	return retval;
}

JsonValue* JsonValueBuffer::operator()(const int s)
{
	JsonValue* retval = GetNewValue();
	retval->setInt32(s);
	return retval;
}

JsonValue* JsonValueBuffer::operator()(const unsigned s)
{
	JsonValue* retval = GetNewValue();
	retval->setUInt32(s);
	return retval;
}

JsonValue* JsonValueBuffer::operator()(const bool s)
{
	JsonValue* retval = GetNewValue();
	retval->setBool(s);
	return retval;
}

JsonValue* JsonValueBuffer::operator()(const JID jid)
{
	JsonValue* retval = GetNewValue();
	retval->jid = jid;
	return retval;
};

JsonValue* JsonValueBuffer::operator()(const float* const arr, int size)
{
	if (!arr)
		size = 0;
	JsonValue* retval = (*this)(JID_ARRAY_BEGIN);
	for (int t = 0; t < size; ++t) {
		retval->arrPush((*this)(arr[t]));
	}
	return retval;
}

JsonValue* JsonValueBuffer::operator()(const int* const arr, int size)
{
	if (!arr)
		size = 0;
	JsonValue* retval = (*this)(JID_ARRAY_BEGIN);
	for (int t = 0; t < size; ++t) {
		retval->arrPush((*this)(arr[t]));
	}
	return retval;
}

JsonValue* JsonValueBuffer::operator()(const std::vector<float>& vec)
{
	return (*this)(vec.data(), (int)vec.size());
}

JsonValue* JsonValueBuffer::operator()(const std::vector<int>& vec)
{
	return (*this)(vec.data(), (int)vec.size());
}

JsonValue* JsonValueBuffer::operator()(const std::vector<std::string>& vec)
{
	JsonValue* retval = (*this)(JID_ARRAY_BEGIN);
	for (const auto& v : vec) {
		retval->arrPush((*this)(v));
	}

	return retval;
}

/////////////////////////////////////////////////////////////////////////
// struct JsonValue
/////////////////////////////////////////////////////////////////////////
void JsonValue::setFloat(const float v)
{
	*this = JsonValue();
	jid = JID_REAL32;
	value_float = v;
}

void JsonValue::setInt32(const sint32 v)
{
	*this = JsonValue();
	jid = JID_INT32;
	value_int32 = v;
}

void JsonValue::setUInt32(const uint32 v)
{
	*this = JsonValue();
	jid = JID_INT32;
	value_uint32 = v;
}

void JsonValue::setString(const char* const str)
{
	*this = JsonValue();
	jid = JID_STRING;
	size_t len = strlen(str) + 1;
	uniformData.resize(len);
	memcpy(uniformData.data(), str, len);
}

JsonValue* JsonValue::setMember(const char* const name, JsonValue* value)
{
	if (jid != JID_MAP_BEGIN) {
		sgeAssert(false);
		return nullptr; //[TODO] should this return value here?
	}

	if (value == nullptr)
		return nullptr;

	// minor curcular references check
	sgeAssert(value != this);

	for (size_t t = 0; t < members.size(); ++t) {
		if (strcmp((char*)members[t].first.data(), name) == 0) {
			members[t].second = value;
			return value;
		}
	}

	std::pair<std::vector<unsigned char>, JsonValue*> elem;
	elem.first.resize(strlen(name) + 1);
	sge_strcpy((char*)elem.first.data(), elem.first.size(), name);
	elem.second = value;

	members.push_back(std::move(elem));
	return value;
}

const JsonValue* JsonValue::getMember(const char* const name) const
{
	sgeAssert(jid == JID_MAP_BEGIN);
	for (size_t t = 0; t < members.size(); ++t) {
		if (strcmp((char*)members[t].first.data(), name) == 0) {
			return members[t].second;
		}
	}
	return nullptr;
}

const JsonValue& JsonValue::getMemberOrThrow(const char* const name, JID explectedType) const
{
	sgeAssert(jid == JID_MAP_BEGIN);
	for (size_t t = 0; t < members.size(); ++t) {
		if (strcmp((char*)members[t].first.data(), name) == 0) {
			if (members[t].second != nullptr) {
				if (explectedType != JID_NULL && members[t].second->jid != explectedType) {
					throw JsonExceptAccess();
				}

				return *members[t].second;
			}
		}
	}
	throw JsonExceptAccess();
}

JsonValue* JsonValue::arrPush(JsonValue* value)
{
	if (jid != JID_ARRAY_BEGIN) {
		sgeAssert(false);
		return nullptr; // [TODO] Should we return value here?
	}

	// minor curcular references check
	sgeAssert(value != this);

	arrayValues.push_back(value);
	return value;
}

JsonValue* JsonValue::Clone(const JsonValue& root, JsonValueBuffer& jvb)
{
	JsonValue* const result = jvb(root.jid);
	sgeAssert(result);

	// Just duplicate the numeric value and the uniform data.
	result->value_uint64 = root.value_uint64;
	result->uniformData = root.uniformData;

	if (result->jid == JID_ARRAY) {
		result->arrayValues.reserve(root.arrayValues.size());

		for (const JsonValue* const val : root.arrayValues) {
			result->arrPush(JsonValue::Clone(*val, jvb));
		}
	}
	else if (result->jid == JID_MAP) {
		for (const auto& pair : root.members) {
			result->setMember((const char*)pair.first.data(), JsonValue::Clone(*pair.second, jvb));
		}
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////
// class JsonParser
/////////////////////////////////////////////////////////////////////////
void JsonParser::Clear()
{
	root = nullptr;
	accumCh = 0;
	stream = nullptr;
	parsingErrorMsg = nullptr;
	clearAllValues();
}

bool JsonParser::parse(IReadStream* instream)
{
	// reset the parser to inital state
	Clear();

	// validate the input
	if (!instream)
		return false;

	stream = instream;

	try {
		root = parseValue(getNextJID());
	}
	catch ([[maybe_unused]] const JsonParseError& except) {
		return false;
	}

	return true;
}

JID JsonParser::getNextJID()
{
	skipSpacesAhead();

	JID retval = JID_NULL;
	char ch = GetChar();

	if (ch == '[')
		retval = JID_ARRAY_BEGIN;
	else if (ch == ']')
		retval = JID_ARRAY_END;
	else if (ch == '{')
		retval = JID_MAP_BEGIN;
	else if (ch == '}')
		retval = JID_MAP_END;
	else if (ch == '"')
		retval = JID_STRING;
	else if (ch == '-' || ch == '+' || sge_isdigit(ch)) {
		retval = JID_SOME_NUMBER;
		returnChar(ch); // return this for later parsing
	}
	else if (ch == 't') {
		retval = JID_TRUE;
	}
	else if (ch == 'f') {
		retval = JID_FALSE;
	}
	else {
		throw JsonParseError("Unknown json element found!");
	}

	return retval;
}

bool JsonParser::skipSeparatorAhead(char separator)
{
	while (true) {
		char ch = GetChar();
		if (!sge_isspace(ch)) {
			if (ch == separator) {
				return true;
			}
			else {
				returnChar(ch);
				return false;
			}
		}
	}
}

void JsonParser::skipSpacesAhead()
{
	while (true) {
		char ch = GetChar();
		if (!sge_isspace(ch)) {
			returnChar(ch);
			break;
		}
	}
}

void JsonParser::readString(std::vector<unsigned char>& str, const bool procStringTokens)
{
	str.reserve(32); // good enough for the most common cases
	char ch;
	while ((ch = GetChar()) != '"') {
		if (procStringTokens == true && ch == '\\') {
			const char nextCh = GetChar();

			if (nextCh == 't')
				ch = '\t';
			else if (nextCh == 'n')
				ch = '\n';
			else if (nextCh == 'r')
				ch = '\r';
			else if (nextCh == 'b')
				ch = '\b';
			else if (nextCh == '\\')
				ch = '\\';
			else if (nextCh == '\f')
				ch = '\f';
			else if (nextCh == '"')
				ch = '\"';
			else if (nextCh == 'u') {
				sgeAssert(false);
				throw JsonParseError("'\\u' tokens aren't supported!");
			}
			else {
				sgeAssert(false);
				throw JsonParseError("'\\?' unknown token after slash!");
			}
		}

		str.push_back(ch);
	}

	// add the null terimanor
	str.push_back(0);
}

void JsonParser::readNumber()
{
	numAppearsFloaty = false;
	numconvertidx = 0;
	while (true) {
		const char ch = GetChar();
		if (sge_isdigit(ch) || ch == '.' || ch == '-' || ch == '+' || ch == 'e' || ch == 'E') {
			numAppearsFloaty = numAppearsFloaty || ch == '.' || ch == 'e' || ch == 'E';
			numconvert[numconvertidx++] = ch;
		}
		else {
			returnChar(ch);
			break;
		}
	}

	sgeAssert(numconvertidx < numconvert.size());
	numconvert[numconvertidx] = 0;
}

JsonValue* JsonParser::parseValue(const JID selfJID)
{
	// NUMBERS
	if (selfJID == JID_SOME_NUMBER) {
		readNumber();

		JsonValue* result = GetNewValue();

		//[TODO] Add options so the user can
		// specify how he wants to read the numbers
		if (numAppearsFloaty) {
			const float vf = cstr2float(numconvert.data());
			result->setFloat(vf);
		}
		else {
			const int vi = atoi(numconvert.data());
			result->setInt32(vi);
		}

		return result;
	}
	// BOOLEANS
	else if (selfJID == JID_TRUE) {
		// read the rest of the word True
		if (GetChar() != 'r')
			throw JsonParseError("Unknow identifier!");
		if (GetChar() != 'u')
			throw JsonParseError("Unknow identifier!");
		if (GetChar() != 'e')
			throw JsonParseError("Unknow identifier!");

		JsonValue* result = GetNewValue();
		result->setBool(true);

		return result;
	}
	else if (selfJID == JID_FALSE) {
		// read the rest of the word False
		if (GetChar() != 'a')
			throw JsonParseError("Unknow identifier!");
		if (GetChar() != 'l')
			throw JsonParseError("Unknow identifier!");
		if (GetChar() != 's')
			throw JsonParseError("Unknow identifier!");
		if (GetChar() != 'e')
			throw JsonParseError("Unknow identifier!");

		JsonValue* result = GetNewValue();
		result->setBool(false);

		return result;
	}
	// STRINGS
	else if (selfJID == JID_STRING) {
		JsonValue* result = GetNewValue();
		result->jid = JID_STRING;
		readString(result->uniformData, true);
		return result;
	}
	// ARRAYS
	else if (selfJID == JID_ARRAY_BEGIN) {
		JsonValue* result = GetNewValue();
		result->jid = JID_ARRAY_BEGIN;

		//[NOTE] That helps a bit but leave it commented
		// result->arrayValues.reserve(8);
		while (true) {
			JID jid = getNextJID();

			// for empty arrays
			if (jid == JID_ARRAY_END) {
				break;
			}

			// read and add the member
			JsonValue* member = parseValue(jid);
			result->arrayValues.emplace_back(member);
#if 1
			// if there isn't comma then
			// this is the end of the array
			if (!skipSeparatorAhead(',')) {
				if (getNextJID() != JID_ARRAY_END) {
					throw JsonParseError("Missing ',' while reading array value!");
				}
				break;
			}
#else
			// the code above doesnt really give us useful information.
			// the error will be caught by GetNextJID()
			skipSeparatorAhead(',');
#endif
		}

		return result;
	}
	// MAPS
	else if (selfJID == JID_MAP_BEGIN) {
		JsonValue* result = GetNewValue();
		result->jid = JID_MAP_BEGIN;

		//[NOTE] That helps a bit but leave it commented
		// result->members.reserve(8);
		while (true) {
			JID jid = getNextJID();

			// check if this is an empty map
			if (jid == JID_MAP_END) {
				break;
			}

			// this must be the member identifier(name)
			if (jid != JID_STRING) {
				throw JsonParseError("Expected indentifier!");
			}

			// read the variable name
			std::vector<unsigned char> idnetifier;
			readString(idnetifier, false);

			// read the :
#if 1
			if (!skipSeparatorAhead(':')) {
				throw JsonParseError("Missing ':' while reading map value!");
			}
#else
			skipSeparatorAhead(':');
#endif

			// get the member value
			JsonValue* member = parseValue(getNextJID());
			result->members.emplace_back(std::pair<std::vector<unsigned char>, JsonValue*>(std::move(idnetifier), std::move(member)));

#if 1
			// ckeck for comma ahead.
			// if there isn't such check
			// then the next JID must be JID_MAP_END
			if (!skipSeparatorAhead(',')) {
				if (getNextJID() != JID_MAP_END) {
					throw JsonParseError("Missing ',' while reading map value!");
				}
				break;
			}
#else
			// the code above doesnt really give us useful information.
			// the error will be caught by GetNextJID()
			skipSeparatorAhead(',');
#endif
		}

		return result;
	}
	// UNKNOWN
	else {
		throw JsonParseError("Trying to parse unknown json element!");
	}
}

char JsonParser::GetChar()
{
	if (accumCh != 0) {
		char retval = accumCh;
		accumCh = 0;
		return retval;
	}

	char ch;
	if (stream->read(&ch, 1) == 0) {
		throw JsonParseError("Unexpected end of stream!");
	}

	return ch;
}

void JsonParser::returnChar(char ch)
{
#ifdef SGE_USE_DEBUG
	if (accumCh != 0) {
		throw JsonParseError("Json Parser Implementation error! ReturnChar() called multiple times!");
	}
#endif

	accumCh = ch;
}

/////////////////////////////////////////////////////////////////////////
// JsonWriter
/////////////////////////////////////////////////////////////////////////
bool JsonWriter::write(IWriteStream* wstream, const JsonValue* const root, const bool prettify)
{
	if (!wstream || !root) {
		return false;
	}

	stream = wstream;

	bPretty = prettify;
	prettyIdentation = 0;

	try {
		writeVairable(root);
	}
	catch (const JsonParseError& except) {
		[[maybe_unused]] const char* const err = except.error;
		sgeAssert(false);
		return false;
	}

	return true;
}

bool JsonWriter::WriteInFile(const char* const filename, const JsonValue* const root, const bool prettify)
{
	FileWriteStream fws;
	if (!fws.open(filename)) {
		return false;
	}

	return write(&fws, root, prettify);
}

// [NOTE][CAUTION] Make SHURE that event the PRETTY WRITER
// DOES NOT add ANY SYMBOLS AFTER THE LAST CLOSING SYMBOL
// FOR THE VALUE (] or } for example) !!!
void JsonWriter::write(const char ch, bool forceNoPretty)
{
	if (bPretty && !forceNoPretty) {
		const bool isOpenBlock = (ch == '{' || ch == '[');
		const bool isCloseBlock = (ch == '}' || ch == ']');

		if (isOpenBlock)
			prettyIdentation++;
		if (isCloseBlock)
			prettyIdentation--;

		if (isCloseBlock) {
			write('\n');
			for (int t = 0; t < prettyIdentation; ++t)
				write('\t');
		}

		stream->write(&ch, 1);

		if (isOpenBlock || ch == ',') {
			write('\n');
			for (int t = 0; t < prettyIdentation; ++t)
				write('\t');
		}
	}
	else {
		stream->write(&ch, 1);
	}
}

void JsonWriter::writeString(const char* str, const bool procStringTokens)
{
	if (procStringTokens == false) {
		stream->write(str, strlen(str));
		return;
	}

	while (*str != '\0') {
		if (*str == '\'')
			write('\\', true), write('\'', true);
		else if (*str == '\"')
			write('\\', true), write('"', true);
		else if (*str == '\?')
			write('\\', true), write('?', true);
		else if (*str == '\\')
			write('\\', true), write('\\', true);
		else if (*str == '\a')
			write('\\', true), write('a', true);
		else if (*str == '\b')
			write('\\', true), write('b', true);
		else if (*str == '\f')
			write('\\', true), write('f', true);
		else if (*str == '\n')
			write('\\', true), write('n', true);
		else if (*str == '\r')
			write('\\', true), write('r', true);
		else if (*str == '\t')
			write('\\', true), write('t', true);
		else if (*str == '\v')
			write('\\', true), write('v', true);
		else
			write(*str, true);

		str += 1;
	}
}

void JsonWriter::writeVairable(const JsonValue* const value)
{
	const JID jid = value->jid;

	if (jid == JID_INT8) {
		sge_snprintf(numconvert.data(), numconvert.size(), "%d", (int)(value->value_int8));
		writeString(numconvert.data(), false);
	}
	else if (jid == JID_INT16) {
		sge_snprintf(numconvert.data(), numconvert.size(), "%d", value->value_int16);
		writeString(numconvert.data(), false);
	}
	else if (jid == JID_INT32) {
		sge_snprintf(numconvert.data(), numconvert.size(), "%d", value->value_int32);
		writeString(numconvert.data(), false);
	}
	else if (jid == JID_INT64) {
		sge_snprintf(numconvert.data(), numconvert.size(), "%ll", value->value_int64);
		writeString(numconvert.data(), false);
	}
	else if (jid == JID_REAL16) {
		sgeAssert(false);
	}
	else if (jid == JID_REAL32) {
		sge_snprintf(numconvert.data(), numconvert.size(), "%f", value->value_float);
		writeString(numconvert.data(), false);
	}
	else if (jid == JID_REAL64) {
		sge_snprintf(numconvert.data(), numconvert.size(), "%f", value->value_double);
		writeString(numconvert.data(), false);
	}
	else if (jid == JID_UINT8) {
		sge_snprintf(numconvert.data(), numconvert.size(), "%d", (int)(value->value_uint8));
		writeString(numconvert.data(), false);
	}
	else if (jid == JID_UINT16) {
		sge_snprintf(numconvert.data(), numconvert.size(), "%d", (int)(value->value_uint16));
		writeString(numconvert.data(), false);
	}
	else if (jid == JID_STRING) {
		write('"');
		writeString((char*)value->uniformData.data(), true);
		write('"');
	}
	else if (jid == JID_FALSE) {
		writeString("false", false);
	}
	else if (jid == JID_TRUE) {
		writeString("true", false);
	}
	else if (jid == JID_ARRAY_BEGIN) {
		write('[');
		for (size_t t = 0; t < value->arrayValues.size(); ++t) {
			writeVairable(value->arrayValues[t]);

			if (t != value->arrayValues.size() - 1) {
				write(',');
			}
		}
		write(']');
	}
	else if (jid == JID_MAP_BEGIN) {
		write('{');
		for (size_t t = 0; t < value->members.size(); ++t) {
			// the variable name
			write('"');
			writeString((char*)value->members[t].first.data(), false);
			write('"');
			write(':');

			// the value
			writeVairable(value->members[t].second);

			if (t + 1 != value->members.size()) {
				write(',');
			}
		}
		write('}');
	}
	else {
		sgeAssert(false);
		throw JsonParseError("Trying to write variable with unsupported jid!");
	}

	return;
}

} // namespace sge
