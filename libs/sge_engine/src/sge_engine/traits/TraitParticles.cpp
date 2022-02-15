#include "sge_engine/traits/TraitParticles.h"
#include "sge_core/Camera.h"
#include "sge_engine/GameDrawer/RenderItems/TraitParticlesRenderItem.h"
#include "sge_engine/GameWorld.h"

namespace sge {

// clang-format off
ReflAddTypeId(TraitParticlesSimple, 20'03'02'0051);
ReflBlock() {
	ReflAddType(TraitParticlesSimple)
		ReflMember(TraitParticlesSimple, m_isEnabled)
		ReflMember(TraitParticlesSimple, m_isInWorldSpace)
		ReflMember(TraitParticlesSimple, m_pgroups)
	;
}

ReflAddTypeId(ParticleGroupDesc, 20'03'02'0049);
ReflAddTypeId(std::vector<ParticleGroupDesc>, 20'03'02'0050);

ReflAddTypeId(, 22'01'22'0001);

ReflBlock() {

	// Particle birth and shapes.
	ReflAddType(ParticleBirthCount)
		ReflMember(ParticleBirthCount, instantBirthCount).uiRange(0, 10000, 0.01f)
		ReflMember(ParticleBirthCount, continiousBirthCount).uiRange(0, 10000, 0.01f)
		ReflMember(ParticleBirthCount, continiousBirthDuration).uiRange(0.f, 1000.f, 0.01f)
		ReflMember(ParticleBirthCount, passiveBirthCountPerSecond).uiRange(0, 10000, 0.01f)
		ReflMember(ParticleBirthCount, maxLife).uiRange(0.f, 10000.f, 0.01f)
		ReflMember(ParticleBirthCount, maxLifeVariation).uiRange(0.f, 10000.f, 0.01f)
		ReflMember(ParticleBirthCount, sizeScale).uiRange(0.f, 10000.f, 0.01f)
		ReflMember(ParticleBirthCount, sizeScaleVariation).uiRange(0.f, 10000.f, 0.01f)
	;

	ReflAddType(SphereBirthShape)
		ReflMember(SphereBirthShape, sphereIsVolume)
		ReflMember(SphereBirthShape, sphereCenter)
		ReflMember(SphereBirthShape, sphereRadius).uiRange(0.f, 1000.f, 0.01f)
	;

	ReflAddType(PlaneBirthShape)
		ReflMember(PlaneBirthShape, planePosition)
		ReflMember(PlaneBirthShape, planeRotation)
		ReflMember(PlaneBirthShape, planeSize)
	;

	ReflAddType(PointBirthShape)
		ReflMember(PointBirthShape, pointLocation)
	;
	
	ReflAddType(LineBirthShape)
		ReflMember(LineBirthShape, lineFrom)
		ReflMember(LineBirthShape, lineTo)
	;

	ReflAddType(CircleBirthShape)
		ReflMember(CircleBirthShape, circleAsDisk)
		ReflMember(CircleBirthShape, circlePosition)
		ReflMember(CircleBirthShape, circleRadius).uiRange(0.f, 1000.f, 0.01f)
		ReflMember(CircleBirthShape, circleRotation)
	;

	ReflAddType(BirthShape)
		ReflEnumVal(birthShape_sphere, "Sphere")
		ReflEnumVal(birthShape_plane, "Plane")
		ReflEnumVal(birthShape_point, "Point")
		ReflEnumVal(birthShape_line, "Line")
		ReflEnumVal(birthShape_circle, "Circle/Disk")
	;

	ReflAddType(BirthShapeDesc)
		ReflMember(BirthShapeDesc, shapeType)
		ReflMember(BirthShapeDesc, sphere)
		ReflMember(BirthShapeDesc, plane)
		ReflMember(BirthShapeDesc, point)
		ReflMember(BirthShapeDesc, line)
		ReflMember(BirthShapeDesc, circle)
	;

	// Velocity
	ReflAddType(VelocityDirectionalVelocty)
		ReflMember(VelocityDirectionalVelocty, directon)
		ReflMember(VelocityDirectionalVelocty, velocityAmount).uiRange(-1000.f, 1000.f, 0.01f)
	;

	ReflAddType(VelocityTowardsPoint)
		ReflMember(VelocityTowardsPoint, pointLocation)
		ReflMember(VelocityTowardsPoint, velocityAmount).uiRange(-1000.f, 1000.f, 0.01f)
	;

	ReflAddType(VelocitySpherical)
		ReflMember(VelocitySpherical, sphereCenter)
		ReflMember(VelocitySpherical, velocityAmount).uiRange(-1000.f, 1000.f, 0.01f)
	;

	ReflAddType(VeloctyForce)
		ReflEnumVal(VelictyForce_none, "None")
		ReflEnumVal(VelictyForce_directional, "Directional")
		ReflEnumVal(VelictyForce_towardsPoint, "Point")
		ReflEnumVal(VelictyForce_spherical, "Spherical")
	;

	ReflAddType(Velocity)
		ReflMember(Velocity, forceType)
		ReflMember(Velocity, directional)
		ReflMember(Velocity, towardsPoint)
		ReflMember(Velocity, spherical)
	;
	
	ReflAddType(ParticlesAlpha)
		ReflMember(ParticlesAlpha, fadeInTimeAfterBirth)
		ReflMember(ParticlesAlpha, fadeOutTimeBeforeDeath)
	;

	ReflAddType(std::vector<Velocity>);

	ReflAddType(ParticleGroupDesc)
		ReflMember(ParticleGroupDesc, m_name)
		ReflMember(ParticleGroupDesc, simulationSpeed)
		ReflMember(ParticleGroupDesc, m_particlesSprite)
		ReflMember(ParticleGroupDesc, m_spriteGrid)
		ReflMember(ParticleGroupDesc, m_spriteFPS)
		ReflMember(ParticleGroupDesc, m_spritePixelsPerUnit)
		ReflMember(ParticleGroupDesc, birth)
		ReflMember(ParticleGroupDesc, birthShape)
		ReflMember(ParticleGroupDesc, initialVelocty)
		ReflMember(ParticleGroupDesc, velocityForces)
		ReflMember(ParticleGroupDesc, velocityDrag)
		ReflMember(ParticleGroupDesc, alpha)
	;

	ReflAddType(std::vector<ParticleGroupDesc>);
}
// clang-format on


/// Computes the random point on the surface of a sphere.
/// it is importatnt that u and v are picked uniformly.
/// @param [in] uRandom a number in range [0;1]
/// @param [in] vRandom a number in range [0;1]
/// @return the direction on the sphere.
vec3f getRandomOnSphereSurface(float uRandom, float vRandom)
{
	float theta = uRandom * two_pi();
	float phi = acosf(2.f * vRandom - 1.f);
	float r = 1.f;

	float sinTheta = sinf(theta);
	float cosTheta = cosf(theta);
	float sinPhi = sinf(phi);
	float cosPhi = cosf(phi);

	vec3f randomPoint;

	randomPoint.x = r * cosPhi;
	randomPoint.y = r * sinPhi * sinTheta;
	randomPoint.z = r * sinPhi * cosTheta;

	return randomPoint;
}

/// Computes the random point inside a sphere.
/// it is importatnt that u, v and radiusRandom are picked uniformly.
/// @param [in] uRandom a number in range [0;1]
/// @param [in] vRandom a number in range [0;1]
/// @param [in] radiusRandom a number in range [0;1]
/// @param [in] maxRadius is the radius of the sphere where the random points will be located.
/// @return the random point in the sphere.
vec3f getRandomInSphereVolume(float uRandom, float vRandom, float radiusRandom, float maxRadius)
{
	float theta = uRandom * two_pi();
	float phi = acosf(2.f * vRandom - 1.f);
	float r = std::max(1e-3f, cbrtf(radiusRandom) * maxRadius);

	float sinTheta = sinf(theta);
	float cosTheta = cosf(theta);
	float sinPhi = sinf(phi);
	float cosPhi = cosf(phi);

	vec3f randomPoint;

	randomPoint.x = r * cosPhi;
	randomPoint.y = r * sinPhi * sinTheta;
	randomPoint.z = r * sinPhi * cosTheta;

	return randomPoint;
}

//--------------------------------------------------------------
// ParticleGroupState
//--------------------------------------------------------------
vec3f computeAddedVelocity(Random& rnd, const vec3f& particlePosition, const Velocity& velDesc)
{
	vec3f velocity = vec3f(0.f);

	switch (velDesc.forceType) {
		case VelictyForce_none: {
		} break;
		case VelictyForce_directional: {
			// TODO: precalculate this.
			vec3f dir = velDesc.directional.directon.getDirection();

			velocity += dir * velDesc.directional.velocityAmount;
		} break;
		case VelictyForce_towardsPoint: {
			vec3f dir = normalized0(velDesc.towardsPoint.pointLocation - particlePosition);

			velocity += dir * velDesc.towardsPoint.velocityAmount;
		} break;
		case VelictyForce_spherical: {
			vec3f dir = particlePosition - velDesc.spherical.sphereCenter;
			float dirLenSqr = dir.lengthSqr();
			if (dirLenSqr < 1e-6f) {
				// The point is at the velocity source position.
				// pick a random point direction based on a sphere.
				getRandomOnSphereSurface(rnd.next01(), rnd.next01());
			}
			else {
				dir *= 1.f / sqrtf(dirLenSqr);
			}

			velocity = dir * velDesc.spherical.velocityAmount;

		} break;
		default:
			sgeAssert(false);
			break;
	}

	return velocity;
}

void ParticleGroupState::update(bool isInWorldSpace, const mat4f node2world, const ParticleGroupDesc& pgDesc, float dt)
{
	m_bboxFromLastUpdate = Box3f();
	m_isInWorldSpace = isInWorldSpace;
	m_n2w = node2world;

	dt *= pgDesc.simulationSpeed;

	const Optional<mat4f> spawnLocationMtx = m_isInWorldSpace ? Optional<mat4f>(node2world) : NullOptional();

	// Delete all dead particles.
	for (int t = 0; t < int(m_particles.size()); ++t) {
		if (m_particles[t].isDead()) {
			m_particles.erase(m_particles.begin() + t);
			t--;
			continue;
		}
	}

	// Spawn the new particles.
	int numParticlesToSpawn = 0;
	if (pgDesc.birth.passiveBirthCountPerSecond > 0) {
		m_passiveParticleBirthAccumulator += float(pgDesc.birth.passiveBirthCountPerSecond) * dt;

		if (m_passiveParticleBirthAccumulator >= 1.f) {
			numParticlesToSpawn += (int)floorf(m_passiveParticleBirthAccumulator);
			m_passiveParticleBirthAccumulator =
			    m_passiveParticleBirthAccumulator - floorf(m_passiveParticleBirthAccumulator);
		}
	}

	if (m_timeRunning == 0.f) {
		numParticlesToSpawn += pgDesc.birth.instantBirthCount;
	}

	if (pgDesc.birth.continiousBirthCount > 0 && pgDesc.birth.continiousBirthDuration > 0.f &&
	    pgDesc.birth.continiousBirthDuration <= m_timeRunning) {
		float particlesPerSecond = float(pgDesc.birth.continiousBirthCount) / pgDesc.birth.continiousBirthDuration;

		m_contiiousParticleBirthAccumulator += particlesPerSecond * dt;

		if (m_contiiousParticleBirthAccumulator >= 1.f) {
			numParticlesToSpawn += (int)floorf(m_contiiousParticleBirthAccumulator);
			m_contiiousParticleBirthAccumulator =
			    m_contiiousParticleBirthAccumulator - floorf(m_contiiousParticleBirthAccumulator);
		}
	}

	// To avoid needless allocations, preallocate.
	m_particles.reserve(m_particles.size() + numParticlesToSpawn);

	// Spawm the new particles and compute their position.
	for (int t = 0; t < numParticlesToSpawn; ++t) {
		vec3f spawnPos = vec3f(0.f);
		switch (pgDesc.birthShape.shapeType) {
			case birthShape_sphere: {
				float u = m_rnd.next01();
				float v = m_rnd.next01();
				if (pgDesc.birthShape.sphere.sphereIsVolume) {
					float rRnd = m_rnd.next01();
					spawnPos = getRandomInSphereVolume(u, v, rRnd, pgDesc.birthShape.sphere.sphereRadius);
				}
				else {
					spawnPos = getRandomOnSphereSurface(u, v) * pgDesc.birthShape.sphere.sphereRadius;
				}

				spawnPos += pgDesc.birthShape.sphere.sphereCenter;
			} break;
			case birthShape_plane: {
				sgeAssert(false);
			} break;
			case birthShape_point: {
				spawnPos = pgDesc.birthShape.point.pointLocation;
			} break;
			case birthShape_line: {
				spawnPos = lerp(pgDesc.birthShape.line.lineFrom, pgDesc.birthShape.line.lineTo, m_rnd.next01());
			} break;
			case birthShape_circle: {
				if (pgDesc.birthShape.circle.circleAsDisk) {
					float rho = std::max(1e-3f, sqrtf(m_rnd.next01()) * pgDesc.birthShape.circle.circleRadius);
					float theta = m_rnd.next01() * two_pi();
					spawnPos = vec3f(sin(theta), 0.f, cosf(theta)) * rho;
				}
				else {
					float angle = m_rnd.next01() * two_pi();
					spawnPos.x = pgDesc.birthShape.circle.circleRadius * cosf(angle);
					spawnPos.y = 0.f;
					spawnPos.z = pgDesc.birthShape.circle.circleRadius * sinf(angle);
				}

				spawnPos = pgDesc.birthShape.circle.circleRotation.transformPt(spawnPos);
				spawnPos += pgDesc.birthShape.circle.circlePosition;
			} break;
			default: {
				sgeAssert(false);
			}
		}

		// Apply the initial Velocity.
		vec3f initalVelocity = computeAddedVelocity(m_rnd, spawnPos, pgDesc.initialVelocty);

		// If the particle sumlation is in world space, convert the parties to world space.
		if (spawnLocationMtx.isValid()) {
			spawnPos = mat_mul_pos(spawnLocationMtx.get(), spawnPos);
			initalVelocity = mat_mul_dir(spawnLocationMtx.get(), initalVelocity);
		}

		// Compute the lifetime of the particle.
		float life = pgDesc.birth.maxLife;
		if (pgDesc.birth.maxLifeVariation > 0.f) {
			life += m_rnd.nextInRange(pgDesc.birth.maxLifeVariation);
		}

		// Compute the scale of the particles.
		float scale = pgDesc.birth.sizeScale;
		if (pgDesc.birth.sizeScaleVariation > 0.f) {
			scale += m_rnd.nextInRange(pgDesc.birth.sizeScaleVariation);
		}

		const ParticleState newParticle = {
		    spawnPos,
		    initalVelocity,
		    scale,
		    life,
		};

		m_particles.emplace_back(newParticle);
	}

	// Update the particles - their position, velocity, scale and alpha.
	for (ParticleState& particle : m_particles) {
		// Compute the opacity of the particle.
		particle.opacity = 1.f;

		// Fade-in after birth.
		if (pgDesc.alpha.fadeInTimeAfterBirth > 1e-3f &&
		    particle.m_timeSpendAlive < pgDesc.alpha.fadeInTimeAfterBirth) {
			particle.opacity *= sqr(clamp01(particle.m_timeSpendAlive / pgDesc.alpha.fadeInTimeAfterBirth));
		}

		// Fade-out before death.
		if (pgDesc.alpha.fadeOutTimeBeforeDeath > 1e-3f &&
		    (particle.m_timeSpendAlive > particle.m_maxLife - pgDesc.alpha.fadeOutTimeBeforeDeath)) {
			particle.opacity *=
			    sqr(clamp01((particle.m_maxLife - particle.m_timeSpendAlive) / pgDesc.alpha.fadeOutTimeBeforeDeath));
		}

		vec3f addedVelocity = vec3f(0.f);

		for (const Velocity& vel : pgDesc.velocityForces) {
			addedVelocity += computeAddedVelocity(m_rnd, particle.pos, vel);
		}

		if (spawnLocationMtx.isValid()) {
			addedVelocity = mat_mul_dir(spawnLocationMtx.get(), addedVelocity);
		}

		particle.velocity += addedVelocity;

		particle.velocity -= particle.velocity * pgDesc.velocityDrag * dt;

		particle.m_fSpriteIndex += dt * pgDesc.m_spriteFPS;

		// Apply the velocity and acommodate the time change.
		particle.pos += particle.velocity * dt;
		particle.m_timeSpendAlive += dt;
		m_bboxFromLastUpdate.expand(particle.pos);
	}

	m_timeRunning += dt;
}

ParticleGroupState::SpriteRendData* ParticleGroupState::computeSpriteRenderData(
    SGEContext& sgecon, const ParticleGroupDesc& pdesc, const ICamera& camera)
{
	struct Vertex {
		vec3f pos = vec3f(0.f);
		vec4f color = vec4f(1.f);
		vec3f normal = vec3f(1.f, 0.f, 0.f);
		vec2f uv = vec2f(0.f);
	};

	if (m_particles.empty()) {
		return nullptr;
	}

	// Sort the particles along the ray, so the generated vertex buffer have them sorted
	// so they could be blender correctly during rendering.
	vec3f const camPosWs = camera.getCameraPosition();
	vec3f const camLookWs = camera.getCameraLookDir();

	indicesForSorting.resize(m_particles.size());
	for (int t = 0; t < int(indicesForSorting.size()); ++t) {
		indicesForSorting[t].index = t;
		indicesForSorting[t].distanceAlongRay = projectPointOnLine(camPosWs, camLookWs, m_particles[t].pos);
	}

	std::sort(indicesForSorting.begin(), indicesForSorting.end(), [&](const SortingData& a, const SortingData& b) {
		return a.distanceAlongRay > b.distanceAlongRay;
	});

	// Obtain the sprite texture and check if it is valid.
	const AssetIface_Texture2D* texIface = pdesc.m_particlesSprite.getAssetInterface<AssetIface_Texture2D>();
	Texture* const sprite = texIface ? texIface->getTexture() : nullptr;
	if (sprite == nullptr) {
		spriteRenderData = NullOptional();
		return nullptr;
	}

	// Compute the sprite sub-images UV regions.
	const int numFrames = std::max(pdesc.m_spriteGrid.volume(), 0);
	if (numFrames != spriteFramesUVCache.size()) {
		float frmUWidth = 1.f / float(pdesc.m_spriteGrid.x);
		float frmVHeight = 1.f / float(pdesc.m_spriteGrid.y);

		spriteFramesUVCache.resize(numFrames);
		for (int iFrame = 0; iFrame < numFrames; ++iFrame) {
			int gridPosX = iFrame % pdesc.m_spriteGrid.x;
			int gridPosY = iFrame / pdesc.m_spriteGrid.x;

			Pair<vec2f, vec2f> uvRegion;
			uvRegion.first = vec2f(gridPosX * frmUWidth, gridPosY * frmVHeight);
			uvRegion.second = vec2f((gridPosX + 1.f) * frmUWidth, (gridPosY + 1.f) * frmVHeight);

			spriteFramesUVCache[iFrame] = uvRegion;
		}
	}

	// Obtain the sprite size in world space.
	const float particleWidthWs = sprite->getDesc().texture2D.width / pdesc.m_spritePixelsPerUnit;
	const float particleHeightWs = sprite->getDesc().texture2D.width / pdesc.m_spritePixelsPerUnit;

	const float hx = particleWidthWs * 0.5f;
	const float hy = particleHeightWs * 0.5f;

	std::vector<Vertex> vertices;
	vertices.reserve(int(m_particles.size()) * 6);

	mat4f faceCameraMtx = camera.getView();
	faceCameraMtx.c3 = vec4f(0.f, 0.f, 0.f, 1.f); // kill the translation.
	faceCameraMtx = inverse(faceCameraMtx);       // TODO: Optimize.

	// Generate the vertices so they are oriented towards the camera.
	for (const SortingData& sortedData : indicesForSorting) {
		const ParticleState& p = m_particles[sortedData.index];
		// Compute the transformation that will make the particle face the camera.
		vec3f particlePosRaw = p.pos;
		vec3f particlePosTransformed = (!m_isInWorldSpace) ? m_n2w.transfPos(particlePosRaw) : particlePosRaw;

		mat4f orientationMtx = mat4f::getTranslation(particlePosTransformed) * faceCameraMtx;

		vec2f uv0 = vec2f(0.f);
		vec2f uv1 = vec2f(1.f);
		int frmIndex = int(p.m_fSpriteIndex) % int(spriteFramesUVCache.size());
		if (frmIndex >= 0 && frmIndex < spriteFramesUVCache.size()) {
			uv0 = spriteFramesUVCache[frmIndex].first;
			uv1 = spriteFramesUVCache[frmIndex].second;
		}

		// A template vertices represeting a quad, centered at (0,0,0) with correct size,
		// later we are going to transform these vertices to get the final transform
		// for the vertex buffer.

		vec4f particleColor = vec4f(1.f, 1.f, 1.f, p.opacity);

		const Vertex templateVetex[6] = {
		    Vertex{vec3f(-hx, -hy, 0.f), particleColor, vec3f::getAxis(2), uv0},
		    Vertex{vec3f(+hx, -hy, 0.f), particleColor, vec3f::getAxis(2), vec2f(uv1.x, uv0.y)},
		    Vertex{vec3f(+hx, +hy, 0.f), particleColor, vec3f::getAxis(2), vec2f(uv1.x, uv1.y)},

		    Vertex{vec3f(-hx, -hy, 0.f), particleColor, vec3f::getAxis(2), vec2f(uv0.x, uv0.y)},
		    Vertex{vec3f(+hx, +hy, 0.f), particleColor, vec3f::getAxis(2), uv1},
		    Vertex{vec3f(-hx, +hy, 0.f), particleColor, vec3f::getAxis(2), vec2f(uv0.x, uv1.y)},
		};

		for (const Vertex& templateVtx : templateVetex) {
			Vertex vtx = templateVtx;
			vtx.pos *= fabsf(p.scale) * m_n2w.extractUnsignedScalingVector();
			vtx.pos = mat_mul_pos(orientationMtx, vtx.pos);
			vtx.normal = mat_mul_dir(orientationMtx, vtx.normal); // TODO: inverse transpose.
			vertices.push_back(vtx);
		}
	}

	// Generate the Geometry data.
	if (spriteRenderData.isValid() == false) {
		spriteRenderData = SpriteRendData();
	}

	const int strideSizeBytes = sizeof(vertices[0]);
	const int neededVtxBufferByteSize = int(vertices.size()) * strideSizeBytes;

	const bool isVtxBufferCorrectSize = spriteRenderData->vertexBuffer.IsResourceValid() &&
	                                    spriteRenderData->vertexBuffer->getDesc().sizeBytes == neededVtxBufferByteSize;

	if (isVtxBufferCorrectSize == false) {
		BufferDesc const vbDesc = BufferDesc::GetDefaultVertexBuffer(neededVtxBufferByteSize, ResourceUsage::Dynamic);
		spriteRenderData->vertexBuffer = sgecon.getDevice()->requestResource<Buffer>();
		spriteRenderData->vertexBuffer->create(vbDesc, vertices.data());
	}
	else {
		// The vertex buffer is large enough, just update it's contents.
		void* mappedMem = sgecon.map(spriteRenderData->vertexBuffer, Map::WriteDiscard);
		if (mappedMem) {
			memcpy(mappedMem, vertices.data(), neededVtxBufferByteSize);
		}
		sgecon.unMap(spriteRenderData->vertexBuffer);
	}

	VertexDecl vertexDecl[4] = {
	    VertexDecl(0, "a_position", UniformType::Float3, 0),
	    VertexDecl(0, "a_color", UniformType::Float4, 3 * sizeof(float)),
	    VertexDecl(0, "a_normal", UniformType::Float3, 7 * sizeof(float)),
	    VertexDecl(0, "a_uv", UniformType::Float2, 10 * sizeof(float)),
	};

	VertexDeclIndex vertexDeclIdx = sgecon.getDevice()->getVertexDeclIndex(vertexDecl, SGE_ARRSZ(vertexDecl));

	spriteRenderData->geometry = Geometry(
	    spriteRenderData->vertexBuffer,
	    nullptr,
	    nullptr,
	    -1,
	    vertexDeclIdx,
	    true,
	    true,
	    true,
	    false,
	    PrimitiveTopology::TriangleList,
	    0,
	    0,
	    strideSizeBytes,
	    UniformType::Unknown,
	    int(vertices.size()));

	spriteRenderData->material.diffuseColorSrc = DefaultPBRMtlData::diffuseColorSource_diffuseMap;
	spriteRenderData->material.diffuseTexture = sprite;
	spriteRenderData->material.tintByVertexColor = true;
	spriteRenderData->material.forceNoLighting = true;
	return &spriteRenderData.get();
}

//--------------------------------------------------------------
// TraitParticlesSimple
//--------------------------------------------------------------
void TraitParticlesSimple::update(const GameUpdateSets& u)
{
	bool simulateBecauseWeArePlaying = u.isSimulationPaused() && !m_isEnabled;
	bool simulateBecauseWeAreEditing = m_isPreviewPlaying;
	if (!simulateBecauseWeArePlaying && !simulateBecauseWeAreEditing) {
		return;
	}

	for (ParticleGroupDesc& desc : m_pgroups) {
		desc.m_particlesSprite.update();

		// TODO: handle duplicated names!
		m_pgroupState[desc.m_name].update(m_isInWorldSpace, getActor()->getTransformMtx(), desc, u.dt);
	}
}

Box3f TraitParticlesSimple::getBBoxOS() const
{
	if (m_isInWorldSpace) {
		const mat4f ownerWorld2Object = inverse(getActor()->getTransformMtx());
		Box3f bbox;
		for (auto& pair : m_pgroupState) {
			bbox.expand(pair.second.getBBox().getTransformed(ownerWorld2Object));
		}
		return bbox;
	}
	else {
		Box3f bbox;
		for (auto& pair : m_pgroupState) {
			bbox.expand(pair.second.getBBox());
		}
		return bbox;
	}
}

void TraitParticlesSimple::getRenderItems(std::vector<TraitParticlesSimpleRenderItem>& renderItems)
{
	TraitParticlesSimpleRenderItem renderItem;

	renderItem.traitParticles = this;

	// TODO: Is this the best alpha sorting point that we can come up with?
	// TODO: We do not really always need alpha sorting for particles. Add some logic to determine if we need it.
	renderItem.needsAlphaSorting = true;
	renderItem.zSortingPositionWs = mat_mul_pos(getActor()->getTransformMtx(), getBBoxOS().center());

	renderItems.push_back(renderItem);
}

//--------------------------------------------------------------
// TraitParticlesProgrammable
//--------------------------------------------------------------
void TraitParticlesProgrammable::getRenderItems(std::vector<TraitParticlesProgrammableRenderItem>& renderItems)
{
	// TODO: add separate render item for each group.
	TraitParticlesProgrammableRenderItem renderItem;

	renderItem.traitParticles = this;

	// TODO: Is this the best alpha sorting point that we can come up with?
	// TODO: We do not really always need alpha sorting for particles. Add some logic to determine if we need it.
	renderItem.needsAlphaSorting = false;

	// renderItem.zSortingPositionWs = mat_mul_pos(getActor()->getTransformMtx(), getBBoxOS().center());
	renderItems.push_back(renderItem);
}

//--------------------------------------------------------------
// ParticleRenderDataGen
//--------------------------------------------------------------
ReflAddTypeId(TraitParticlesProgrammable, 20'11'23'0001);
bool ParticleRenderDataGen::generate(
    const TraitParticlesProgrammable::ParticleGroup& particles,
    SGEContext& sgecon,
    const ICamera& camera,
    const mat4f& n2w)
{
	if (particles.allParticles.empty()) {
		return false;
	}

	// Sort the particles along the ray, so the generated vertex buffer have them sorted
	// so they could be blender correctly during rendering.
	{
		vec3f const camPosWs = camera.getCameraPosition();
		vec3f const camLookWs = camera.getCameraLookDir();

		indicesForSorting.resize(particles.allParticles.size());
		for (int t = 0; t < int(indicesForSorting.size()); ++t) {
			indicesForSorting[t].index = t;
			indicesForSorting[t].distanceAlongRay =
			    projectPointOnLine(camPosWs, camLookWs, particles.allParticles[t].position);
		}

		std::sort(indicesForSorting.begin(), indicesForSorting.end(), [&](const SortingData& a, const SortingData& b) {
			return a.distanceAlongRay > b.distanceAlongRay;
		});
	}

	// Obtain the sprite texture and check if it is valid.
	Texture* const sprite = getLoadedAssetIface<AssetIface_Texture2D>(particles.spriteTexture)->getTexture();

	// Compute the sprite sub-images UV regions.
	const int numFrames = std::max(particles.spriteFramsCount.volume(), 0);
	if (numFrames != spriteFramesUVCache.size()) {
		float frmUWidth = 1.f / float(particles.spriteFramsCount.x);
		float frmVHeight = 1.f / float(particles.spriteFramsCount.y);

		spriteFramesUVCache.resize(numFrames);
		for (int iFrame = 0; iFrame < numFrames; ++iFrame) {
			int gridPosX = iFrame % particles.spriteFramsCount.x;
			int gridPosY = iFrame / particles.spriteFramsCount.x;

			Pair<vec2f, vec2f> uvRegion;
			uvRegion.first = vec2f(gridPosX * frmUWidth, gridPosY * frmVHeight);
			uvRegion.second = vec2f((gridPosX + 1.f) * frmUWidth, (gridPosY + 1.f) * frmVHeight);

			spriteFramesUVCache[iFrame] = uvRegion;
		}
	}

	// Obtain the sprite size in world space.
	const float particleWidthWs = 1.f;  // sprite->getDesc().texture2D.width / pdesc.m_spritePixelsPerUnit;
	const float particleHeightWs = 1.f; // sprite->getDesc().texture2D.width / pdesc.m_spritePixelsPerUnit;

	const float hx = particleWidthWs * 0.5f;
	const float hy = particleHeightWs * 0.5f;

	std::vector<ParticleVertexData> vertices;
	vertices.reserve(int(particles.allParticles.size()) * 6);

	mat4f faceCameraMtx = camera.getView();
	faceCameraMtx.c3 = vec4f(0.f, 0.f, 0.f, 1.f); // kill the translation.
	faceCameraMtx = inverse(faceCameraMtx);       // TODO: Optimize.

	// Generate the vertices so they are oriented towards the camera.
	for (const SortingData& sortedData : indicesForSorting) {
		const TraitParticlesProgrammable::ParticleGroup::ParticleData& p = particles.allParticles[sortedData.index];
		// Compute the transformation that will make the particle face the camera.
		vec3f particlePosRaw = p.position;
		vec3f particlePosTransformed = (!particles.isInWorldSpace) ? n2w.transfPos(particlePosRaw) : particlePosRaw;

		mat4f orientationMtx = mat4f::getTranslation(particlePosTransformed) * faceCameraMtx;

		vec2f uv0 = vec2f(0.f);
		vec2f uv1 = vec2f(1.f);
		int frmIndex = int(p.frameIndex) % int(spriteFramesUVCache.size());
		if (frmIndex >= 0 && frmIndex < spriteFramesUVCache.size()) {
			uv0 = spriteFramesUVCache[frmIndex].first;
			uv1 = spriteFramesUVCache[frmIndex].second;
		}

		// A template vertices represeting a quad, centered at (0,0,0) with correct size,
		// later we are going to transform these vertices to get the final transform
		// for the vertex buffer.
		const ParticleVertexData templateVetex[6] = {
		    ParticleVertexData{vec3f(-hx, -hy, 0.f), vec3f::getAxis(2), p.tint, uv0},
		    ParticleVertexData{vec3f(+hx, -hy, 0.f), vec3f::getAxis(2), p.tint, vec2f(uv1.x, uv0.y)},
		    ParticleVertexData{vec3f(+hx, +hy, 0.f), vec3f::getAxis(2), p.tint, vec2f(uv1.x, uv1.y)},

		    ParticleVertexData{vec3f(-hx, -hy, 0.f), vec3f::getAxis(2), p.tint, vec2f(uv0.x, uv0.y)},
		    ParticleVertexData{vec3f(+hx, +hy, 0.f), vec3f::getAxis(2), p.tint, uv1},
		    ParticleVertexData{vec3f(-hx, +hy, 0.f), vec3f::getAxis(2), p.tint, vec2f(uv0.x, uv1.y)},
		};

		for (const ParticleVertexData& templateVtx : templateVetex) {
			ParticleVertexData vtx = templateVtx;
			vtx.a_position *= fabsf(p.scale) * n2w.extractUnsignedScalingVector();
			vtx.a_position = mat_mul_pos(orientationMtx, vtx.a_position);
			vtx.a_normal = mat_mul_dir(orientationMtx, vtx.a_normal); // TODO: inverse transpose.
			vertices.push_back(vtx);
		}
	}

	const int strideSizeBytes = sizeof(vertices[0]);
	const int neededVtxBufferByteSize = int(vertices.size()) * strideSizeBytes;

	const bool isVtxBufferCorrectSize =
	    vertexBuffer.IsResourceValid() && vertexBuffer->getDesc().sizeBytes == neededVtxBufferByteSize;

	if (isVtxBufferCorrectSize == false) {
		BufferDesc const vbDesc = BufferDesc::GetDefaultVertexBuffer(neededVtxBufferByteSize, ResourceUsage::Dynamic);
		vertexBuffer = sgecon.getDevice()->requestResource<Buffer>();
		vertexBuffer->create(vbDesc, vertices.data());
	}
	else {
		// The vertex buffer is large enough, just update it's contents.
		void* mappedMem = sgecon.map(vertexBuffer, Map::WriteDiscard);
		if (mappedMem) {
			memcpy(mappedMem, vertices.data(), neededVtxBufferByteSize);
		}
		sgecon.unMap(vertexBuffer);
	}

	VertexDecl vertexDecl[4] = {
	    VertexDecl(0, "a_position", UniformType::Float3, 0),
	    VertexDecl(0, "a_normal", UniformType::Float3, -1),
	    VertexDecl(0, "a_color", UniformType::Float4, -1),
	    VertexDecl(0, "a_uv", UniformType::Float2, -1),
	};

	VertexDeclIndex vertexDeclIdx = sgecon.getDevice()->getVertexDeclIndex(vertexDecl, SGE_ARRSZ(vertexDecl));

	geometry = Geometry(
	    vertexBuffer,
	    nullptr,
	    nullptr,
	    -1,
	    vertexDeclIdx,
	    false,
	    true,
	    true,
	    false,
	    PrimitiveTopology::TriangleList,
	    0,
	    0,
	    strideSizeBytes,
	    UniformType::Unknown,
	    int(vertices.size()));

	material.diffuseTexture = sprite;

	return true;
}

} // namespace sge
