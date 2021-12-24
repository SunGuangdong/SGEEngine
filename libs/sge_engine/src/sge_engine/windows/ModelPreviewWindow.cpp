#include "ModelPreviewWindow.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/Camera.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/input.h"
#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/tiny/FileOpenDialog.h"
#include <imgui/imgui.h>

namespace sge {

/// Opens a file-open dialog asking the user to pik a mdl file.
static bool promptForModel(AssetPtr& asset) {
	AssetLibrary* const assetLib = getCore()->getAssetLib();

	const std::string filename = FileOpenDialog("Pick a model", true, "*.mdl\0*.mdl\0", nullptr);
	if (!filename.empty()) {
		asset = assetLib->getAssetFromFile(filename.c_str());
		return true;
	}
	return false;
}

void ModelPreviewWidget::doWidget(SGEContext* const sgecon, const InputState& is, EvaluatedModel& m_eval, Optional<vec2f> widgetSize) {
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
	mat4f proj = mat4f::getPerspectiveFovRH(deg2rad(45.f), ratio, 0.1f, 10000.f, 0.f,
	                                        kIsTexcoordStyleD3D); // The Y flip for OpenGL is done by the modelviewer.

	sgecon->clearColor(m_frameTarget, 0, vec4f(0.f).data);
	sgecon->clearDepth(m_frameTarget, 1.f);

	QuickDraw& debugDraw = getCore()->getQuickDraw();


	debugDraw.drawWired_Clear();
	debugDraw.drawWiredAdd_Grid(vec3f(0), vec3f::getAxis(0), vec3f::getAxis(2), 5, 5, 0xFF333733);
	debugDraw.drawWired_Execute(rdest, proj * lookAt, nullptr);

	RawCamera rawCamera = RawCamera(camera.eyePosition(), lookAt, proj);
	drawEvalModel(rdest, rawCamera, mat4f::getIdentity(), ObjectLighting::makeAmbientLightOnly(), m_eval, InstanceDrawMods());

	if (kIsTexcoordStyleD3D) {
		ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(canvas_size.x, canvas_size.y));
	} else {
		ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(canvas_size.x, canvas_size.y), ImVec2(0, 1), ImVec2(1, 0));
	}

	if (ImGui::IsItemHovered()) {
		camera.update(true, is.IsKeyDown(Key_MouseLeft), false, is.IsKeyDown(Key_MouseRight), is.GetCursorPos());
	}
}

void ModelPreviewWindow::update(SGEContext* const sgecon, const InputState& is) {
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		if (m_frameTarget.IsResourceValid() == false) {
			m_frameTarget = sgecon->getDevice()->requestResource<FrameTarget>();
			m_frameTarget->create2D(64, 64);
		}


		if (ImGui::Button("Pick##ModeToPreview")) {
			promptForModel(m_model);
			m_eval = EvaluatedModel();
			iPreviewAnimDonor = -1;
			iPreviewAnimation = -1;
			animationComboPreviewValue = "<None>";
			animationDonors.clear();
			previewAimationTime = 0.f;
			autoPlayAnimation = true;
			m_evalAnimator = ModelAnimator2();
			nextTrackIndex = 0;
			if (isAssetLoaded(m_model, assetIface_model3d)) {
				m_eval.initialize(&getLoadedAssetIface<AssetIface_Model3D>(m_model)->getModel3D());
				m_evalAnimator.create(*m_eval.m_model);

				nextTrackIndex = 0;
				for (int iAnim = 0; iAnim < m_eval.m_model->numAnimations(); ++iAnim) {
					m_evalAnimator.trackAddAmim(nextTrackIndex, nullptr, iAnim);

					trackDisplayName.push_back(m_eval.m_model->animationAt(iAnim)->animationName);
					trackAnimDuration.push_back(m_eval.m_model->animationAt(iAnim)->durationSec);
					nextTrackIndex++;
				}

				// If there is at least one track play it.
				if (nextTrackIndex > 0) {
					m_evalAnimator.playTrack(0);
				}
			}
		}

		if (isAssetLoaded(m_model)) {
			if (ImGui::Button("Add Animation from other model")) {
				AssetPtr animSrcModel;
				if (promptForModel(animSrcModel)) {
					if (AssetIface_Model3D* modelIface = getLoadedAssetIface<AssetIface_Model3D>(animSrcModel)) {
						Model& srcModel = modelIface->getModel3D();

						for (int iAnim = 0; iAnim < srcModel.numAnimations(); ++iAnim) {
							m_evalAnimator.trackAddAmim(nextTrackIndex, &srcModel, iAnim);

							trackDisplayName.push_back(m_eval.m_model->animationAt(iAnim)->animationName + " @ " + animSrcModel->getPath());
							trackAnimDuration.push_back(srcModel.animationAt(iAnim)->durationSec);
							nextTrackIndex++;
						}
					}
				}
			}
		}

		ImGui::SameLine();
		if (isAssetLoaded(m_model)) {
			ImGui::Text("%s", m_model->getPath().c_str());
		} else {
			ImGui::Text("Pick a Model");
		}

		ImGui::DragFloat("Time", &animationTime, 0.01f);

		if (m_model.get() != NULL && m_evalAnimator.getNumTacks() > 0) {

			int playingTrackId = m_evalAnimator.getPlayingTrackId();
			static std::string none = "None";
			const std::string& previewName = playingTrackId >= 0 ? trackDisplayName[playingTrackId] : none;

			if (ImGui::BeginCombo("Animation", previewName.c_str())) {
				for (int t = 0; t < trackDisplayName.size(); ++t) {
					if (ImGui::Selectable(trackDisplayName[t].c_str())) {
						m_evalAnimator.playTrack(t);
					}
				}

				ImGui::EndCombo();
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
			mat4f proj = mat4f::getPerspectiveFovRH(deg2rad(45.f), ratio, 0.1f, 10000.f, 0.f,
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
				camera.update(is.IsKeyDown(Key_LAlt), is.IsKeyDown(Key_MouseLeft), is.IsKeyDown(Key_MouseMiddle),
				              is.IsKeyDown(Key_MouseRight), is.GetCursorPos());
			}

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 canvasMax = canvas_pos;
			canvasMax.x += canvas_size.x;
			canvasMax.y += canvas_size.y;

			draw_list->AddImage(m_frameTarget->getRenderTarget(0), canvas_pos, canvasMax);
			draw_list->AddRect(canvas_pos, canvasMax, ImColor(255, 255, 255));
		}
	}
	ImGui::End();
}

} // namespace sge
