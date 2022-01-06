#include "ImGuiDragDrop.h"


namespace sge {
//-----------------------------------------------------
// DragDropPayloadActor
//-----------------------------------------------------
sge::Optional<std::set<sge::ObjectId>> DragDropPayloadActor::decode(const ImGuiPayload* payload) {
	if (payload && payload->IsDataType("sgedd_actors") && payload->Data != nullptr && payload->DataSize % sizeof(int) == 0) {
		int numActors = payload->DataSize / sizeof(int);
		std::set<sge::ObjectId> objects;

		const int* idsRaw = (const int*)payload->Data;
		for (int t = 0; t < numActors; ++t) {
			objects.insert(sge::ObjectId(idsRaw[t]));
		}

		return objects;
	}
	return sge::NullOptional();
}
sge::Optional<sge::ObjectId> DragDropPayloadActor::decodeSingle(const ImGuiPayload* payload) {
	if (payload && payload->IsDataType("sgedd_actors") && payload->Data != nullptr && payload->DataSize == sizeof(int)) {
		const int* idRaw = (int*)payload->Data;
		return sge::ObjectId(*idRaw);
	}
	return sge::NullOptional();
}

void DragDropPayloadActor::setPayload(sge::ObjectId id) {
	int idInt = id.id;
	ImGui::SetDragDropPayload("sgedd_actors", &idInt, sizeof(idInt));
}

void DragDropPayloadActor::setPayload(vector_set<sge::ObjectId> ids) {
	std::vector<int> linearIds;
	linearIds.reserve(ids.size());
	for (const sge::ObjectId& id : ids) {
		linearIds.push_back(id.id);
	}

	ImGui::SetDragDropPayload("sgedd_actors", linearIds.data(), sizeof(linearIds[0]) * linearIds.size());
}

sge::Optional<sge::ObjectId> DragDropPayloadActor::acceptSingle() {
	if (ImGui::AcceptDragDropPayload("sgedd_actors")) {
		return decodeSingle(ImGui::GetDragDropPayload());
	}
	return sge::NullOptional();
}

sge::Optional<std::set<sge::ObjectId>> DragDropPayloadActor::accept() {
	if (ImGui::AcceptDragDropPayload("sgedd_actors")) {
		return decode(ImGui::GetDragDropPayload());
	}
	return sge::NullOptional();
}

//-----------------------------------------------------
// DragDropPayloadAsset
//-----------------------------------------------------
sge::Optional<std::string> DragDropPayloadAsset::decode(const ImGuiPayload* payload) {
	if (payload && payload->IsDataType("sgedd_asset") && payload->Data != nullptr && payload->DataSize > 0) {
		return std::string((char*)payload->Data, payload->DataSize);
	}
	return sge::NullOptional();
}

void DragDropPayloadAsset::setPayload(const std::string& assetPath) {
	if (assetPath.empty() == false) {
		ImGui::SetDragDropPayload("sgedd_asset", assetPath.c_str(), assetPath.size());
	}
}

sge::Optional<std::string> DragDropPayloadAsset::accept() {
	if (ImGui::AcceptDragDropPayload("sgedd_asset")) {
		return decode(ImGui::GetDragDropPayload());
	}
	return sge::NullOptional();
}

//-----------------------------------------------------
// DragDropPayloadPrefabFile
//-----------------------------------------------------
sge::Optional<std::string> DragDropPayloadPrefabFile::decode(const ImGuiPayload* payload) {
	if (payload && payload->IsDataType("sgedd_prefab_path") && payload->Data != nullptr && payload->DataSize > 0) {
		return std::string((char*)payload->Data, payload->DataSize);
	}
	return sge::NullOptional();
}

void DragDropPayloadPrefabFile::setPayload(const std::string& assetPath) {
	if (assetPath.empty() == false) {
		ImGui::SetDragDropPayload("sgedd_prefab_path", assetPath.c_str(), assetPath.size());
	}
}

sge::Optional<std::string> DragDropPayloadPrefabFile::accept() {
	if (ImGui::AcceptDragDropPayload("sgedd_prefab_path")) {
		return decode(ImGui::GetDragDropPayload());
	}
	return sge::NullOptional();
}

} // namespace sge
