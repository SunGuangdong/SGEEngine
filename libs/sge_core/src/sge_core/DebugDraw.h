#pragma once

#include "QuickDraw.h"
#include "sgecore_api.h"
#include <unordered_map>

namespace sge {

struct SGE_CORE_API DebugDraw {
	struct SGE_CORE_API WiredCommandData {
		void clear(bool deallocMem)
		{
			m_verts.clear();
			if (deallocMem)
				m_verts = decltype(m_verts)();
		}

		void line(const vec3f& a, const vec3f& b, const int rgba);
		void box(const mat4f& world, const int rgba);
		void box(const Box3f& aabb, const int rgba);
		void box(const mat4f& world, const Box3f& aabb, const int rgba);
		void sphere(const mat4f& world, const int rgba, float radius, int numSides = 3);
		void capsule(const mat4f& world, const int rgba, float height, float radius, int numSides = 3);
		void cylinder(const mat4f& world, const int rgba, float height, float radius, int numSides = 3);
		void basis(const mat4f& world);
		void grid(
		    const vec3f& origin, const vec3f& xAxis, const vec3f& zAxis, const int xLines, const int zLines, const int color = 0x000000);


		const std::vector<GeomGen::PosColorVert>& getVerts() const { return m_verts; }

	  public:
		std::vector<GeomGen::PosColorVert> m_verts;
	};

	struct Group {
		Group() = default;

		void clear(bool deallocMem) { m_wieredData.clear(deallocMem); }

		const char* getName() const { return m_name.c_str(); }
		WiredCommandData& getWiered() { return m_wieredData; }

	  private:
		std::string m_name;
		WiredCommandData m_wieredData;
	};

  public:
	void initialze(SGEDevice* sgedev);

	void draw(const RenderDestination& rdest, const mat4f& projView);
	Group& getGroup(const char* name) { return m_groups[name]; }

  private:
	void drawWieredCommand(const RenderDestination& rdest, const mat4f& projView, const WiredCommandData& cmd);

  private:
	std::unordered_map<std::string, Group> m_groups;

	GpuHandle<Buffer> m_vertexBuffer;

	bool isInitialized = false;
	GpuHandle<ShadingProgram> m_shaderSolidVertexColor;
	int m_projViewWorld_strIdx = 0;
	VertexDeclIndex m_vertexDeclIndex_pos3d_rgba_int;

	StateGroup m_stateGroup;
};

} // namespace sge