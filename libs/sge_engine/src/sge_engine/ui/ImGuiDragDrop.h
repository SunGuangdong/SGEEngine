#pragma once

#include "sge_core/SGEImGui.h"
#include "sge_engine/GameObject.h"
#include "sge_utils/utils/optional.h"
#include "sge_utils/utils/vector_set.h"
#include <set>

namespace sge {

//struct DragDropPayload {
//	DragDropPayload() = default;
//	virtual ~DragDropPayload() = default;
//};

struct DragDropPayloadActor {
	static sge::Optional<std::set<sge::ObjectId>> decode(const ImGuiPayload* payload);
	static sge::Optional<sge::ObjectId> decodeSingle(const ImGuiPayload* payload);
	static void setPayload(sge::ObjectId id);
	static void setPayload(vector_set<sge::ObjectId> ids);
	static sge::Optional<sge::ObjectId> acceptSingle();
	static sge::Optional<std::set<sge::ObjectId>> accept();
};

struct DragDropPayloadAsset {
	static sge::Optional<std::string> decode(const ImGuiPayload* payload);
	static void setPayload(const std::string& assetPath);
	static sge::Optional<std::string> accept();
};

} // namespace sge
