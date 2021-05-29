#pragma once

#include <string>

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/math/primitives.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/ChunkContainer.h"

#include "CollisionMesh.h"

namespace sge {

namespace Model {
	struct CollisionShapeBox {
		CollisionShapeBox() = default;
		CollisionShapeBox(std::string name, transf3d transform, vec3f halfDiagonal)
		    : name(std::move(name))
		    , transform(transform)
		    , halfDiagonal(halfDiagonal) {}

		std::string name;
		transf3d transform = transf3d::getIdentity();
		vec3f halfDiagonal = vec3f(0.f);
	};

	/// Represents a capsule. The capsule is defined by all points that are at distace "radius" from
	/// the line defined by ({0.f, -halfHeight, 0.f}, {0.f, halfHeight, 0.f}}
	struct CollisionShapeCapsule {
		CollisionShapeCapsule() = default;
		CollisionShapeCapsule(std::string name, transf3d transform, float halfHeight, float radius)
		    : name(std::move(name))
		    , transform(transform)
		    , halfHeight(halfHeight)
		    , radius(radius) {}

		std::string name;
		transf3d transform = transf3d::getIdentity();
		float halfHeight = 0.f;
		float radius = 0.f;
	};

	struct CollisionShapeCylinder {
		CollisionShapeCylinder() = default;
		CollisionShapeCylinder(std::string name, transf3d transform, vec3f halfDiagonal)
		    : name(std::move(name))
		    , transform(transform)
		    , halfDiagonal(halfDiagonal) {}

		std::string name;
		transf3d transform = transf3d::getIdentity();
		vec3f halfDiagonal = vec3f(0.f);
	};

	struct CollisionShapeSphere {
		CollisionShapeSphere() = default;
		CollisionShapeSphere(std::string name, transf3d transform, float radius)
		    : name(std::move(name))
		    , transform(transform)
		    , radius(radius) {}

		std::string name;
		transf3d transform = transf3d::getIdentity();
		float radius = 0.f;
	};

	// Loading settings describing how the model should be loaded.
	struct LoadSettings {
		enum KeepMeshData {
			KeepMeshData_None, // Do not keep any vertex/index buffer data for the CPU.
			KeepMeshData_Skin, // Keep only vetex/index buffer data for meshes with skinning for the CPU.
			KeepMeshData_All,  // Keep all vertex/index buffer data for the CPU.
		};

		KeepMeshData cpuMeshData = KeepMeshData_All;

		// The directory of the file, used for loading asset.
		std::string assetDir;
	};

	// Mesh material.
	struct Material {
		std::string name;

		vec4f diffuseColor = vec4f(1.f);
		vec4f emissionColor = vec4f(0.f);
		float metallic = 0.f;
		float roughness = 1.f;

		std::string diffuseTextureName;
		std::string emissionTextureName;
		std::string normalTextureName;
		std::string metallicTextureName;
		std::string roughnessTextureName;
	};

	// Skinning bone.
	struct Bone {
		Bone() = default;
		Bone(const mat4f& offsetMatrix, int nodeIdx)
		    : offsetMatrix(offsetMatrix)
		    , nodeIdx(nodeIdx) {}

		mat4f offsetMatrix = mat4f::getIdentity(); ///< The transformation matrix used for binding a bone to a mesh.
		int nodeIdx = -1;                          ///< The index of the node representing this bone transformation.
	};

	struct SGE_CORE_API Mesh {
		std::string name; ///< The name of the mesh.

		PrimitiveTopology::Enum primTopo = PrimitiveTopology::Unknown;
		int vbByteOffset = 0;                           ///< 1st vertex byte offset into the vertex buffer
		int ibByteOffset = 0;                           ///< 1st index byte offse int the index buffer
		UniformType::Enum ibFmt = UniformType::Unknown; ///< The format the index buffer, if unknown this mesh doesn't use index buffers.
		int numElements = 0;                            ///< The number of vertices/indices used by this mesh.
		int numVertices = 0;                            ///< The number of vertices in the mesh.
		std::vector<VertexDecl> vertexDecl;             ///< The vertex declaration

		int stride = 0;                    ///< The byte size stride of a single vertex.
		int vbVertexColorOffsetBytes = -1; ///, The byte offset of the vertex color.
		int vbPositionOffsetBytes = -1;    ///< The byte offset in the stride of vertex position.
		int vbNormalOffsetBytes = -1;      ///< The byte offset in the stride of the vertex normal.
		int vbTangetOffsetBytes = -1;
		int vbBinormalOffsetBytes = -1;
		int vbUVOffsetBytes = -1; ///< The byte offset in the stride of the 1st vertex UV channel.
		int vbBonesIdsBytesOffset = -1;
		int vbBonesWeightsByteOffset = -1;

		GpuHandle<Buffer> vertexBuffer; ///< The vertex buffer to be used for rendering of that mesh.
		GpuHandle<Buffer> indexBuffer;  ///< The index buffer to be used for rendering of that mesh.

		std::vector<char> vertexBufferRaw; ///< The raw data containing all vertices in the vertex buffer,
		std::vector<char> indexBufferRaw;  ///< The raw data containing all indices in the vertex buffer,

		AABox3f aabox; ///< The bounding box around the vertices of the mesh, without any deformation by skinning or anything else.

		int materialIndex = -1;  ///< The material assigned by default to this mesh.
		std::vector<Bone> bones; ///< A list of bones affecting the mesh.
	};

	struct MeshAttachment {
		MeshAttachment() = default;

		MeshAttachment(int attachedMeshIndex, int attachedMaterialIndex)
		    : attachedMeshIndex(attachedMeshIndex)
		    , attachedMaterialIndex(attachedMaterialIndex) {}

		int attachedMeshIndex = -1;
		int attachedMaterialIndex = -1;
	};

	struct KeyFrames {
		std::map<float, vec3f> positionKeyFrames;
		std::map<float, quatf> rotationKeyFrames;
		std::map<float, vec3f> scalingKeyFrames;

		void evaluate(transf3d& result, const float t) const;
	};

	struct Animation {
		Animation() = default;

		Animation(std::string animationName, float durationSec, std::map<int, KeyFrames> perNodeKeyFrames)
		    : animationName(std::move(animationName))
		    , durationSec(durationSec)
		    , perNodeKeyFrames(std::move(perNodeKeyFrames)) {}

		bool evaluateForNode(transf3d& outTransform, const int nodeIndex, const float time) const {
			auto itr = perNodeKeyFrames.find(nodeIndex);
			if (itr != perNodeKeyFrames.end()) {
				itr->second.evaluate(outTransform, time);
				return true;
			}
			return false;
		}

	  public:
		/// The name of the animation.
		std::string animationName;

		/// The lenght of the animation in seconds.. Keep in mind that there might be key frames beyond that value,
		/// The artist might need them for interpolation purposes.
		float durationSec = 0;

		/// The keyframes of all affected nodes in their local space (relative to their parents).
		std::map<int, KeyFrames> perNodeKeyFrames;
	};

	struct Node {
		/// int nodeIdx = -1; // The "id" used to identify this piece of data in the file.
		transf3d staticLocalTransform; ///< The local transformation of the node when not animated.

		std::vector<MeshAttachment> meshAttachments;
		std::vector<int> childNodes; ///< The indices of all child nodes.

		std::string name; ///< The name of the node.
	};

	//--------------------------------------------------------------
	// Model
	//--------------------------------------------------------------
	struct SGE_CORE_API Model {

		void createRenderingResources(SGEDevice& sgedev);

		int makeNewNode();
		int makeNewMaterial();
		int makeNewMesh();
		int makeNewAnim();

		int getRootNodeIndex() const { return m_rootNodeIndex; }
		Node* getRootNode() { return nodeAt(getRootNodeIndex()); }
		const Node* getRootNode() const { return nodeAt(getRootNodeIndex()); }

		int getNumNodes() const { return int(m_nodes.size()); }
		Node* nodeAt(int nodeIndex);
		const Node* nodeAt(int nodeIndex) const;
		int findFistNodeIndexWithName(const std::string& name) const;

		int getNumMaterials() const { return int(m_materials.size()); }
		Material* materialAt(int materialIndex);
		const Material* materialAt(int materialIndex) const;

		int getNumMeshes() const { return int(m_meshes.size()); }
		Mesh* getMeshByIndex(int meshIndex);
		const Mesh* getMeshByIndex(int meshIndex) const;

		int getNumAnimations() const { return int(m_animations.size()); }
		const Animation* getAnimation(int iAnim) const {
			if (iAnim >= 0 && iAnim < int(m_animations.size())) {
				return &m_animations[iAnim];
			}
			return nullptr;
		}
		Animation* getAnimation(int iAnim) {
			if (iAnim >= 0 && iAnim < int(m_animations.size())) {
				return &m_animations[iAnim];
			}
			return nullptr;
		}
		const Animation* getAnimationByName(const std::string& name) const;

		const std::vector<Node*>& getNodes() { return m_nodes; }
		const std::vector<Mesh*>& getMeshes() { return m_meshes; }
		const std::vector<Material*>& getMatrials() { return m_materials; }
		

	  private:
		int m_rootNodeIndex = -1;
		std::vector<Animation> m_animations;
		std::vector<Node*> m_nodes;
		std::vector<Mesh*> m_meshes;
		std::vector<Material*> m_materials;

		/// The actual storage for the model data.
		ChunkContainer<Mesh> m_containerMesh;
		ChunkContainer<Material> m_containerMaterial;
		ChunkContainer<Node> m_containerNode;

	  public:
		/// Stores the various collision shapes defineded in the model.
		/// They are useful for having one place for the rendering geometry and the
		/// geometry that is going to be used for game physics.
		std::vector<CollisionMesh> m_convexHulls;
		std::vector<CollisionMesh> m_concaveHulls;
		std::vector<CollisionShapeBox> m_collisionBoxes;
		std::vector<CollisionShapeCapsule> m_collisionCapsules;
		std::vector<CollisionShapeCylinder> m_collisionCylinders;
		std::vector<CollisionShapeSphere> m_collisionSpheres;

		/// Cached loading settings.
		LoadSettings m_loadSets;
	};
} // namespace Model

} // namespace sge
