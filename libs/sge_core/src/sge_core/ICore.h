#pragma once

#include "sge_audio/AudioDevice.h"
#include "sge_log/Log.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/mat4f.h"
#include "sge_utils/sge_utils.h"
#include "sgecore_api.h"
#include <string>

namespace sge {
struct AssetLibrary;
struct MaterialFamilyLibrary;
struct QuickDraw;
struct DebugDraw;
struct BasicModelDraw;
struct SolidWireframeModelDraw;
struct InputState;

struct SGE_CORE_API ICore {
	// A group of commonly used by the editor graphics resources and render states.
	struct GraphicsResources {
		// Unform string indices.
		unsigned projViewWorld_strIdx = 0;
		unsigned color_strIdx = 0;

		GpuHandle<RasterizerState> RS_default;
		GpuHandle<RasterizerState> RS_defaultWireframe;
		GpuHandle<RasterizerState> RS_cullInverse;
		GpuHandle<RasterizerState> RS_noCulling;
		GpuHandle<RasterizerState> RS_defaultBackfaceCCW;
		GpuHandle<RasterizerState> RS_wireframeBackfaceCCW;

		GpuHandle<DepthStencilState> DSS_default;
		GpuHandle<DepthStencilState> DSS_default_lessEqual;
		GpuHandle<DepthStencilState> DSS_default_lessEqual_noWrite;
		GpuHandle<DepthStencilState> DSS_always_noTest;
		GpuHandle<DepthStencilState> DSS_noWrite_noTest;

		GpuHandle<BlendState> BS_backToFrontAlpha;
		GpuHandle<BlendState> BS_addativeColor;
	};

  public:
	ICore() = default;
	virtual ~ICore() = default;

	virtual void setup(SGEDevice* const sgedev, AudioDevice* const sgeAudioDevice) = 0;

	/// @brief AssetLibrary is the "manager" of all loaded assets - 3d models, textures and others.
	virtual AssetLibrary* getAssetLib() = 0;

	/// @brief MaterialFamilyLibrary is the manager of all available materials trough the standard material
	/// pipeline, enabling assets to define materials and then get rendered via @IGeometryDrawer.
	virtual MaterialFamilyLibrary* getMaterialLib() = 0;

	/// @brief QuickDraw provides an easy way to draw simple shapes, textures and text.
	virtual QuickDraw& getQuickDraw() = 0;

	/// @brief DebugDraw provides a way for the user to easily draw, from anywhere in code, lines, boxes, arrows and
	/// more. The idea of this class is to help you visualize a particlar problem - maybe it is forces, collsion
	/// geometry, guides and more.
	virtual DebugDraw& getDebugDraw() = 0;

	virtual SGEDevice* getDevice() = 0;
	virtual GraphicsResources& getGraphicsResources() = 0;

	virtual void setInputState(const InputState& is) = 0;
	virtual const InputState& getInputState() const = 0;
	virtual const FrameStatistics& getLastFrameStatistics() const = 0;
	virtual void setLastFrameStatistics(const FrameStatistics& stats) = 0;

	virtual AudioDevice* getAudioDevice() = 0;
};


/// The global of the current module(dll, exe, ect.). However we could "borrow" another modules "ICore" and use theirs.
SGE_CORE_API ICore* getCore();
SGE_CORE_API void setCore(ICore* global);
} // namespace sge
