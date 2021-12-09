#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"


namespace sge {

/// An interface for something that can give you a 3D model.
struct SGE_CORE_API AssetIface_Model3D : public IAssetInterface {
	virtual Model& getModel3D() = 0;
	virtual const Model& getModel3D() const = 0;

	/// The state of the model with no animation applied.
	/// Almost all models are not going to have any animation on them
	/// and it is convinient to just reuse the same evaluation instead of makind a new one
	/// for every single instance.
	/// For every other case the user should create its own EvaluatedModel based on @getModel3D.
	virtual EvaluatedModel& getStaticEval() = 0;
	virtual const EvaluatedModel& getStaticEval() const = 0;
};

struct SGE_CORE_API AssetModel3D : public Asset, public AssetIface_Model3D {
	AssetModel3D(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib) {
	}

	virtual Model& getModel3D() override {
		return m_model;
	}

	virtual const Model& getModel3D() const override {
		return m_model;
	}


	virtual EvaluatedModel& getStaticEval() override {
		return m_staticEval;
	}


	virtual const EvaluatedModel& getStaticEval() const override {
		return m_staticEval;
	}

	bool loadAssetFromFile(const char* const path) override;

  public:
	Model m_model;
	EvaluatedModel m_staticEval;
};

} // namespace sge
