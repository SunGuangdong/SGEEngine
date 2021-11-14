#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"


namespace sge {

struct SGE_CORE_API AssetIface_Model3D : public IAssetInterface {
	virtual Model& getModel3D() = 0;
	virtual EvaluatedModel& getStaticEval() = 0;
	virtual EvaluatedModel& getSharedEval() = 0;

	virtual const Model& getModel3D() const = 0;
	virtual const EvaluatedModel& getStaticEval() const = 0;
	virtual const EvaluatedModel& getSharedEval() const = 0;
};

struct SGE_CORE_API AssetModel3D : public Asset, public AssetIface_Model3D {
	AssetModel3D(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib) {
	}

	virtual Model& getModel3D() override {
		return m_model;
	}

	virtual EvaluatedModel& getStaticEval() override {
		return m_staticEval;
	}

	virtual EvaluatedModel& getSharedEval() override {
		return m_sharedEval;
	}

	virtual const Model& getModel3D() const override {
		return m_model;
	}

	virtual const EvaluatedModel& getStaticEval() const override {
		return m_staticEval;
	}

	virtual const EvaluatedModel& getSharedEval() const override {
		return m_sharedEval;
	}

	bool loadAssetFromFile(const char* const path) override;

  public:
	Model m_model;
	EvaluatedModel m_staticEval;
	EvaluatedModel m_sharedEval;
};


} // namespace sge
