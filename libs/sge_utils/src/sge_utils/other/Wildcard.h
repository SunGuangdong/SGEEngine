#pragma once

namespace sge {
// https://www.codeproject.com/Articles/188256/A-Simple-Wildcard-Matching-Function
bool WildcardMatch(const char* pszString, const char* pszMatch);
} // namespace sge