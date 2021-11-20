#include "sge_utils/math/Box.h"
#include "sge_utils/math/Rangef.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/math/transform.h"

#include "sge_utils/math/MultiCurve2D.h"

#include <string>
#include <vector>

#include "typeLib.h"

namespace sge {

// clang-format off
RelfAddTypeId(bool,                       1);
RelfAddTypeId(char,                       2);
RelfAddTypeId(int,                        3);
RelfAddTypeId(unsigned,                   4);
RelfAddTypeId(float,                      5);
RelfAddTypeId(double,                     6);

//RelfAddTypeId(std::vector<bool>,        7);
RelfAddTypeId(std::vector<char>,          8);
RelfAddTypeId(std::vector<int>,           9);
RelfAddTypeId(std::vector<unsigned>,     10);
RelfAddTypeId(std::vector<float>,        11);
RelfAddTypeId(std::vector<double>,       12);

RelfAddTypeId(std::string,               13);

RelfAddTypeId(vec2i,                     14);
RelfAddTypeId(vec3i,                     15);
RelfAddTypeId(vec4i,                     16);

RelfAddTypeId(vec2f,                     17);
RelfAddTypeId(vec3f,                     18);
RelfAddTypeId(vec4f,                     19);
RelfAddTypeId(quatf,                     20);

RelfAddTypeId(mat4f,                     21);
RelfAddTypeId(transf3d,                  22);

RelfAddTypeId(AABox3f,                   23);

RelfAddTypeId(Rangef,                    24);

RelfAddTypeId(MultiCurve2D::PointType,                25);
RelfAddTypeId(MultiCurve2D::Point,                    26);
RelfAddTypeId(std::vector<MultiCurve2D::Point>,       27);
RelfAddTypeId(MultiCurve2D,                           28);

RelfAddTypeId(std::vector<vec2i>,                     29);
RelfAddTypeId(std::vector<vec3i>,                     30);
RelfAddTypeId(std::vector<vec4i>,                     31);

RelfAddTypeId(std::vector<vec2f>,                     32);
RelfAddTypeId(std::vector<vec3f>,                     33);
RelfAddTypeId(std::vector<vec4f>,                     34);
RelfAddTypeId(std::vector<quatf>,                     35);

RelfAddTypeId(Axis,                                   40);

RelfAddTypeId(std::vector<transf3d>,                  41);

RelfAddTypeId(short,                                  42);
RelfAddTypeId(unsigned short,                         43);

RelfAddTypeId(TypeId,                                 44);
RelfAddTypeId(std::vector<TypeId>,                    45);

ReflBlock() {
	ReflAddType(bool);
	ReflAddType(char);
	ReflAddType(int);
	ReflAddType(unsigned);
	ReflAddType(float);
	ReflAddType(double);

	//ReflAddType(std::vector<bool>);
	ReflAddType(std::vector<char>);
	ReflAddType(std::vector<int>);
	ReflAddType(std::vector<unsigned>);
	ReflAddType(std::vector<float>);
	ReflAddType(std::vector<double>);

	ReflAddType(std::string);

	// vec*i
	ReflAddType(vec2i)
		ReflMember(vec2i, x)
		ReflMember(vec2i, y);
	ReflAddType(vec3i)
		ReflMember(vec3i, x)
		ReflMember(vec3i, y)
		ReflMember(vec3i, z);
	ReflAddType(vec4i)
		ReflMember(vec4i, x)
		ReflMember(vec4i, y)
		ReflMember(vec4i, z)
		ReflMember(vec4i, w);

	// vec*f
	ReflAddType(vec2f)
		ReflMember(vec2f, x)
		ReflMember(vec2f, y);
	ReflAddType(vec3f)
		ReflMember(vec3f, x)
		ReflMember(vec3f, y)
		ReflMember(vec3f, z);
	ReflAddType(vec4f)
		ReflMember(vec4f, x)
		ReflMember(vec4f, y)
		ReflMember(vec4f, z)
		ReflMember(vec4f, w);

	ReflAddType(quatf)
		ReflMember(quatf, x)
		ReflMember(quatf, y)
		ReflMember(quatf, z)
		ReflMember(quatf, w);
	
	ReflAddType(std::vector<vec2i>);
	ReflAddType(std::vector<vec3i>);
	ReflAddType(std::vector<vec4i>);
	
	ReflAddType(std::vector<vec2f>);
	ReflAddType(std::vector<vec3f>);
	ReflAddType(std::vector<vec4f>);
	ReflAddType(std::vector<quatf>);

	ReflAddType(mat4f)
		ReflMember(mat4f, c0)
		ReflMember(mat4f, c1)
		ReflMember(mat4f, c2)
		ReflMember(mat4f, c3);

	ReflAddType(transf3d)
		ReflMember(transf3d, p).setPrettyName("position")
		ReflMember(transf3d, r).setPrettyName("rotation")
		ReflMember(transf3d, s).setPrettyName("scaling")
	;

	ReflAddType(AABox3f)
		ReflMember(AABox3f, min)
		ReflMember(AABox3f, max);

	ReflAddType(Rangef)
		ReflMember(Rangef, min)
		ReflMember(Rangef, max);

		// MultiCurve2D
	ReflAddType(MultiCurve2D::PointType)
		ReflEnumVal(MultiCurve2D::pointType_linear, "Linear")
		ReflEnumVal(MultiCurve2D::pointType_constant, "Constant")
		ReflEnumVal(MultiCurve2D::pointType_smooth, "Smooth")
		ReflEnumVal(MultiCurve2D::pointType_bezierKey, "Bezier")
		ReflEnumVal(MultiCurve2D::pointType_bezierHandle0, "BezierHandle0")
		ReflEnumVal(MultiCurve2D::pointType_bezierHandle1, "BezierHandle1")
	;

	ReflAddType(MultiCurve2D::Point)
		ReflMember(MultiCurve2D::Point, type)
		ReflMember(MultiCurve2D::Point, x)
		ReflMember(MultiCurve2D::Point, y)
	;

	ReflAddType(std::vector<MultiCurve2D::Point>);

	ReflAddType(MultiCurve2D)
		ReflMember(MultiCurve2D, m_pointsWs)
	;

	ReflAddType(Axis)
		ReflEnumVal(axis_x, "axisIdx_x")
		ReflEnumVal(axis_y, "axisIdx_y")
		ReflEnumVal(axis_z, "axisIdx_z")
	;

	ReflAddType(std::vector<transf3d>);

	ReflAddType(short);
	ReflAddType(unsigned short);

#if 1
	ReflAddType(TypeId)
		ReflMember(TypeId, id);
#else
	ReflAddType(TypeId)
		ReflMember(TypeId, hash)
		ReflMember(TypeId, name)
#endif
	;

	ReflAddType(std::vector<TypeId>);
}

// clang-format on

} // namespace sge
