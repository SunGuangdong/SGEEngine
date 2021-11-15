#pragma once

#include "AssetTexture2D.h"
#include "sge_Log/Log.h"
#include "sge_core/ICore.h"
#include "sge_core/dds/dds.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"
#include "sge_utils/utils/optional.h"

#include "stb_image.h"
#include <filesystem>

namespace sge {

inline JsonValue* samplerDesc_toJson(const SamplerDesc& desc, JsonValueBuffer& jvb) {
	const auto addressModeToString = [](TextureAddressMode::Enum adressMode) -> const char* {
		switch (adressMode) {
			case TextureAddressMode::Repeat:
				return "repeat";
			case TextureAddressMode::ClampEdge:
				return "edge";
			case TextureAddressMode::ClampBorder:
				return "border";
			default:
				sgeAssert(false);
				return "";
				break;
		}
	};

	const auto filterToString = [](TextureFilter::Enum filter) -> const char* {
		switch (filter) {
			case sge::TextureFilter::Min_Mag_Mip_Point:
				return "point";
				break;
			case sge::TextureFilter::Min_Mag_Mip_Linear:
				return "linear";
				break;
			case sge::TextureFilter::Anisotropic:
				return "anisotropic";
				break;
			default:
				sgeAssert(false);
				return "";
				break;
		}
	};

	auto jRoot = jvb(JID_MAP);

	jRoot->setMember("addressModeX", jvb(addressModeToString(desc.addressModes[0])));
	jRoot->setMember("addressModeY", jvb(addressModeToString(desc.addressModes[1])));
	jRoot->setMember("addressModeZ", jvb(addressModeToString(desc.addressModes[2])));
	jRoot->setMember("filter", jvb(filterToString(desc.filter)));

	return jRoot;
}

inline Optional<SamplerDesc> samplerDesc_fromJson(const JsonValue& jRoot) {
	auto addressModeFromStringOrThrow = [](const char* str) -> TextureAddressMode::Enum {
		if (str == nullptr || str[0] == '\0') {
			throw std::exception();
		}

		if (strcmp("repeat", str) == 0)
			return TextureAddressMode::Repeat;
		if (strcmp("edge", str) == 0)
			return TextureAddressMode::ClampEdge;
		if (strcmp("border", str) == 0)
			return TextureAddressMode::ClampBorder;

		throw std::exception();
	};

	auto filterFormStringOrThrow = [](const char* str) -> TextureFilter::Enum {
		if (str == nullptr || str[0] == '\0') {
			throw std::exception();
		}

		if (strcmp("point", str) == 0)
			return TextureFilter::Min_Mag_Mip_Point;

		if (strcmp("linear", str) == 0)
			return TextureFilter::Min_Mag_Mip_Linear;

		if (strcmp("anisotropic", str) == 0)
			return TextureFilter::Anisotropic;

		throw std::exception();
	};

	try {
		const char* const addressModeXStr = jRoot.getMemberOrThrow("addressModeX").GetStringOrThrow();
		const char* const addressModeYStr = jRoot.getMemberOrThrow("addressModeY").GetStringOrThrow();
		const char* const addressModeZStr = jRoot.getMemberOrThrow("addressModeZ").GetStringOrThrow();
		const char* const filterStr = jRoot.getMemberOrThrow("filter").GetStringOrThrow();

		SamplerDesc result;

		result.addressModes[0] = addressModeFromStringOrThrow(addressModeXStr);
		result.addressModes[1] = addressModeFromStringOrThrow(addressModeYStr);
		result.addressModes[2] = addressModeFromStringOrThrow(addressModeZStr);
		result.filter = filterFormStringOrThrow(filterStr);

		return result;
	} catch (...) {
		sgeAssert(false);
		return NullOptional();
	}
}

AssetTextureMeta loadAssetTextureMeta2(const std::string& baseAssetPath) {
	// [TEXTURE_ASSET_INFO]
	const std::string infoPath = baseAssetPath + ".info";

	FileReadStream frs;
	if (!frs.open(infoPath.c_str())) {
		// No info file, just use the defaults.
		return AssetTextureMeta();
	}

	// Parse the json inside that file.
	JsonParser jp;
	if (!jp.parse(&frs)) {
		// No info file, just use the defaults.
		sgeAssert(false && "Parsing the texture info file, that contains additional settings for a texture asset cannot be parsed!");
		return AssetTextureMeta();
	}

	try {
		AssetTextureMeta result;
		auto jRoot = jp.getRoot();

		const int version = jRoot->getMemberOrThrow("version").getNumberAsOrThrow<int>();
		if (version == 1) {
			// Default version, nothing special in it.
			auto jSampler = jRoot->getMemberOrThrow("sampler", JID_MAP);
			Optional<SamplerDesc> samplerDesc = samplerDesc_fromJson(jSampler);
			if (samplerDesc) {
				result.assetSamplerDesc = *samplerDesc;
			}

			const JsonValue* jIsSemiTransparent = jRoot->getMember("isSemiTransparent");
			result.isSemiTransparent = jIsSemiTransparent && jIsSemiTransparent->jid == JID_TRUE;

			const JsonValue* jShoudGenerateMips = jRoot->getMember("shouldGenerateMips");
			result.shouldGenerateMips = jShoudGenerateMips && jShoudGenerateMips->jid == JID_TRUE;
		}

		return result;
	} catch (...) {
	}

	sgeAssert(false && "Parsing the texture info file, that contains additional settings for a texture asset cannot be parsed!");
	return AssetTextureMeta();
}




AssetTexture2d::DDSLoadCode AssetTexture2d::loadDDS(const char* const rawPath) {
	std::string const ddsPath = (extractFileExtension(rawPath) == "dds") ? rawPath : std::string(rawPath) + ".dds";

	std::vector<char> ddsDataRaw;
	if (FileReadStream::readFile(ddsPath.c_str(), ddsDataRaw) == false) {
		return ddsLoadCode_fileDoesntExist;
	}

	// Parse the file and generate the texture creation strctures.
	DDSLoader loader;
	TextureDesc desc;
	std::vector<TextureData> initalData;

	if (loader.load(ddsDataRaw.data(), ddsDataRaw.size(), desc, initalData) == false) {
		return ddsLoadCode_importOrCreationFailed;
	}

	m_texture = getCore()->getDevice()->requestResource<Texture>();

	// Create the texture.
	m_textureMeta = loadAssetTextureMeta2(rawPath);
	bool const createSucceeded = m_texture->create(desc, &initalData[0], m_textureMeta.assetSamplerDesc);

	if (createSucceeded == false) {
		m_texture.Release();
		return ddsLoadCode_importOrCreationFailed;
	}

	return ddsLoadCode_fine;
}

bool AssetTexture2d::loadAssetFromFile(const char* path) {
	m_status = AssetStatus_LoadFailed;

#if !defined(__EMSCRIPTEN__)
	DDSLoadCode const ddsLoadStatus = loadDDS(path);

	if (ddsLoadStatus == ddsLoadCode_fine) {
		return true;
	} else if (ddsLoadStatus == ddsLoadCode_importOrCreationFailed) {
		SGE_DEBUG_WAR("Failed to load the DDS equivalent to '%s'!\n", path);
		return false;
	}
#endif

	// Now check for the actual asset that is requested.
	FileReadStream frs(path);
	if (!frs.isOpened()) {
		SGE_DEBUG_ERR("Unable to find texture2d asset: '%s'!\n", path);
		return false;
	}

	int width, height, components;
	const unsigned char* textureData = stbi_load(path, &width, &height, &components, 4);

	TextureDesc textureDesc;

	m_textureMeta = loadAssetTextureMeta2(path);

	textureDesc.textureType = UniformType::Texture2D;
	textureDesc.format = TextureFormat::R8G8B8A8_UNORM;
	textureDesc.usage = TextureUsage::ImmutableResource;
	textureDesc.texture2D.arraySize = 1;
	textureDesc.texture2D.numMips = 1;
	textureDesc.texture2D.numSamples = 1;
	textureDesc.texture2D.sampleQuality = 0;
	textureDesc.texture2D.width = width;
	textureDesc.texture2D.height = height;
	textureDesc.generateMips = m_textureMeta.shouldGenerateMips;

	TextureData textureDataDesc;
	textureDataDesc.data = textureData;
	textureDataDesc.rowByteSize = size_t(width) * 4;

	m_texture = getCore()->getDevice()->requestResource<Texture>();
	m_texture->create(textureDesc, &textureDataDesc, m_textureMeta.assetSamplerDesc);

	if (textureData != nullptr) {
		stbi_image_free((void*)textureData);
		textureData = nullptr;
	}

	if (m_texture.IsResourceValid()) {
		m_status = AssetStatus_Loaded;
	}

	return m_status == AssetStatus_Loaded;
}

bool AssetTexture2d::saveTextureSettingsToInfoFile() const {
	// [TEXTURE_ASSET_INFO]
	const std::string infoPath = getPath() + ".info";

	JsonValueBuffer jvb;
	auto jRoot = jvb(JID_MAP);
	jRoot->setMember("version", jvb(1));
	jRoot->setMember("sampler", samplerDesc_toJson(m_textureMeta.assetSamplerDesc, jvb));
	jRoot->setMember("isSemiTransparent", jvb(m_textureMeta.isSemiTransparent));
	jRoot->setMember("shouldGenerateMips", jvb(m_textureMeta.shouldGenerateMips));

	JsonWriter jsonWriter;
	bool success = jsonWriter.WriteInFile(infoPath.c_str(), jRoot, true);
	return success;
}

} // namespace sge
