#pragma once

#include <map>
#include <string.h>
#include <string>

#include "sge_core/sgecore_api.h"
#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/StaticArray.h"
#include <type_traits>

#include "sge_utils/math/quat.h"
#include "sge_utils/math/vec4.h"
#include "sge_utils/utils/vector_map.h"

namespace sge {

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
struct SGE_CORE_API ParameterType {
	enum Enum {
		Float,
		Float2,
		Float3,
		Float4,
		Quaternion,
		String,

		NumParams,

		// Flags that are not releated to parameter type.
		DontCreate,      // Used as a default argument in ParameterBlock::FindParameter.
		FromStringError, // Used as an error code when converging String to ParameterType::Enum in FromString.
	};

	struct Info {
		const char* name;
		int sizeBytes;
	};

	static const Info& info(const Enum e) {
		static const Info info[] = {
		    {"Float", 4}, {"Float2", 8}, {"Float3", 12}, {"Float4", 16}, {"Quaternion", 16}, {"String", 0},
		};

		return info[(int)e];
	}

	static Enum FromString(const char* const str) {
		for (int t = 0; t < NumParams; ++t) {
			if (strcmp(info(Enum(t)).name, str) == 0)
				return (Enum)t;
		}
		return FromStringError;
	}

	static int SizeBytes(const Enum e) { return info(e).sizeBytes; }

	virtual void operator()() = 0;
};

struct SGE_CORE_API ParameterCurve {
	ParameterType::Enum type;
	std::vector<float> keys;
	std::vector<char> data;

  public:
	ParameterCurve() = default;
	ParameterCurve(const ParameterCurve& other) = default;

	bool Add(float key, const void* const value);

	// [TODO] Add an assert if the sizeof(T) != data type size. take care for the string case...
	template <typename T>
	bool TAdd(float key, const T& v) {
		return Add(key, (void*)&v);
	}
	bool Evaluate(float key, void* dest) const;

	bool debug_VerifyData() const;
};

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
class SGE_CORE_API Parameter {
  public:
	~Parameter() { Destroy(); }

	Parameter() {}

	// Clears the current stage and specifies the data type.
	// @type - the type of the parameter.
	// @staticValue - If NULL the static moment data will be the default value for that type.
	void Create(ParameterType::Enum const paramType, const void* staticValue = NULL);
	void Destroy();

	bool CreateCurve(const char* const name);    // Returns true of success.
	void SetStaticValue(const void* const data); // Changes the default value of this parameter.

	// A bunch of getters...
	ParameterType::Enum GetType() const { return type; }
	int GetNumCurves() const { return (int)curves.size(); }
	ParameterCurve* GetCurve(const int index) { return &curves.valueAtIdx(index); }
	const ParameterCurve* GetCurve(const int index) const { return &curves.valueAtIdx(index); }
	const char* GetCurveName(const int index) const { return curves.keyAtIdx(index).c_str(); }
	int GetCurveIndex(const char* const name) const;
	ParameterCurve* GetCurve(const char* name);
	const ParameterCurve* GetCurve(const char* name) const;
	const void* GetStaticValue() const;

	// Evalues the value of the parameter at curve "curveName" at sampleTime moment.
	// Depending on the ParameterType the resultData changes it's type.
	// [CAUTION]:
	// For the numeric values the data is what you expect.
	// For ParameterType::String this must be std::string!!!
	void Evalute(void* const resultData, const char* const curveName, const float sampleTime) const;

  private:
	union {
		float staticValue_float;
		vec2f staticValue_vec2f;
		vec3f staticValue_vec3f;
		vec4f staticValue_vec4f;
		quatf staticValue_quatf;
	};

	ParameterType::Enum type;
	vector_map<std::string, ParameterCurve> curves;

	std::string staticValue_string;
};

//-------------------------------------------------------------------------
// Just a set of named parameters.
//-------------------------------------------------------------------------
class SGE_CORE_API ParameterBlock {
  public:
	typedef std::map<std::string, Parameter> map_string_parameter;

	ParameterBlock() {}
	~ParameterBlock() { Destroy(); }

	void Destroy() { m_parameters = map_string_parameter(); }

	map_string_parameter::iterator begin() { return m_parameters.begin(); }
	map_string_parameter::iterator end() { return m_parameters.end(); }

	map_string_parameter::const_iterator begin() const { return m_parameters.cbegin(); }
	map_string_parameter::const_iterator end() const { return m_parameters.cend(); }

	// Searches for a parameter with a given name and returns it.
	// if the parameter was not found and 'typeIfMissing' is != DontCreate
	// a new parameter will be created with that type!
	Parameter* FindParameter(const char* const name,
	                         const ParameterType::Enum typeIfMissing = ParameterType::DontCreate,
	                         const void* staticValue = NULL);
	const Parameter* FindParameter(const char* const name) const;

	// All that std::move madness here cuold be pretty benefitial here?
	void AddParameter(const char* const name, const Parameter& param);

  protected:
	// TODO: This is pretty slow when searching for a param by name.
	map_string_parameter m_parameters;
};

} // namespace sge
