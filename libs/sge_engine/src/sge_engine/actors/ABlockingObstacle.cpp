#include <imgui/imgui.h>

#include "ABlockingObstacle.h"
#include "sge_core/ICore.h"
#include "sge_core/typelib/MemberChain.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/InspectorCmds.h"
#include "sge_engine_ui/windows/PropertyEditorWindow.h"


namespace sge {

// clang-format off
ReflAddTypeId(ABlockingObstacle, 20'03'02'0013);
ReflBlock() {
	ReflAddActor(ABlockingObstacle)
		ReflMember(ABlockingObstacle, targetDesc)
		ReflMember(ABlockingObstacle, m_textureX)
		ReflMember(ABlockingObstacle, m_textureXScale)
		ReflMember(ABlockingObstacle, m_textureY)
		ReflMember(ABlockingObstacle, m_textureYScale)
		ReflMember(ABlockingObstacle, m_rbPropConfig)
	;
}
// clang-format on

void ABlockingObstacle::create()
{
	registerTrait(m_traitRB);
	registerTrait(*static_cast<IActorCustomAttributeEditorTrait*>(this));
	registerTrait(tt_rendGeom);

	this->currentDesc.stairs.numStairs = 0; // Just make this something invalid inorder to force generate on creation.

	// Disable making the rigid body dynamic. This Actor represents a static object.
	m_rbPropConfig.extractPropsFromRigidBody(*m_traitRB.getRigidBody());
	m_rbPropConfig.dontShowDynamicProperties = true;

	postUpdateSubscription =
	    Actor::getWorld()->onPostUpdate.subscribe([this](const GameUpdateSets& u) { postUpdate(u); });
}

void ABlockingObstacle::onPlayStateChanged(bool const isStartingToPlay)
{
	if (isStartingToPlay) {
		m_rbPropConfig.applyProperties(*this);
	}
}

void ABlockingObstacle::update(const GameUpdateSets& UNUSED(u))
{
	material = SimpleTriplanarMtlData();

	material.diffuseTextureX = m_textureX.getAssetInterface<AssetIface_Texture2D>()
	                               ? m_textureX.getAssetInterface<AssetIface_Texture2D>()->getTexture()
	                               : nullptr;
	material.diffuseTextureY = m_textureY.getAssetInterface<AssetIface_Texture2D>()
	                               ? m_textureY.getAssetInterface<AssetIface_Texture2D>()->getTexture()
	                               : nullptr;
	material.diffuseTextureZ = material.diffuseTextureX;

	material.uvwTransform = mat4f::getScaling(m_textureXScale, m_textureYScale, m_textureXScale);
	material.sharpness = 64.f;
}

void ABlockingObstacle::postUpdate(const GameUpdateSets& UNUSED(updateSets))
{
	m_textureX.update();
	m_textureY.update();

	// Check if the description of the obstacle has changed.
	// If so update the object,
	if (memcmp(&currentDesc, &targetDesc, sizeof(currentDesc)) != 0) {
		boundingBox.setEmpty();

		if (m_traitRB.getRigidBody()->isValid()) {
			GameObject::getWorld()->physicsWorld.removePhysicsObject(*m_traitRB.getRigidBody());
			m_traitRB.getRigidBody()->destroy();
		}

		currentDesc = targetDesc;

		// Rendering geometry.
		std::vector<TerrainGenerator::Vertex> vertices;
		std::vector<int> indices;

		// Collision geometry.
		std::vector<Box3f> bboxes;

		ModelCollisionMesh slopeCollisionMesh;

		if (currentDesc.type == SimpleObstacleType::Stairs &&
		    TerrainGenerator::generateStairs(vertices, indices, bboxes, currentDesc.stairs, &numVerts, &numIndices)) {
			// Create the rendering resources.
			BufferDesc const vbDesc = BufferDesc::GetDefaultVertexBuffer(vertices.size() * sizeof(vertices[0]));
			BufferDesc const ibDesc = BufferDesc::GetDefaultIndexBuffer(indices.size() * sizeof(indices[0]));

			this->vertexBuffer = getCore()->getDevice()->requestResource<Buffer>();
			this->vertexBuffer->create(vbDesc, vertices.data());

			this->indexBuffer = getCore()->getDevice()->requestResource<Buffer>();
			this->indexBuffer->create(ibDesc, indices.data());

			// Create the physics rigid body.
			std::vector<CollsionShapeDesc> bboxesShapeDescs;
			for (const Box3f& box : bboxes) {
				bboxesShapeDescs.emplace_back(CollsionShapeDesc::createBox(box));
			}

			m_traitRB.getRigidBody()->create(this, bboxesShapeDescs.data(), int(bboxesShapeDescs.size()), 0.f, false);
			m_rbPropConfig.applyProperties(*this);

			// Compute the bounding box of the whole thing:
			boundingBox.setEmpty();
			for (const Box3f& box : bboxes) {
				boundingBox.expand(box);
			}

			this->setTransform(getTransform(), true);

			GameObject::getWorld()->physicsWorld.addPhysicsObject(*m_traitRB.getRigidBody());
		}
		else if (
		    currentDesc.type == SimpleObstacleType::Slope &&
		    TerrainGenerator::generateSlope(
		        vertices, indices, slopeCollisionMesh, currentDesc.slope, &numVerts, &numIndices)) {
			// Create the rendering resources.
			BufferDesc const vbDesc = BufferDesc::GetDefaultVertexBuffer(vertices.size() * sizeof(vertices[0]));
			BufferDesc const ibDesc = BufferDesc::GetDefaultIndexBuffer(indices.size() * sizeof(indices[0]));

			this->vertexBuffer = getCore()->getDevice()->requestResource<Buffer>();
			this->vertexBuffer->create(vbDesc, vertices.data());

			this->indexBuffer = getCore()->getDevice()->requestResource<Buffer>();
			this->indexBuffer->create(ibDesc, indices.data());

			// Create the physics rigid body.
			CollsionShapeDesc convexHullDesc =
			    CollsionShapeDesc::createConvexPoly(slopeCollisionMesh.vertices, slopeCollisionMesh.indices);
			m_traitRB.getRigidBody()->create((Actor*)this, &convexHullDesc, 1, 0.f, false);
			m_rbPropConfig.applyProperties(*this);

			// Compute the bounding box of the whole thing:
			boundingBox.setEmpty();
			for (const vec3f& vert : slopeCollisionMesh.vertices) {
				boundingBox.expand(vert);
			}

			this->setTransform(getTransform(), true);

			GameObject::getWorld()->physicsWorld.addPhysicsObject(*m_traitRB.getRigidBody());
		}
		else if (
		    currentDesc.type == SimpleObstacleType::SlantedBlock &&
		    TerrainGenerator::generateSlantedBlock(
		        vertices, indices, slopeCollisionMesh, currentDesc.slantedBlock, &numVerts, &numIndices)) {
			// Create the rendering resources.
			BufferDesc const vbDesc = BufferDesc::GetDefaultVertexBuffer(vertices.size() * sizeof(vertices[0]));
			BufferDesc const ibDesc = BufferDesc::GetDefaultIndexBuffer(indices.size() * sizeof(indices[0]));

			this->vertexBuffer = getCore()->getDevice()->requestResource<Buffer>();
			this->vertexBuffer->create(vbDesc, vertices.data());

			this->indexBuffer = getCore()->getDevice()->requestResource<Buffer>();
			this->indexBuffer->create(ibDesc, indices.data());

			// Create the physics rigid body.
			CollsionShapeDesc convexHullDesc =
			    CollsionShapeDesc::createConvexPoly(slopeCollisionMesh.vertices, slopeCollisionMesh.indices);
			m_traitRB.getRigidBody()->create((Actor*)this, &convexHullDesc, 1, 0.f, false);
			m_rbPropConfig.applyProperties(*this);

			// Compute the bounding box of the whole thing:
			boundingBox.setEmpty();
			for (const vec3f& vert : slopeCollisionMesh.vertices) {
				boundingBox.expand(vert);
			}

			this->setTransform(getTransform(), true);

			GameObject::getWorld()->physicsWorld.addPhysicsObject(*m_traitRB.getRigidBody());
		}

		VertexDecl vertexDecl[2] = {
		    VertexDecl(0, "a_position", UniformType::Float3, 0),
		    VertexDecl(0, "a_normal", UniformType::Float3, 3 * sizeof(float)),
		};

		VertexDeclIndex vertexDeclIdx = getCore()->getDevice()->getVertexDeclIndex(vertexDecl, SGE_ARRSZ(vertexDecl));

		geometry = Geometry(
		    vertexBuffer,
		    indexBuffer,
		    nullptr,
		    -1,
		    vertexDeclIdx,
		    false,
		    false,
		    true,
		    false,
		    PrimitiveTopology::TriangleList,
		    0,
		    0,
		    sizeof(vec3f) * 2,
		    UniformType::Uint,
		    numIndices);


		tt_rendGeom.geoms.resize(1);

		tt_rendGeom.geoms[0].pGeom = &geometry;
		tt_rendGeom.geoms[0].pMtl = &material;
		tt_rendGeom.geoms[0].bboxGeometry = boundingBox;
	}
}

Box3f ABlockingObstacle::getBBoxOS() const
{
	return boundingBox;
}

void ABlockingObstacle::doAttributeEditor(GameInspector* inspector)
{
	MemberChain chain;

	chain.add(typeLib().find<ABlockingObstacle>()->findMember(&ABlockingObstacle::targetDesc));
	chain.add(typeLib().find<SimpleObstacleDesc>()->findMember(&SimpleObstacleDesc::type));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	if (targetDesc.type == SimpleObstacleType::Stairs) {
		chain.add(typeLib().find<SimpleObstacleDesc>()->findMember(&SimpleObstacleDesc::stairs));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();
	}

	if (targetDesc.type == SimpleObstacleType::Slope) {
		chain.add(typeLib().find<SimpleObstacleDesc>()->findMember(&SimpleObstacleDesc::slope));
		ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
		chain.pop();
	}

	chain.clear();
	chain.add(typeLib().find<ABlockingObstacle>()->findMember(&ABlockingObstacle::m_textureY));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.clear();
	chain.add(typeLib().find<ABlockingObstacle>()->findMember(&ABlockingObstacle::m_textureYScale));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.clear();
	chain.add(typeLib().find<ABlockingObstacle>()->findMember(&ABlockingObstacle::m_textureX));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.clear();
	chain.add(typeLib().find<ABlockingObstacle>()->findMember(&ABlockingObstacle::m_textureXScale));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();


	chain.clear();
	chain.add(typeLib().find<ABlockingObstacle>()->findMember(&ABlockingObstacle::m_rbPropConfig));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	// Some presets to resize the geometry.
	if (ImGui::Button("2x1x2")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(2.f, 1.f, 2.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);

		inspector->appendCommand(cmd, true);
	}

	ImGui::SameLine();

	if (ImGui::Button("4x1x4")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(4.f, 1.f, 4.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);

		inspector->appendCommand(cmd, true);
	}

	ImGui::SameLine();

	if (ImGui::Button("8x1x8")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(8.f, 1.f, 8.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}


	if (ImGui::Button("2x4x2")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(2.f, 4.f, 2.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}

	ImGui::SameLine();

	if (ImGui::Button("4x4x4")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(4.f, 4.f, 4.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}

	ImGui::SameLine();

	if (ImGui::Button("8x4x8")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(8.f, 4.f, 8.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}

	if (ImGui::Button("2x8x2")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(2.f, 8.f, 2.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}

	ImGui::SameLine();

	if (ImGui::Button("4x8x4")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(4.f, 4.f, 4.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}

	ImGui::SameLine();

	if (ImGui::Button("8x8x8")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(8.f, 8.f, 8.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}

	if (ImGui::Button("1x2x1")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(1.f, 2.f, 1.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}

	if (ImGui::Button("1x4x1")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(1.f, 4.f, 1.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}

	ImGui::SameLine();

	if (ImGui::Button("1x8x1")) {
		transf3d newTransform = getTransform();
		newTransform.s = vec3f(1.f, 8.f, 1.f);
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, newTransform);
		inspector->appendCommand(cmd, true);
	}
}

} // namespace sge
