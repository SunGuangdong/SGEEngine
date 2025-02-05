#include "AssimpImporter.h"
#include "IAssetRelocationPolicy.h"
#include "ImporterCommon.h"
#include "sge_utils/containers/Range.h"
#include "sge_utils/containers/vector_set.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/text/format.h"

#include <set>
#include <stdexcept>

namespace sge {

vec4f fromAssimp(const aiColor3D& c, float a)
{
	return vec4f(c.r, c.g, c.b, a);
}

vec3f fromAssimp(const aiVector3D& v)
{
	return vec3f(v.x, v.y, v.z);
}

quatf fromAssimp(const aiQuaternion& q)
{
	return quatf(q.x, q.y, q.z, q.w);
}

transf3d fromAssimp(const aiMatrix4x4& m)
{
	transf3d result;

	aiVector3D asmpTranslation;
	aiVector3D asmpScaling;
	aiQuaternion asmpRotation;

	m.Decompose(asmpScaling, asmpRotation, asmpTranslation);

	result = transf3d(fromAssimp(asmpTranslation), fromAssimp(asmpRotation), fromAssimp(asmpScaling));
	return result;
}

Box3f fromAssimp(const aiAABB& asmpBox)
{
	Box3f box;

	box.min = fromAssimp(asmpBox.mMin);
	box.max = fromAssimp(asmpBox.mMax);

	return box;
}

ModelCollisionMesh assimpMeshToCollisionMesh(const aiMesh* const asmpMesh)
{
	// Extract all vertices form all polygons and then remove the duplicate vertices and generate indices.
	const int numPolygons = asmpMesh->mNumFaces;
	const int numVerts = numPolygons * 3;

	std::vector<vec3f> trianglesVeticesWithDuplicated(asmpMesh->mNumFaces * 3);

	for (int const iPoly : RangeInt(asmpMesh->mNumFaces)) {
		for (int const iVertex : RangeInt(3)) {
			if (asmpMesh->mFaces[iPoly].mNumIndices != 3) {
				throw ImportExcept("assimpMeshToCollisionMesh Expected that all faces are triangles!");
			}

			int vertexIndex = asmpMesh->mFaces[iPoly].mIndices[iVertex];
			vec3f vertexPos = fromAssimp(asmpMesh->mVertices[vertexIndex]);

			int const globalVertexIndex = iPoly * 3 + iVertex;

			trianglesVeticesWithDuplicated[globalVertexIndex] = vertexPos;
		}
	}

	// Remove the duplicates and create an index buffer.
	std::vector<vec3f> trianglesVetices;
	std::vector<int> trianglesIndices;

	for (int iVertex = 0; iVertex < trianglesVeticesWithDuplicated.size(); iVertex++) {
		int foundIndex = -1;
		for (int t = 0; t < trianglesVetices.size(); ++t) {
			if (trianglesVetices[t] == trianglesVeticesWithDuplicated[iVertex]) {
				foundIndex = t;
				break;
			}
		}

		if (foundIndex == -1) {
			trianglesVetices.push_back(trianglesVeticesWithDuplicated[iVertex]);
			foundIndex = int(trianglesVetices.size()) - 1;
		}

		trianglesIndices.push_back(foundIndex);
	}

	sgeAssert(trianglesIndices.size() % 3 == 0);

	return ModelCollisionMesh{std::move(trianglesVetices), std::move(trianglesIndices)};
}

// Assimp supports GLTF embedded textures.
// When we ask a material for a texture assigned to it,
// Assimp might return to us a real filename, or it can return some strnage looking string like "*0",
// witch means that there is probably an embedded texture and the so far returned filename is not a real one.
// If that is the case, we need to extract that emedded texture generate a filename for it and then
// return it as a real texture name.
//
// @param [in] asmpScene the assimp scene being imported.
// @param [in] asmpMtlTexName is the name returned bvy aiMaterial::Get method.
// @return the filename to be referenced in the material for this texture.
std::string extractTextureFilenameAndEmbeddedTex(
    ModelImportAdditionalResult* additionalResult, const aiScene* asmpScene, const aiString& asmpMtlTexName)
{
	const aiTexture* embeddedTexture = asmpScene->GetEmbeddedTexture(asmpMtlTexName.C_Str());
	if (embeddedTexture) {
		// Might seem strange, but according to the documentation in
		// aiTexture::pcData comment:
		//
		// If aiTexture::mHeight is 0, then the aiTexture holds whole file data (a png, jpeg and so on),
		// the buffer in pcData is of size mWidth (bytes I assume) and it is just a dump of that file contents.
		// So basically if we just create a file with the specified memory we will end up with an image file.
		if (embeddedTexture->mHeight == 0) {
			std::string suggestedFilename =
			    std::string(embeddedTexture->mFilename.C_Str()) + "." + embeddedTexture->achFormatHint;

			// Check if the texture has already been marked for creation.
			if (additionalResult->textureToCopy.count(suggestedFilename) == 0) {
				std::vector<char> fileContents;
				fileContents.resize(embeddedTexture->mWidth);
				memcpy(fileContents.data(), (char*)embeddedTexture->pcData, fileContents.size());
				additionalResult->texturesToCreate[suggestedFilename] = {std::move(fileContents)};
			}

			return suggestedFilename;
		}
		else {
			throw ImportExcept("Unsupported embedded texture format.");
		}
	}
	else {
		// No ebedded texture the provided name seems to be a real texture filename.
		return std::string(asmpMtlTexName.C_Str());
	}
}

//----------------------------------------------------------------------------
// AssimpImporter
//----------------------------------------------------------------------------
bool AssimpImporter::parse(
    Model* result,
    ModelImportAdditionalResult& additionalResult,
    std::string& materialsPrefix,
    const aiScene* scene,
    aiNode* enforcedRootNode,
    const ModelParseSettings& parseSettings)
{
	try {
		m_model = result;
		m_parseSettings = parseSettings;
		asmpScene = scene;
		m_additionalResult = &additionalResult;
		m_materialsPrefix = materialsPrefix;

		// Discover all the nodes that need to be parsed. Allocate them and store their hierarchy.
		// This step also dicovers all meshes and materials that need to be imported.
		const aiNode* const rootNodeToBeUsed = enforcedRootNode != nullptr ? enforcedRootNode : asmpScene->mRootNode;
		const int importedRootNodeIndex = discoverNodesRecursive(rootNodeToBeUsed);

		m_model->setRootNodeIndex(importedRootNodeIndex);

		importMaterials();
		importMeshes(enforcedRootNode == nullptr);
		importNodes();
		// Importing animations when we only import a sub-tree of the fbx scene isn't supported yet.
		if (enforcedRootNode == nullptr) {
			importAnimations();
		}

		// When importing a sub-tree reset the transformation of the root node,
		// we want the sub tree to be centered.
		// However we keep the rotation and scaling, as the exporters might do axis conversion.
		if (enforcedRootNode != nullptr) {
			m_model->getRootNode()->staticLocalTransform.p = vec3f(0.f);

			// Since we are going to reposition the root node location, we aloso need to reposition
			// the collision geometries.
			aiVector3D asmpTranslation;
			aiVector3D asmpScaling;
			aiQuaternion asmpRotation;

			enforcedRootNode->mTransformation.Decompose(asmpScaling, asmpRotation, asmpTranslation);
			asmpScaling = aiVector3D(1.f, 1.f, 1.f);
			asmpRotation = aiQuaternion(1.f, 0.f, 0.f, 0.f); // Assimp quats are in w xyz order.

			// The colision geometry needs to be shifted as well.
			m_collision_transfromCorrection = transf3d(-fromAssimp(asmpTranslation));
		}

		importCollisionGeometry();

		return true;
	}
	catch ([[maybe_unused]] const std::exception& except) {
		return false;
	}
}

int AssimpImporter::discoverNodesRecursive(const aiNode* const asmpNode)
{
	const int nodeIndex = m_model->makeNewNode();
	ModelNode* node = m_model->nodeAt(nodeIndex);
	m_asmpNode2NodeIndex[asmpNode] = nodeIndex;

	node->name = asmpNode->mName.C_Str();

	// [SGE_COLISION_GEOM_DUPLICATE]
	// Check if the geometry attached to this node should
	// be used for as collsion geometry and not for rendering.
	// This is guessed by the prefix in the node name.
	bool const isCollisionGeometryTriMeshNode =
	    node->name.find("SCConcave_") == 0 || node->name.find("SCTriMesh_") == 0;
	bool const isCollisionGeometryConvexNode = node->name.find("SCConvex_") == 0;
	bool const isCollisionGeometryBoxNode = node->name.find("SCBox_") == 0;
	bool const isCollisionGeometryCapsuleNode = node->name.find("SCCapsule_") == 0;
	bool const isCollisionGeometryCylinderNode = node->name.find("SCCylinder_") == 0;
	bool const isCollisionGeometrySphereNode = node->name.find("SCSphere_") == 0;

	// clang-format off
	bool const isCollisionGeometryNode = 
			isCollisionGeometryTriMeshNode 
		|| isCollisionGeometryConvexNode 
		|| isCollisionGeometryBoxNode 
		|| isCollisionGeometryCapsuleNode 
		|| isCollisionGeometryCylinderNode 
		|| isCollisionGeometrySphereNode;
	// clang-format on

	// TODO: meshes for rendering and meshes for collision.
	for (int iMesh = 0; iMesh < (int)asmpNode->mNumMeshes; ++iMesh) {
		unsigned int assimpMeshIndex = asmpNode->mMeshes[iMesh];
		const aiMesh* asmpMesh = asmpScene->mMeshes[assimpMeshIndex];

		if (isCollisionGeometryNode) {
			// The mesh is going to be used for collision.

			const transf3d collisionPosModelSpace = fromAssimp(asmpNode->mTransformation);

			if (isCollisionGeometryConvexNode) {
				m_collision_ConvexHullMeshes[asmpMesh].push_back(collisionPosModelSpace);
			}
			else if (isCollisionGeometryTriMeshNode) {
				m_collision_BvhTriMeshes[asmpMesh].push_back(collisionPosModelSpace);
			}
			else if (isCollisionGeometryBoxNode) {
				m_collision_BoxMeshes[asmpMesh].push_back(collisionPosModelSpace);
			}
			else if (isCollisionGeometryCapsuleNode) {
				m_collision_CaplsuleMeshes[asmpMesh].push_back(collisionPosModelSpace);
			}
			else if (isCollisionGeometryCylinderNode) {
				m_collision_CylinderMeshes[asmpMesh].push_back(collisionPosModelSpace);
			}
			else if (isCollisionGeometrySphereNode) {
				m_collision_SphereMeshes[asmpMesh].push_back(collisionPosModelSpace);
			}
			else {
				sgeAssert(false);
			}
		}
		else {
			// The mesh is going to be used for rendering.
			if (m_asmpMesh2MeshIndex.count(assimpMeshIndex) == 0) {
				const int meshIndex = m_model->makeNewMesh();
				m_asmpMesh2MeshIndex[assimpMeshIndex] = meshIndex;
			}

			if (m_asmpMtl2MtlIndex.count(asmpMesh->mMaterialIndex) == 0) {
				int materialIndex = m_model->makeNewMaterial();
				m_asmpMtl2MtlIndex[asmpMesh->mMaterialIndex] = materialIndex;
			}
		}
	}

	// Parse the child nodes and store the hierarchy.
	for (int iChild = 0; iChild < (int)asmpNode->mNumChildren; ++iChild) {
		const aiNode* asmpChild = asmpNode->mChildren[iChild];

		node->childNodes.push_back(discoverNodesRecursive(asmpChild));
	}

	return nodeIndex;
}

void AssimpImporter::importMaterials()
{
	for (auto& pair : m_asmpMtl2MtlIndex) {
		const aiMaterial* asmpMtl = asmpScene->mMaterials[pair.first];

		aiString asmpMtlName;
		asmpMtl->Get(AI_MATKEY_NAME, asmpMtlName);

		// TODO material asset names
		std::string materialAssetName = m_materialsPrefix + "_" + std::string(asmpMtlName.C_Str()) + ".mtl";

		ModelMaterial* const material = m_model->materialAt(pair.second);

		material->name = asmpMtlName.C_Str();
		material->assetForThisMaterial = materialAssetName;

		// Check if the material is already parsed (in the case of importing single scene as multiple 3d models).
		// if so do not import it again.
		if (m_additionalResult->mtlsToCreate.count(materialAssetName)) {
			continue;
		}

		aiColor3D asmpDiffuseColor = aiColor3D(1.f, 1.f, 1.f);
		aiColor3D asmpEmissiveColor = aiColor3D(0.f, 0.f, 0.f);
		float metallic = 0.f;
		float roughness = 1.f;
		float emissionItensity = 0.f;

		asmpMtl->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
		asmpMtl->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
		asmpMtl->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissionItensity);

		if (asmpMtl->Get(AI_MATKEY_BASE_COLOR, asmpDiffuseColor) != aiReturn_SUCCESS) {
			if (asmpMtl->Get(AI_MATKEY_COLOR_DIFFUSE, asmpDiffuseColor) != aiReturn_SUCCESS) {
				asmpDiffuseColor = aiColor3D(1.f, 1.f, 1.f);
			}
		}

		aiString asmpBaseColorTexName;
		aiString asmpEmissionTexName;
		aiString asmpMetallicTexName;
		aiString asmpRoughnessTexName;

		aiString normalMapTexName;

		if (asmpMtl->GetTexture(aiTextureType_BASE_COLOR, 0, &asmpBaseColorTexName) != aiReturn_SUCCESS) {
			if (asmpMtl->GetTexture(aiTextureType_DIFFUSE, 0, &asmpBaseColorTexName) != aiReturn_SUCCESS) {
				asmpBaseColorTexName = aiString();
			}
		}

		asmpMtl->GetTexture(aiTextureType_METALNESS, 0, &asmpMetallicTexName);
		asmpMtl->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &asmpRoughnessTexName);
		asmpMtl->GetTexture(aiTextureType_EMISSIVE, 0, &asmpRoughnessTexName);
		asmpMtl->GetTexture(aiTextureType_NORMALS, 0, &normalMapTexName);

		ExternalPBRMaterialSettings importedMtlSets;

		importedMtlSets.diffuseTextureName =
		    extractTextureFilenameAndEmbeddedTex(m_additionalResult, asmpScene, asmpBaseColorTexName);
		importedMtlSets.emissionTextureName =
		    extractTextureFilenameAndEmbeddedTex(m_additionalResult, asmpScene, asmpEmissionTexName);
		importedMtlSets.metallicTextureName =
		    extractTextureFilenameAndEmbeddedTex(m_additionalResult, asmpScene, asmpMetallicTexName);
		importedMtlSets.roughnessTextureName =
		    extractTextureFilenameAndEmbeddedTex(m_additionalResult, asmpScene, asmpRoughnessTexName);
		importedMtlSets.normalTextureName =
		    extractTextureFilenameAndEmbeddedTex(m_additionalResult, asmpScene, normalMapTexName);

		importedMtlSets.diffuseColor = fromAssimp(asmpDiffuseColor, 1.f);
		importedMtlSets.emissionColor = fromAssimp(asmpDiffuseColor, 0.f);
		importedMtlSets.metallic = metallic;
		importedMtlSets.roughness = roughness;

		// Finally add the material to the list of the imported materials.
		m_additionalResult->mtlsToCreate[materialAssetName] = importedMtlSets;

		// Mark the textures to get copied.
		if (importedMtlSets.diffuseTextureName.empty() == false)
			m_additionalResult->textureToCopy.insert(importedMtlSets.diffuseTextureName);
		if (importedMtlSets.emissionTextureName.empty() == false)
			m_additionalResult->textureToCopy.insert(importedMtlSets.emissionTextureName);
		if (importedMtlSets.metallicTextureName.empty() == false)
			m_additionalResult->textureToCopy.insert(importedMtlSets.metallicTextureName);
		if (importedMtlSets.roughnessTextureName.empty() == false)
			m_additionalResult->textureToCopy.insert(importedMtlSets.roughnessTextureName);
		if (importedMtlSets.normalTextureName.empty() == false)
			m_additionalResult->textureToCopy.insert(importedMtlSets.normalTextureName);
	}
}

void AssimpImporter::importMeshes(const bool importSkinningData)
{
	for (auto& pair : m_asmpMesh2MeshIndex) {
		int asmpMeshIndex = pair.first;
		int meshIndex = pair.second;
		importMeshes_singleMesh(asmpMeshIndex, meshIndex, importSkinningData);
	}
}

void AssimpImporter::importMeshes_singleMesh(
    unsigned asimpMeshIndex, int importedMeshIndex, const bool importSkinningData)
{
	const aiMesh* asmpMesh = asmpScene->mMeshes[asimpMeshIndex];

	std::vector<void*> asmpChannelsData;
	std::vector<int> asmpChannelByteStep;

	ModelMesh& mesh = *m_model->meshAt(importedMeshIndex);
	mesh.name = asmpMesh->mName.C_Str();

	// Remember, we are assuming that the meshes are already triangulated.
	// Compute the vertex layout.
	int stride = 0;

	// Vertex positions.
	{
		VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_position";
		decl.format = sge::UniformType::Float3;
		decl.byteOffset = -1;

		mesh.vertexDecl.push_back(decl);

		stride += 12;

		asmpChannelsData.push_back(asmpMesh->mVertices);
		asmpChannelByteStep.push_back(sizeof(asmpMesh->mVertices[0]));
	}

	// Color.
	int color0ByteOffset = -1;
	if (asmpMesh->GetNumColorChannels() > 0) {
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_color";
		decl.format = sge::UniformType::Float4;
		decl.byteOffset = -1;

		color0ByteOffset = stride;
		stride += 16;
		mesh.vertexDecl.push_back(decl);

		asmpChannelsData.push_back(asmpMesh->mColors[0]);
		asmpChannelByteStep.push_back(sizeof(asmpMesh->mColors[0]));
	}

	// Normals.
	int normals0ByteOffset = -1;
	if (asmpMesh->HasNormals()) {
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_normal";
		decl.format = sge::UniformType::Float3;
		decl.byteOffset = -1;

		normals0ByteOffset = stride;
		stride += 12;
		mesh.vertexDecl.push_back(decl);

		asmpChannelsData.push_back(asmpMesh->mNormals);
		asmpChannelByteStep.push_back(sizeof(asmpMesh->mNormals[0]));
	}

	// Tangents and bi-tangents
	int tangents0ByteOffset = -1;
	int binormals0ByteOffset = -1;
	if (asmpMesh->HasTangentsAndBitangents()) {
		{
			sge::VertexDecl decl;
			decl.bufferSlot = 0;
			decl.semantic = "a_tangent";
			decl.format = sge::UniformType::Float3;
			decl.byteOffset = -1;

			tangents0ByteOffset = stride;
			stride += 12;
			mesh.vertexDecl.push_back(decl);

			asmpChannelsData.push_back(asmpMesh->mTangents);
			asmpChannelByteStep.push_back(sizeof(asmpMesh->mTangents[0]));
		}

		{
			sge::VertexDecl decl;
			decl.bufferSlot = 0;
			decl.semantic = "a_binormal";
			decl.format = sge::UniformType::Float3;
			decl.byteOffset = -1;

			binormals0ByteOffset = stride;
			stride += 12;
			mesh.vertexDecl.push_back(decl);

			asmpChannelsData.push_back(asmpMesh->mBitangents);
			asmpChannelByteStep.push_back(sizeof(asmpMesh->mBitangents[0]));
		}
	}

	// UVs
	int UV0ByteOffset = -1;
	if (asmpMesh->HasTextureCoords(0)) {
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_uv";
		decl.format = sge::UniformType::Float2;
		decl.byteOffset = -1;

		UV0ByteOffset = stride;
		stride += 8;
		mesh.vertexDecl.push_back(decl);

		asmpChannelsData.push_back(asmpMesh->mTextureCoords[0]);
		asmpChannelByteStep.push_back(sizeof(asmpMesh->mTextureCoords[0][0]));
	}

	struct BoneInfluence {
		BoneInfluence() = default;
		BoneInfluence(int boneIndex, float boneWeight)
		    : boneIndex(boneIndex)
		    , boneWeight(boneWeight)
		{
		}

		int boneIndex = -1;
		float boneWeight = 0.f;
	};

	std::map<int, std::vector<BoneInfluence>> perVertexIdBoneInfuence;

	// Find if there is a skinning data and if so, load the bones for mesh skinning (skeletal animation).
	if (importSkinningData && asmpMesh->HasBones()) {
		mesh.bones.reserve(asmpMesh->mNumBones);
		for (int iBone = 0; iBone < (int)asmpMesh->mNumBones; ++iBone) {
			const aiBone* asmpBone = asmpMesh->mBones[iBone];

			// Find the node that represents the bone. The animation on that node
			// will represent the movement of the bone.
			const auto& itrFindBoneNode = m_asmpNode2NodeIndex.find(asmpBone->mNode);
			if (itrFindBoneNode == m_asmpNode2NodeIndex.end()) {
				// Assimp fails to create find the nodes for some bones.
				// 1st found on some DAE files.
				throw ImportExcept("Found a bone that doesn't have node associated with it.");
			}

			const int boneNodeIndex = itrFindBoneNode->second;

			// Get the offset matrix.
			// Additionally transpose the matrix as Assimp is using row major matrices.
			mat4f boneOffseMtx = mat4f::getIdentity();
			static_assert(sizeof(boneOffseMtx) == sizeof(asmpBone->mOffsetMatrix));
			memcpy(&boneOffseMtx, &asmpBone->mOffsetMatrix, sizeof(boneOffseMtx));
			boneOffseMtx = boneOffseMtx.transposed();

			// Read the affected vertices.
			for (const int iWeight : RangeInt(asmpBone->mNumWeights)) {
				const aiVertexWeight& asmpWeight = asmpBone->mWeights[iWeight];
				perVertexIdBoneInfuence[asmpWeight.mVertexId].push_back(BoneInfluence(iBone, asmpWeight.mWeight));
			}

			// Finally Add the bone to the mesh.
			mesh.bones.push_back(ModelMeshBone(boneOffseMtx, boneNodeIndex));
		}

		// Hack:
		// Usually in most 3D models all verices have bones that infuence them
		// and usually they sum up to one.
		//
		// However... from Sketchfab I found a mesh that had a few vertices that did not
		// have any bones infuencing them.
		// In that case, I guess, the correct solution would be to use the transformation
		// of the node where the mesh is attached for that vertex.
		// This however would introduce more special cases in the code to be handled.
		// (maybe this is what I should do).
		//
		// In order to avoid that, if the mesh has any bones,
		// for each vertex that is not infuenced by anything,
		// we find the closest bone to it and add 100% weight to it
		// that way the vertex is going to get transformed.
		// This is probably produce unwanted motion but it should be minimal.
		//
		// The same is duplicated in FBX SDK importer.
		bool hasVertsWithNoBonesAttached = perVertexIdBoneInfuence.size() != asmpMesh->mNumVertices;

		if (asmpMesh->mNumBones > 0 && hasVertsWithNoBonesAttached) {
			printf(
			    "Found a skinned mesh with vertices that have no bones infuencing them. A workaround soultion will "
			    "take place.");
			for (int iVert = 0; iVert < (int)asmpMesh->mNumVertices; ++iVert) {
				if (perVertexIdBoneInfuence.count(iVert) == 0) {
					// The vertex doesn't have any bones infuencing it.
					// find the closest bone to the vertex and pick that one to have 100% infuence.
					int iBestBone = -1;
					float bestClosestDistanceSqr = FLT_MAX;
					for (int iBone = 0; iBone < (int)asmpMesh->mNumBones; ++iBone) {
						const aiBone* asmpBone = asmpMesh->mBones[iBone];

						const aiVector3D vertexPos = asmpMesh->mVertices[iVert];
						const aiVector3D vertexPosRelToBone = asmpBone->mOffsetMatrix * vertexPos;

						const aiVector3D bonePosition = aiVector3D(
						    asmpBone->mNode->mTransformation[0][3],
						    asmpBone->mNode->mTransformation[1][3],
						    asmpBone->mNode->mTransformation[2][3]);

						float currDistanceSqr = (bonePosition - vertexPosRelToBone).SquareLength();
						if (currDistanceSqr < bestClosestDistanceSqr) {
							iBestBone = iBone;
							bestClosestDistanceSqr = currDistanceSqr;
						}
					}

					// Append the workaround infuence.
					if (iBestBone >= 0) {
						perVertexIdBoneInfuence[iVert].push_back(BoneInfluence(iBestBone, 1.f));
					}
					else {
						throw ImportExcept("Coundn't find a bone to auto assign for a skinned mesh.");
					}
				}
			}
		}
	}

	// If there are bones in the mesh. Create the vertex attributres for them.
	int boneIdsByteOffset = -1;
	int boneWeightsByteOffset = -1;
	if (!perVertexIdBoneInfuence.empty()) {
		for (auto& pair : perVertexIdBoneInfuence) {
			// Some control point might be infuenced by more then kMaxBonesPerVertex,
			// if reduce the bones that amount and renormalize the weigths so they sum to 1.
			// In addition Assimp likes to add all bones that do not infuence with 0 weight (with GLTF) for some reason.
			if (pair.second.size() > kMaxBonesPerVertex) {
				// Sort by weigths.
				for (int i = 0; i < pair.second.size(); ++i) {
					for (int j = i + 1; j < pair.second.size(); ++j) {
						if (pair.second[j].boneWeight > pair.second[i].boneWeight) {
							std::swap(pair.second[j], pair.second[i]);
						}
					}
				}

				// Cut off the lear contributing ifluences beyond kMaxBonesPerVertex.
				pair.second.resize(kMaxBonesPerVertex);
			}

			// Copied from the previously existing FBX importer, but the note sounds reasonable to get copied:
			//
			// Believe it or not some vertices had the sum of
			// all weights exceed 1 (the Jammo robot form Mix and Jam youtube channel).
			// I'm not sure if the way we read the bones forcontrol points is wrong but this seemed necessery.
			float weigthSum = 0.f;
			for (int t = 0; t < pair.second.size(); ++t) {
				weigthSum += pair.second[t].boneWeight;
			}

			if (weigthSum > 1e-6f && weigthSum != 1.f) {
				float invSum = 1.f / weigthSum;
				for (int t = 0; t < pair.second.size(); ++t) {
					pair.second[t].boneWeight = pair.second[t].boneWeight * invSum;
				}
			}
		}

		{
			sge::VertexDecl decl;
			decl.bufferSlot = 0;
			decl.semantic = "a_bonesIds";
			decl.format = sge::UniformType::Int4;
			decl.byteOffset = -1;

			boneIdsByteOffset = stride;
			stride += 16;
			mesh.vertexDecl.push_back(decl);
		}

		{
			sge::VertexDecl decl;
			decl.bufferSlot = 0;
			decl.semantic = "a_bonesWeights";
			decl.format = sge::UniformType::Float4;
			decl.byteOffset = -1;

			boneWeightsByteOffset = stride;
			stride += 16;
			mesh.vertexDecl.push_back(decl);
		}
	}

	mesh.primitiveTopology = PrimitiveTopology::TriangleList;
	mesh.vertexDecl = sge::VertexDecl::NormalizeDecl(mesh.vertexDecl.data(), int(mesh.vertexDecl.size()));

	std::vector<char> vertexBufferData(asmpMesh->mNumVertices * stride);

	// Fill the vertex buffer data channels.
	Box3f bboxMesh;
	for (unsigned declIdx = 0; declIdx < mesh.vertexDecl.size(); ++declIdx) {
		if (declIdx >= asmpChannelsData.size()) {
			continue;
		}
		if (declIdx >= asmpChannelByteStep.size()) {
			continue;
		}

		for (unsigned vertIdx = 0; vertIdx < asmpMesh->mNumVertices; ++vertIdx) {
			// Insert the vertex data into the buffers.
			const VertexDecl& vertexDecl = mesh.vertexDecl[declIdx];
			const size_t writeOffset =
			    stride * vertIdx + vertexDecl.byteOffset; // The data offset location form the begining of our buffer.

			// Copy the data to the result buffer.
			char* destLoc = vertexBufferData.data() + writeOffset;
			const char* readLoc = (char*)asmpChannelsData[declIdx] + asmpChannelByteStep[declIdx] * vertIdx;
			const size_t bytes = UniformType::GetSizeBytes(vertexDecl.format);

			memcpy(destLoc, readLoc, bytes);

			// Accumulate the position data into AABox.
			if (0 == declIdx) {
				bboxMesh.expand(*(vec3f*)(readLoc));
			}
		}
	}

	// Fill the bones data channels.
	if (boneIdsByteOffset >= 0 && boneWeightsByteOffset >= 0 && !perVertexIdBoneInfuence.empty()) {
		for (const auto& pair : perVertexIdBoneInfuence) {
			int vertIdx = pair.first;

			// The data offset location form the begining of our buffer.
			const size_t writeOffsetBoneId = stride * vertIdx + boneIdsByteOffset;
			const size_t writeOffsetBoneWeight = stride * vertIdx + boneWeightsByteOffset;

			int* boneIdsWriteLoc = (int*)(vertexBufferData.data() + writeOffsetBoneId);
			float* boneWeightsWriteLoc = (float*)(vertexBufferData.data() + writeOffsetBoneWeight);

			boneIdsWriteLoc[0] = 0;
			boneIdsWriteLoc[1] = 0;
			boneIdsWriteLoc[2] = 0;
			boneIdsWriteLoc[3] = 0;

			boneWeightsWriteLoc[0] = 0.f;
			boneWeightsWriteLoc[1] = 0.f;
			boneWeightsWriteLoc[2] = 0.f;
			boneWeightsWriteLoc[3] = 0.f;

			sgeAssert(pair.second.size() <= kMaxBonesPerVertex);
			for (int iInfuence = 0; iInfuence < pair.second.size(); ++iInfuence) {
				boneIdsWriteLoc[iInfuence] = pair.second[iInfuence].boneIndex;
				boneWeightsWriteLoc[iInfuence] = pair.second[iInfuence].boneWeight;
			}
		}
	}

	// Read the faces and create the index buffer.
	bool isIndexBuferAllSequental = true;
	unsigned ibSeqCnt = 0;
	const uint32 numIndices = asmpMesh->mNumFaces * 3;

	std::vector<char> indicesData;
	indicesData.resize(numIndices * sizeof(uint32));

	for (int iFace = 0; iFace < (int)asmpMesh->mNumFaces; ++iFace) {
		const aiFace& face = asmpMesh->mFaces[iFace];

		if (face.mNumIndices != 3) {
			throw ImportExcept("Expected that all faces are triangles!");
		}

		// clang-format off
		isIndexBuferAllSequental &=
		    (face.mIndices[0] == ibSeqCnt) 
			&& (face.mIndices[1] == face.mIndices[0] + 1) 
			&& (face.mIndices[2] == face.mIndices[1] + 1);

		ibSeqCnt += 3;
		// clang-format on

		((uint32*)(indicesData.data() + sizeof(uint32) * 3 * iFace))[0] = (face.mIndices[0]);
		((uint32*)(indicesData.data() + sizeof(uint32) * 3 * iFace))[1] = (face.mIndices[1]);
		((uint32*)(indicesData.data() + sizeof(uint32) * 3 * iFace))[2] = (face.mIndices[2]);
	}

	mesh.ibFmt = UniformType::Uint;
	sgeAssert(mesh.ibFmt == UniformType::Uint);
	mesh.numElements = (int)(indicesData.size() / sizeof(uint32));
	mesh.numVertices = int(vertexBufferData.size()) / stride;
	mesh.ibByteOffset = 0;
	mesh.indexBufferRaw = std::move(indicesData);
	mesh.vbByteOffset = 0;
	mesh.vertexBufferRaw = std::move(vertexBufferData);
	mesh.aabox = bboxMesh;

	mesh.vbVertexColorOffsetBytes = color0ByteOffset;
	mesh.vbPositionOffsetBytes = 0;
	mesh.vbNormalOffsetBytes = normals0ByteOffset;
	mesh.vbTangetOffsetBytes = tangents0ByteOffset;
	mesh.vbBinormalOffsetBytes = binormals0ByteOffset;
	mesh.vbUVOffsetBytes = UV0ByteOffset;
	mesh.vbBonesIdsBytesOffset = boneIdsByteOffset;
	mesh.vbBonesWeightsByteOffset = boneWeightsByteOffset;
}

void AssimpImporter::importNodes()
{
	for (const auto& pair : m_asmpNode2NodeIndex) {
		const aiNode* asmpNode = pair.first;
		importNodes_singleNode(asmpNode, pair.second);
	}
}

void AssimpImporter::importNodes_singleNode(const aiNode* asmpNode, int importNodeIndex)
{
	ModelNode* node = m_model->nodeAt(importNodeIndex);

	node->name = asmpNode->mName.C_Str();

	aiVector3D asmpTranslation;
	aiVector3D asmpScaling;
	aiQuaternion asmpRotation;

	asmpNode->mTransformation.Decompose(asmpScaling, asmpRotation, asmpTranslation);

	node->staticLocalTransform.p = fromAssimp(asmpTranslation);
	node->staticLocalTransform.r = fromAssimp(asmpRotation);
	node->staticLocalTransform.s = fromAssimp(asmpScaling);

	for (int iMesh = 0; iMesh < (int)asmpNode->mNumMeshes; ++iMesh) {
		const unsigned asmpMeshIndex = asmpNode->mMeshes[iMesh];
		const aiMesh* asmpMesh = asmpScene->mMeshes[asmpMeshIndex];


		int meshIndex = m_asmpMesh2MeshIndex[asmpMeshIndex];
		int materialIndex = m_asmpMtl2MtlIndex[asmpMesh->mMaterialIndex];

		node->meshAttachments.push_back(MeshAttachment(meshIndex, materialIndex));
	}
}

void AssimpImporter::importAnimations()
{
	for (int iAnim = 0; iAnim < (int)asmpScene->mNumAnimations; ++iAnim) {
		const aiAnimation* const asmpAnim = asmpScene->mAnimations[iAnim];

		const float animationDuration = (float)(asmpAnim->mDuration / asmpAnim->mTicksPerSecond);

		std::string animationName = asmpAnim->mName.C_Str();
		if (animationName.empty()) {
			animationName = string_format("AutoAnimName_%d", iAnim);
		}

		// Per node keyframes for this animation. They keyframes are in node local space.
		std::map<int, KeyFrames> perNodeKeyFrames;

		for (int iAnimCh = 0; iAnimCh < (int)asmpAnim->mNumChannels; ++iAnimCh) {
			const aiNodeAnim* const asmpNodeAnim = asmpAnim->mChannels[iAnimCh];
			const std::string nodeName = asmpNodeAnim->mNodeName.C_Str();

			KeyFrames nodeKeyFrames;

			const int nodeIndex = m_model->findFistNodeIndexWithName(nodeName);
			if (nodeIndex < 0) {
				throw ImportExcept(
				    "Animated node not found by name. Assimp promises that the names in assimp scene and in aiNodeAnim "
				    "will match but "
				    "something is wrong somewhere!");
			}

			// Translation.
			for (int t = 0; t < (int)asmpNodeAnim->mNumPositionKeys; ++t) {
				const aiVectorKey& key = asmpNodeAnim->mPositionKeys[t];

				float keyTime = (float)(key.mTime / asmpAnim->mTicksPerSecond);
				nodeKeyFrames.positionKeyFrames[keyTime] = fromAssimp(key.mValue);
			}

			// Rotation.
			for (int t = 0; t < (int)asmpNodeAnim->mNumRotationKeys; ++t) {
				const aiQuatKey& key = asmpNodeAnim->mRotationKeys[t];

				float keyTime = (float)(key.mTime / asmpAnim->mTicksPerSecond);
				nodeKeyFrames.rotationKeyFrames[keyTime] = fromAssimp(key.mValue);
			}

			// Scaling
			for (int t = 0; t < (int)asmpNodeAnim->mNumScalingKeys; ++t) {
				const aiVectorKey& key = asmpNodeAnim->mScalingKeys[t];

				float keyTime = (float)(key.mTime / asmpAnim->mTicksPerSecond);
				nodeKeyFrames.scalingKeyFrames[keyTime] = fromAssimp(key.mValue);
			}

			// Save the keyframes to the animation.
			// if there are no key frames, skip the node.
			if (nodeKeyFrames.hasAnyKeyFrames()) {
				perNodeKeyFrames[nodeIndex] = std::move(nodeKeyFrames);
			}
		}

		// Add the animation to the model if it is not empty (no key frames).
		// FBX exporters tend to do this for some reason.
		if (!perNodeKeyFrames.empty()) {
			const int newAnimIndex = m_model->makeNewAnim();
			*(m_model->animationAt(newAnimIndex)) =
			    ModelAnimation(animationName, animationDuration, std::move(perNodeKeyFrames));
		}
	}
}

void AssimpImporter::importCollisionGeometry()
{
	// Convex hulls.
	for (const auto& itrFbxMeshInstantiations : m_collision_ConvexHullMeshes) {
		const aiMesh* const fbxMesh = itrFbxMeshInstantiations.first;

		ModelCollisionMesh collisionMeshObjectSpace = assimpMeshToCollisionMesh(fbxMesh);

		if (collisionMeshObjectSpace.indices.size() % 3 == 0) {
			// For every instance transform the vertices to model (or in this context world space).
			for (int const iInstance : RangeInt(int(itrFbxMeshInstantiations.second.size()))) {
				std::vector<vec3f> verticesWS = collisionMeshObjectSpace.vertices;

				mat4f const n2w =
				    m_collision_transfromCorrection.toMatrix() * itrFbxMeshInstantiations.second[iInstance].toMatrix();
				for (vec3f& v : verticesWS) {
					v = mat_mul_pos(n2w, v);
				}

				m_model->m_convexHulls.emplace_back(
				    ModelCollisionMesh{std::move(verticesWS), collisionMeshObjectSpace.indices});
			}
		}
		else {
			printf("Invalid collision geometry");
		}
	}

	// Triangle meshes.
	for (const auto& itrFbxMeshInstantiations : m_collision_BvhTriMeshes) {
		const aiMesh* const asmpMesh = itrFbxMeshInstantiations.first;

		ModelCollisionMesh collisionMeshObjectSpace = assimpMeshToCollisionMesh(asmpMesh);

		if (collisionMeshObjectSpace.indices.size() % 3 == 0) {
			// For every instance transform the vertices to model (or in this context world space).
			for (int const iInstance : RangeInt(int(itrFbxMeshInstantiations.second.size()))) {
				std::vector<vec3f> verticesWS = collisionMeshObjectSpace.vertices;

				mat4f const n2w =
				    m_collision_transfromCorrection.toMatrix() * itrFbxMeshInstantiations.second[iInstance].toMatrix();
				for (vec3f& v : verticesWS) {
					v = mat_mul_pos(n2w, v);
				}

				m_model->m_concaveHulls.emplace_back(
				    ModelCollisionMesh{std::move(verticesWS), collisionMeshObjectSpace.indices});
			}
		}
		else {
			printf("Invalid collision geometry");
		}
	}

	// Boxes
	for (const auto& itrBoxInstantiations : m_collision_BoxMeshes) {
		const aiMesh* const asmpMesh = itrBoxInstantiations.first;

		// CAUTION: The code assumes that the mesh vertices form a box.
		Box3f bbox = fromAssimp(asmpMesh->mAABB);

		for (int const iInstance : RangeInt(int(itrBoxInstantiations.second.size()))) {
			transf3d n2w = m_collision_transfromCorrection * itrBoxInstantiations.second[iInstance];
			n2w.p += bbox.center();
			m_model->m_collisionBoxes.push_back(Model_CollisionShapeBox{"", n2w, bbox.halfDiagonal()});
		}
	}

	// Capsules.
	for (const auto& itr : m_collision_CaplsuleMeshes) {
		const aiMesh* const asmpMesh = itr.first;

		// CAUTION: The code assumes that the mesh vertices are untoched.
		Box3f bbox = fromAssimp(asmpMesh->mAABB);

		vec3f const halfDiagonal = bbox.halfDiagonal();
		vec3f const ssides = halfDiagonal.getSorted();
		float halfHeight = ssides[0];
		float const radius = maxOf(ssides[1], ssides[2]);

		if_checked(2.f * radius <= halfHeight) { printf("ERROR: Invalid capsule buonding box.\n"); }

		halfHeight -= radius;

		for (int const iInstance : RangeInt(int(itr.second.size()))) {
			transf3d n2w = m_collision_transfromCorrection * itr.second[iInstance];
			n2w.p += bbox.center();
			m_model->m_collisionCapsules.push_back(Model_CollisionShapeCapsule{"", n2w, halfHeight, radius});
		}
	}

	// Cylinders.
	for (const auto& itrCylinderInstantiations : m_collision_CylinderMeshes) {
		const aiMesh* const asmpMesh = itrCylinderInstantiations.first;

		// CAUTION: The code assumes that the mesh vertices are untoched.
		Box3f bbox = fromAssimp(asmpMesh->mAABB);

		vec3f const halfDiagonal = bbox.halfDiagonal();
		for (int const iInstance : RangeInt(int(itrCylinderInstantiations.second.size()))) {
			transf3d const n2w = m_collision_transfromCorrection * itrCylinderInstantiations.second[iInstance];

			m_model->m_collisionCylinders.push_back(Model_CollisionShapeCylinder{"", n2w, halfDiagonal});
		}
	}

	// Spheres.
	for (const auto& itrSphereInstantiations : m_collision_SphereMeshes) {
		const aiMesh* const asmpMesh = itrSphereInstantiations.first;

		// CAUTION: The code assumes that the mesh vertices are untoched.
		Box3f bbox = fromAssimp(asmpMesh->mAABB);

		vec3f const halfDiagonal = bbox.halfDiagonal();
		float const radius = halfDiagonal.getSorted().x;
		for (int const iInstance : RangeInt(int(itrSphereInstantiations.second.size()))) {
			transf3d const n2w = m_collision_transfromCorrection * itrSphereInstantiations.second[iInstance];

			m_model->m_collisionSpheres.push_back(Model_CollisionShapeSphere{"", n2w, radius});
		}
	}
}

} // namespace sge
