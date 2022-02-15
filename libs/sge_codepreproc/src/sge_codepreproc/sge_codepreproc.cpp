#include "sge_codepreproc/sge_codepreproc.h"

#include "sge_utils/io/FileStream.h"
#include "sge_utils/text/format.h"
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace sge {

/// UserData is a structures that holds the user state while parsing with mcpp.
struct UserData {
	const char* code = nullptr;
	const char* codeFilename = nullptr;
	const std::string& includeDir;
	std::unordered_map<std::string, std::vector<char>>& includeFiles;
};

std::string preprocessCode(
    const char* code,
    const char* const codeFilename,
    const char* const* macros,
    const int numMacros,
    const std::string& includeDir,
    std::set<std::string>* outIncludedFiles)
{
	if (isStringEmpty(code) || isStringEmpty(codeFilename)) {
		return false;
	}

	// Not that anyone is going to do it but:
	// mcpp uses some global state, so it isn't thread safe.
	// That is why we need that lock.
	static std::mutex mcppSafetyLock;
	const std::lock_guard<std::mutex> lockGuard(mcppSafetyLock);

	std::vector<std::string> args;


	// 1st arg needs to be mcpp.
	args.push_back("mcpp");
	// 2nd arg needs to be the filename being parsed.
	args.push_back(codeFilename);

	// Then we need to add the macro definitions.
	for (int t = 0; t < numMacros; ++t) {
		args.push_back(string_format("-D%s", macros[t]));
	}

	std::vector<const char*> argsCFormat;
	for (const std::string& arg : args) {
		argsCFormat.push_back(arg.c_str());
	}


	// This variable will hold the contents of all included files.
	std::unordered_map<std::string, std::vector<char>> includeFiles;

	UserData udata = {code, codeFilename, includeDir, includeFiles};

	// loadFileFn will get get called by mcpp when it needs to include a file.
	const auto loadFileFn =
	    [](const char* filename, const char** out_contents, size_t* out_contents_size, void* user_data) -> int {
		UserData& udata = *static_cast<UserData*>(user_data);

		if (strcmp(filename, udata.codeFilename) == 0) {
			if (out_contents) {
				*out_contents = udata.code;
			}

			if (out_contents_size) {
				*out_contents_size = strlen(udata.code);
			}
			return 1;
		}

		// Compute the full filename.
		const std::string fileToLoad = udata.includeDir + std::string("/") + filename;

		// Check if the file has already been read.
		auto itrExisting = udata.includeFiles.find(fileToLoad.c_str());
		std::vector<char>* pFileData = nullptr;
		if (itrExisting != udata.includeFiles.end()) {
			pFileData = &itrExisting->second;
		}
		else {
			std::vector<char> data;
			if (sge::FileReadStream::readFile(fileToLoad.c_str(), data) == false) {
				return 0;
			}
			data.emplace_back('\0');

			udata.includeFiles[fileToLoad] = std::move(data);
			pFileData = &udata.includeFiles[fileToLoad];
		}

		// Pass the file contents to mcpp if the file has been found.
		if (pFileData) {
			if (out_contents) {
				*out_contents = pFileData->data();
			}

			if (out_contents_size) {
				*out_contents_size = pFileData->size();
			}
			return 1;
		}

		return 0;
	};

	// These two are redirected by mcpp to some global mcpp values,
	// they are not allocated do not try do delete them.
	char* compilationResult = nullptr;
	char* colpilationErrors = nullptr;

	// Run the preprocessor.
	[[maybe_unused]] int const res = sge_mcpp_preprocess(
	    &compilationResult, &colpilationErrors, argsCFormat.data(), int(argsCFormat.size()), loadFileFn, &udata);
	std::string result = compilationResult ? compilationResult : "";

	if (outIncludedFiles != nullptr) {
		for (auto& itr : includeFiles) {
			outIncludedFiles->insert(itr.first);
		}
	}

	return result;
}

std::string preprocessCodeFromFile(
    const char* const codeFilename,
    const char* const* macros,
    const int numMacros,
    const std::string& includeDir,
    std::set<std::string>* outIncludedFiles)
{
	std::string code;
	FileReadStream::readTextFile(codeFilename, code);

	if (!code.empty()) {
		return preprocessCode(code.c_str(), codeFilename, macros, numMacros, includeDir, outIncludedFiles);
	}

	return std::string();
}

} // namespace sge
