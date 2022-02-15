#include "ModelPreviewWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/Camera.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/input.h"
#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/other/FileOpenDialog.h"
#include <imgui/imgui.h>

#include <functional>

namespace sge {

/// Opens a file-open dialog asking the user to pik a mdl file.
static bool promptForModel(AssetPtr& asset)
{
	AssetLibrary* const assetLib = getCore()->getAssetLib();

	const std::string filename = FileOpenDialog("Pick a model", true, "*.mdl\0*.mdl\0", nullptr);
	if (!filename.empty()) {
		asset = assetLib->getAssetFromFile(filename.c_str());
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
// ModelPreviewWidget
//--------------------------------------------------------------------------
void ModelPreviewWidget::doWidget(
    SGEContext* const sgecon, const InputState& is, EvaluatedModel& m_eval, Optional<vec2f> widgetSize)
{
	if (m_frameTarget.IsResourceValid() == false) {
		m_frameTarget = sgecon->getDevice()->requestResource<FrameTarget>();
		m_frameTarget->create2D(64, 64);
	}

	RenderDestination rdest(sgecon, m_frameTarget);

	const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	vec2f canvas_size = widgetSize.isValid() ? widgetSize.get() : fromImGui(ImGui::GetContentRegionAvail());

	if (canvas_size.x < 0.f)
		canvas_size.x = ImGui::GetContentRegionAvail().x;
	if (canvas_size.y < 0.f)
		canvas_size.y = ImGui::GetContentRegionAvail().y;

	if (canvas_size.y < 64)
		canvas_size.y = 64;

	vec2f texsize = canvas_size;

	if (texsize.x < 64)
		texsize.x = 64;
	if (texsize.y < 64)
		texsize.y = 64;

	if (m_frameTarget->getWidth() != texsize.x || m_frameTarget->getHeight() != texsize.y) {
		m_frameTarget->create2D((int)texsize.x, (int)texsize.y);
	}

	float ratio = (float)m_frameTarget->getWidth() / (float)m_frameTarget->getHeight();

	mat4f lookAt = camera.GetViewMatrix();
	mat4f proj = mat4f::getPerspectiveFovRH(
	    deg2rad(45.f),
	    ratio,
	    0.1f,
	    10000.f,
	    0.f,
	    kIsTexcoordStyleD3D); // The Y flip for OpenGL is done by the modelviewer.

	sgecon->clearColor(m_frameTarget, 0, vec4f(0.f).data);
	sgecon->clearDepth(m_frameTarget, 1.f);

	QuickDraw& debugDraw = getCore()->getQuickDraw();


	debugDraw.drawWired_Clear();
	debugDraw.drawWiredAdd_Grid(vec3f(0), vec3f::getAxis(0), vec3f::getAxis(2), 5, 5, 0xFF333733);
	debugDraw.drawWired_Execute(rdest, proj * lookAt, nullptr);

	RawCamera rawCamera = RawCamera(camera.eyePosition(), lookAt, proj);
	drawEvalModel(
	    rdest, rawCamera, mat4f::getIdentity(), ObjectLighting::makeAmbientLightOnly(), m_eval, InstanceDrawMods());

	if (kIsTexcoordStyleD3D) {
		ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(canvas_size.x, canvas_size.y));
	}
	else {
		ImGui::Image(
		    m_frameTarget->getRenderTarget(0), ImVec2(canvas_size.x, canvas_size.y), ImVec2(0, 1), ImVec2(1, 0));
	}

	if (ImGui::IsItemHovered()) {
		camera.update(true, is.IsKeyDown(Key_MouseLeft), false, is.IsKeyDown(Key_MouseRight), is.GetCursorPos());
	}
}

void ModelPreviewWindow::setPreviewModel(AssetPtr asset)
{
	m_model = asset;
	m_eval = EvaluatedModel();
	autoPlayAnimation = true;
	m_evalAnimator = ModelAnimator();
	previewAnimationsInfo.clear();
	nextTrackIndex = 0;

	if (isAssetLoaded(m_model, assetIface_model3d)) {
		m_eval.initialize(getLoadedAssetIface<AssetIface_Model3D>(m_model)->getModel3D());
		m_evalAnimator.create(*m_eval.m_model);

		nextTrackIndex = 0;
		for (int iAnim = 0; iAnim < m_eval.m_model->numAnimations(); ++iAnim) {
			m_evalAnimator.trackAddAmim(nextTrackIndex, nullptr, iAnim);

			AnimationDisplayInfo animInfo;
			animInfo.name = m_eval.m_model->animationAt(iAnim)->animationName;
			animInfo.duration = m_eval.m_model->animationAt(iAnim)->durationSec;

			previewAnimationsInfo.push_back(animInfo);

			nextTrackIndex++;
		}

		// If there is at least one track play it.
		if (nextTrackIndex > 0) {
			m_evalAnimator.playTrack(0);
		}
	}
}

//--------------------------------------------------------------------------
// ModelPreviewWindow
//--------------------------------------------------------------------------
void ModelPreviewWindow::update(SGEContext* const sgecon, struct GameInspector* UNUSED(inspector), const InputState& is)
{
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		if (m_frameTarget.IsResourceValid() == false) {
			m_frameTarget = sgecon->getDevice()->requestResource<FrameTarget>();
			m_frameTarget->create2D(64, 64);
		}

		if (ImGui::Button(ICON_FK_FILE " Pick a Model File##ModelToPreview")) {
			AssetPtr newModel;
			promptForModel(newModel);
			setPreviewModel(newModel);
		}

		ImGui::Columns(2);

		if (isAssetLoaded(m_model)) {
			if (ImGui::Button(ICON_FK_FILE_O " Add animation from another Model")) {
				AssetPtr animSrcModel;
				if (promptForModel(animSrcModel)) {
					if (AssetIface_Model3D* modelIface = getLoadedAssetIface<AssetIface_Model3D>(animSrcModel)) {
						Model* srcModel = modelIface->getModel3D();
						if (modelIface) {
							// Add all animations each in a separate track.
							for (int iAnim = 0; iAnim < srcModel->numAnimations(); ++iAnim) {
								m_evalAnimator.trackAddAmim(nextTrackIndex, srcModel, iAnim);

								AnimationDisplayInfo animInfo;
								animInfo.name =
								    m_eval.m_model->animationAt(iAnim)->animationName + " @ " + animSrcModel->getPath();
								animInfo.duration = srcModel->animationAt(iAnim)->durationSec;

								previewAnimationsInfo.push_back(animInfo);

								nextTrackIndex++;
							}
						}
					}
				}
			}

			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip(
				    "You can apply animations for other models to this one, as long as they have identical node "
				    "hierarchy.");
			}
		}

		ImGui::SameLine();
		if (isAssetLoaded(m_model)) {
			ImGui::Text("%s", m_model->getPath().c_str());
		}
		else {
			ImGui::Text("Pick a Model");
		}

		if (m_model.get()) {
			int playingTrackId = m_evalAnimator.getPlayingTrackId();

			if (m_evalAnimator.getNumTacks() > 0) {
				static std::string none = "None";
				const std::string& previewAnimName =
				    playingTrackId >= 0 ? previewAnimationsInfo[playingTrackId].name : none;

				ImGui::SliderFloat("Time", &animationTime, 0.f, previewAnimationsInfo[playingTrackId].duration);

				/// Drop down for picking an animation track (we add one track per animation).
				if (ImGui::BeginCombo(ICON_FK_FILM " Animation", previewAnimName.c_str())) {
					for (int t = 0; t < previewAnimationsInfo.size(); ++t) {
						if (ImGui::Selectable(previewAnimationsInfo[t].name.c_str())) {
							m_evalAnimator.playTrack(t);
						}
					}

					ImGui::EndCombo();
				}
			}
			else {
				ImGui::Text("No animation found.");
			}

			m_evalAnimator.forceTrack(m_evalAnimator.getPlayingTrackId(), animationTime);

			std::vector<mat4f> nodeTrasf;
			m_evalAnimator.computeModleNodesTrasnforms(nodeTrasf);
			m_eval.evaluate(nodeTrasf.data(), int(nodeTrasf.size()));

			const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
			ImVec2 canvas_size = ImGui::GetContentRegionAvail();

			if (canvas_size.y < 64)
				canvas_size.y = 64;

			ImVec2 texsize = canvas_size;

			if (texsize.x < 64)
				texsize.x = 64;
			if (texsize.y < 64)
				texsize.y = 64;

			if (m_frameTarget->getWidth() != texsize.x || m_frameTarget->getHeight() != texsize.y) {
				m_frameTarget->create2D((int)texsize.x, (int)texsize.y);
			}

			float ratio = (float)m_frameTarget->getWidth() / (float)m_frameTarget->getHeight();

			mat4f lookAt = camera.GetViewMatrix();
			mat4f proj = mat4f::getPerspectiveFovRH(
			    deg2rad(45.f),
			    ratio,
			    0.1f,
			    10000.f,
			    0.f,
			    kIsTexcoordStyleD3D); // The Y flip for OpenGL is done by the modelviewer.

			sgecon->clearColor(m_frameTarget, 0, vec4f(0.f).data);
			sgecon->clearDepth(m_frameTarget, 1.f);

			QuickDraw& debugDraw = getCore()->getQuickDraw();

			RenderDestination rdest(sgecon, m_frameTarget, m_frameTarget->getViewport());

			debugDraw.drawWiredAdd_Grid(vec3f(0), vec3f::getAxis(0), vec3f::getAxis(2), 5, 5, 0xFF333733);
			debugDraw.drawWired_Execute(rdest, proj * lookAt, nullptr);

			RawCamera rawCamera = RawCamera(camera.eyePosition(), lookAt, proj);
			drawEvalModel(rdest, rawCamera, mat4f::getIdentity(), ObjectLighting(), m_eval, InstanceDrawMods());

			ImGui::InvisibleButton("TextureCanvasIB", canvas_size);

			if (ImGui::IsItemHovered()) {
				camera.update(
				    is.IsKeyDown(Key_LAlt),
				    is.IsKeyDown(Key_MouseLeft),
				    is.IsKeyDown(Key_MouseMiddle),
				    is.IsKeyDown(Key_MouseRight),
				    is.GetCursorPos());
			}

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 canvasMax = canvas_pos;
			canvasMax.x += canvas_size.x;
			canvasMax.y += canvas_size.y;

			draw_list->AddImage(m_frameTarget->getRenderTarget(0), canvas_pos, canvasMax);
			draw_list->AddRect(canvas_pos, canvasMax, ImColor(255, 255, 255));
		}
	}

	ImGui::NextColumn();

	ImGui::Text("info");

	// Do the information side bar.
	AssetIface_Model3D* modelIface = getLoadedAssetIface<AssetIface_Model3D>(m_model);
	Model* model = modelIface ? modelIface->getModel3D() : nullptr;

	if (model) {
		ImGui::Text(ICON_FK_FILM " Animation Count: %d", model->numAnimations());
		ImGui::Text(ICON_FK_CUBES " Node Count: %d", model->numNodes());
		ImGui::Text(ICON_FK_CUBE " Mesh Count: %d", model->numMeshes());
		ImGui::Text(ICON_FK_PICTURE_O " Material Count: %d", model->numMaterials());

		if (ImGui::CollapsingHeader(ICON_FK_FILM " Animations", ImGuiTreeNodeFlags_DefaultOpen)) {
			for (int iAnim = 0; iAnim < model->numAnimations(); ++iAnim) {
				ImGui::Text(
				    "\t%s duration=%f",
				    model->animationAt(iAnim)->animationName.c_str(),
				    model->animationAt(iAnim)->durationSec);
			}
		}

		if (ImGui::CollapsingHeader(ICON_FK_CUBES " Nodes")) {
			std::function<void(int)> recusiveDoNodeInfo = [&](int nodeIndex) {
				const ModelNode* node = model->nodeAt(nodeIndex);

				ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;

				if (node->childNodes.size() == 0) {
					treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
				}

				bool isTreeNodeOpen = ImGui::TreeNodeEx(node->name.c_str(), treeNodeFlags);

				if (isTreeNodeOpen) {
					bool hasAnythingBelow = false;
					for (int childNodeIndex : node->childNodes) {
						recusiveDoNodeInfo(childNodeIndex);

						hasAnythingBelow = true;
					}

					for (const MeshAttachment& attachment : node->meshAttachments) {
						const int meshIndex = attachment.attachedMeshIndex;
						const int mtlIndex = attachment.attachedMeshIndex;
						const ModelMesh* mesh = model->meshAt(meshIndex);
						const ModelMaterial* mtl = model->materialAt(mtlIndex);
						ImGui::Text("Attached Mesh [index=%d] %s", meshIndex, mesh->name.c_str());
						ImGui::Text("Attached Material [index=%d] %s", meshIndex, mtl->name.c_str());


						hasAnythingBelow = true;
					}

					ImGui::TreePop();
				}
			};

			recusiveDoNodeInfo(model->getRootNodeIndex());
		}

		if (ImGui::CollapsingHeader(ICON_FK_CUBES " Meshes")) {
			for (int iMesh = 0; iMesh < model->numMeshes(); ++iMesh) {
				const ModelMesh* mesh = model->meshAt(iMesh);

				ImGui::Text("Mesh [index=%d] %s", iMesh, mesh->name.c_str());
				ImGui::Text("\tNum Vertices: %d", mesh->numVertices);
				ImGui::Text("\tNum Element(indices): %d", mesh->numElements);
				ImGui::Text("\tNum Vertex Stride: %d", mesh->stride);

				ImGui::Text("\tVertex Declaration (with index %d): ", mesh->vertexDeclIndex);
				for (const VertexDecl& vertexDecl : mesh->vertexDecl) {
					ImGui::NewLine();
					ImGui::Text("\t\tName: %s", vertexDecl.semantic.c_str());
					ImGui::Text("\t\tBuffer Slot: %d", (int)vertexDecl.bufferSlot);
					ImGui::Text("\t\tBuffer format: %d", (int)vertexDecl.format);
					ImGui::Text("\t\tByte Offset: %d", (int)vertexDecl.byteOffset);
				}
				ImGui::NewLine();
			}
		}

		if (ImGui::CollapsingHeader(ICON_FK_PICTURE_O " Materials")) {
			for (int iMtl = 0; iMtl < model->numMaterials(); ++iMtl) {
				ModelMaterial* mtl = model->materialAt(iMtl);
				ImGui::Text("Material [index=%d] %s", iMtl, mtl->name.c_str());
				ImGui::Text("\tAsset for Material: %s", mtl->assetForThisMaterial.c_str());
			}
		}
	}

	ImGui::End();
}

} // namespace sge
