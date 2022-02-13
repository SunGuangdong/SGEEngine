#pragma once

#include <string>

#include "sge_utils/sge_utils.h"

namespace sge {

struct ContentParsingSettings {
	ContentParsingSettings() = default;

	ContentParsingSettings(const std::string& absoluteInputDir, const std::string& absoluteOutputDir)
	    : absoluteInputDir(absoluteInputDir)
	    , absoluteOutputDir(absoluteOutputDir)
	{
	}

	std::string absoluteInputDir;
	std::string absoluteOutputDir;
};

struct IAssetRelocationPolicy {
	IAssetRelocationPolicy() = default;
	virtual ~IAssetRelocationPolicy() = default;

	// @arg askerDir - Where is the file that contains a reference to the "path" assets
	// @arg path - the path that we are trying to resolve
	virtual std::string whatWillBeTheAssetNameOf(const std::string& askerDir, const char* path) = 0;
};

struct NoneAssetRelocationPolicy final : public IAssetRelocationPolicy {
	std::string whatWillBeTheAssetNameOf(const std::string& UNUSED(askerDir), const char* path) final { return path; }
};

struct RelativeAssetRelocationPolicy final : public IAssetRelocationPolicy {
	RelativeAssetRelocationPolicy(const ContentParsingSettings& contentParsingSets)
	    : m_contentParsingSets(contentParsingSets)
	{
	}

	std::string whatWillBeTheAssetNameOf(const std::string& askerDir, const char* path) final;

	const ContentParsingSettings& m_contentParsingSets;
};

#if 0
struct SeparateTypeAssetRelocationPolicy final : public IAssetRelocationPolicy 
{
	SeparateTypeAssetRelocationPolicy(const ContentParsingSettings& contentParsingSets) 
		: m_contentParsingSets(contentParsingSets)
	{}

	std::string whatWillBeTheAssetNameOf(const std::string& askerDir, const char* path) final;

	const ContentParsingSettings& m_contentParsingSets;
};
#endif

} // namespace sge
