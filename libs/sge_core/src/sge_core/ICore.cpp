#include "ICore.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/DebugDraw.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/input.h"
#include "sge_core/materials/DefaultPBRMtl/DefaultPBRMtl.h"
#include "sge_core/materials/DefaultPBRMtl/DefaultPBRMtlGeomDrawer.h"
#include "sge_core/materials/MaterialFamilyList.h"
#include "sge_core/materials/SimpleTriplanar/SimpleTriplanarMtl.h"
#include "sge_core/materials/SimpleTriplanar/SimpleTriplanarMtlGeomDrawer.h"
#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/sge_utils.h"

namespace sge {

struct Core : public ICore {
	typedef void (*CallBack)();

	Core() = default;

	void setup(SGEDevice* const sgedev, AudioDevice* const sgeAudioDevice) final;

	AssetLibrary* getAssetLib() final
	{
		return m_assetLibrary.get();
	}

	MaterialFamilyLibrary* getMaterialLib() final
	{
		return m_materialFamilyLib.get();
	}

	QuickDraw& getQuickDraw() final
	{
		return m_quickDraw;
	}

	DebugDraw& getDebugDraw() final
	{
		return m_debugDraw;
	}

	SGEDevice* getDevice() final
	{
		return m_sgedev;
	}

	AudioDevice* getAudioDevice() final
	{
		return m_audioDevice;
	}

	GraphicsResources& getGraphicsResources() final
	{
		return m_graphicsResources;
	}

	void setInputState(const InputState& is) final
	{
		m_inputState = is;
	}

	const InputState& getInputState() const final
	{
		return m_inputState;
	}

	const FrameStatistics& getLastFrameStatistics() const final
	{
		return lastFrameStatistics;
	}

	void setLastFrameStatistics(const FrameStatistics& stats) final
	{
		lastFrameStatistics = stats;
	}

  public:
	SGEDevice* m_sgedev = nullptr; // The sge device attached to the main window.

	std::unique_ptr<AssetLibrary> m_assetLibrary;
	std::unique_ptr<MaterialFamilyLibrary> m_materialFamilyLib;
	GraphicsResources m_graphicsResources; // A set of commonly used graphics resources.

	QuickDraw m_quickDraw;
	DebugDraw m_debugDraw;

	InputState m_inputState;

	FrameStatistics lastFrameStatistics;
	std::map<std::string, std::map<std::string, CallBack>> m_menuItems;

	AudioDevice* m_audioDevice = nullptr;
};

void Core::setup(SGEDevice* const sgedev, AudioDevice* const sgeAudioDevice)
{
	sgeAssert(sgedev);
	sgeAssert(sgeAudioDevice);
	m_sgedev = sgedev;

	m_audioDevice = sgeAudioDevice;

	m_assetLibrary = std::make_unique<AssetLibrary>();
	m_materialFamilyLib = std::make_unique<MaterialFamilyLibrary>();

	// Uniform string indices.
	m_graphicsResources.projViewWorld_strIdx = sgedev->getStringIndex("projViewWorld");
	m_graphicsResources.color_strIdx = sgedev->getStringIndex("color");

	// Rasterizer States.
	m_graphicsResources.RS_default = sgedev->requestRasterizerState(RasterDesc());

	{
		RasterDesc desc;
		desc.fillMode = FillMode::Wireframe;
		m_graphicsResources.RS_defaultWireframe = sgedev->requestRasterizerState(desc);
	}

	{
		RasterDesc desc;
		desc.cullMode = CullMode::Back;
		desc.backFaceCCW = false;
		m_graphicsResources.RS_cullInverse = sgedev->requestRasterizerState(desc);
	}

	{
		RasterDesc desc;
		desc.cullMode = CullMode::None;
		m_graphicsResources.RS_noCulling = sgedev->requestRasterizerState(desc);
	}

	{
		RasterDesc desc;
		desc.cullMode = CullMode::Back;
		desc.backFaceCCW = true;
		m_graphicsResources.RS_defaultBackfaceCCW = sgedev->requestRasterizerState(desc);
	}

	{
		RasterDesc desc;
		desc.fillMode = FillMode::Wireframe;
		desc.cullMode = CullMode::Back;
		desc.backFaceCCW = true;
		m_graphicsResources.RS_wireframeBackfaceCCW = sgedev->requestRasterizerState(desc);
	}

	// DepthStencil States.
	m_graphicsResources.DSS_default = sgedev->requestDepthStencilState(DepthStencilDesc());

	{
		DepthStencilDesc desc;
		desc.comparisonFunc = DepthComparisonFunc::LessEqual;
		m_graphicsResources.DSS_default_lessEqual = sgedev->requestDepthStencilState(desc);
	}

	{
		DepthStencilDesc desc;
		desc.comparisonFunc = DepthComparisonFunc::LessEqual;
		desc.depthTestEnabled = true;
		desc.depthWriteEnabled = false;
		m_graphicsResources.DSS_default_lessEqual_noWrite = sgedev->requestDepthStencilState(desc);
	}

	{
		DepthStencilDesc desc;
		// desc.comparisonFunc = DepthComparisonFunc::LessEqual;
		desc.depthTestEnabled = true;
		desc.comparisonFunc = DepthComparisonFunc::Always;
		m_graphicsResources.DSS_always_noTest = sgedev->requestDepthStencilState(desc);
	}

	{
		DepthStencilDesc desc;
		desc.depthTestEnabled = false;
		desc.depthWriteEnabled = false;
		desc.comparisonFunc = DepthComparisonFunc::Always;
		m_graphicsResources.DSS_noWrite_noTest = sgedev->requestDepthStencilState(desc);
	}

	// Blend Staets.
	{
		m_graphicsResources.BS_backToFrontAlpha = sgedev->requestBlendState(BlendStateDesc::GetDefaultBackToFrontAlpha());
		m_graphicsResources.BS_addativeColor = sgedev->requestBlendState(BlendStateDesc::getColorAdditiveBlending());
	}

	m_quickDraw.initialize(sgedev->getContext());
	m_debugDraw.initialze(sgedev);

	// Register the DefaultPBR material family.
	{
		// clang-format off
		MaterialFamilyDesc pbrFd = {
			1001,
			"DefaultPBR", 
			[]() -> IGeometryDrawer* { return new DefaultPBRMtlGeomDrawer(); },
		    []() -> std::unique_ptr<IMaterial> { return std::make_unique<DefaultPBRMtl>(); }
		};

		getMaterialLib()->registerFamily(pbrFd);
		// clang-format on

		// clang-format off
		MaterialFamilyDesc simpleTriplanarFd = {
			1002,
			"SimpleTriplanar", 
			[]() -> IGeometryDrawer* { return new SimpleTriplanarMtlGeomDrawer(); },
		    []() -> std::unique_ptr<IMaterial> { return std::make_unique<SimpleTriplanarMtl>(); }
		};

		getMaterialLib()->registerFamily(simpleTriplanarFd);
		// clang-format on
	}

	return;
}

Core g_moduleLocalCore;
ICore* g_pWorkingCore = &g_moduleLocalCore;

ICore* getCore()
{
	return g_pWorkingCore;
}

void setCore(ICore* core)
{
	sgeAssert(core);
	g_pWorkingCore = core;
}

} // namespace sge
