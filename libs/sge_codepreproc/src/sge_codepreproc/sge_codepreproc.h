#pragma once

#include "sge_mcpp.h"
#include <set>
#include <string>

namespace sge {


/// Preprocesses the specified code using a C-style preprocessor.
/// @param [in] code the code to be compiled
/// @param [in] codeFilename is the name of the file that contains the code (could be anything really that fits the command line).
/// @param [in] macros an arrays of marcos to be specified (may be nullptr).
/// @param [in] numMacros the array size of @macros.
/// @param [in] includeDir the include directory to be used when resolving #include macros (TODO: make this accept more that one directory).
/// @param [out] outIncludedFiles optional list of all included files during the parsing, excluding @codeFilename if not included
/// explicitly.
std::string preprocessCode(const char* code,
                           const char* const codeFilename,
                           const char* const* macros,
                           const int numMacros,
                           const std::string& includeDir,
                           std::set<std::string>* outIncludedFiles);


/// Preprocesses the specified code using a C-style preprocessor.
/// see the comment of @preprocessCode
std::string preprocessCodeFromFile(const char* const codeFilename,
                                   const char* const* macros,
                                   const int numMacros,
                                   const std::string& includeDir,
                                   std::set<std::string>* outIncludedFiles);


} // namespace sge
