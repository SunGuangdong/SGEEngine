#include "sge_utils/utils/common.h"
#include <algorithm>
#include <sge_utils/math/mat4.h>
#include <sge_utils/utils/json.h>

#include "Parameter.h"

namespace sge {

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
bool ParameterCurve::debug_VerifyData() const {
	if (type == ParameterType::String) {
		return true;
	}

	const size_t dataSize = ParameterType::SizeBytes(type);

	const size_t numKeys = keys.size();
	const size_t numDatas = data.size() / dataSize;

	return numKeys == numDatas;
}

bool ParameterCurve::Add(float key, const void* const value) {
	// String Parameters are not animateable(currently).
	sgeAssert(type != ParameterType::String);

	bool keyAsSameTimeExisted = false;
	auto it = std::end(keys);
	for (auto itr = std::begin(keys); itr != std::end(keys); ++itr) {
		if (key < *itr) {
			it = itr;
			break;
		} else if (key == *itr) {
			keyAsSameTimeExisted = true;
			it = itr;
			break;
		}
	}

	const int idx = (int)(it - keys.begin());
	const int dataSize = ParameterType::SizeBytes(type);

	if (keyAsSameTimeExisted) {
		keys[idx] = key;
		memcpy(&data.front() + +idx * dataSize, value, dataSize);
	} else {
		keys.insert(it, key);
		data.insert(data.begin() + idx * dataSize, (char*)value, (char*)value + dataSize);
	}

	debug_VerifyData();

	return true;
}

bool ParameterCurve::Evaluate(float time, void* dest) const {
	if (type == ParameterType::String) {
		// String parameters are not animatable currently.
		return false;
	}

	// If the curve has no keys there is nothing we can evaluate.
	if (keys.size() == 0) {
		return false;
	}

	const int dataSize = ParameterType::SizeBytes(type);

#if 0
	// Find the 1st larger element we interpolate will in [keyIdx-1, keyIdx] interval.
	// [TODO] Implement a better than linear search here.
	int keyIdx = keys.size();

	for(int t = 0; t < (int)keys.size(); ++t) {
		if(keys[t] >= time) {
			keyIdx = t;
			break;
		}
	}
#else
	const auto itr = std::lower_bound(keys.begin(), keys.end(), time);
	int keyIdx = int(itr - keys.begin());
#endif

	if (keyIdx == 0) {
		// The requested evaluation time is before the 1st keyframe.
		memcpy(dest, data.data(), dataSize);
		return true;
	} else if (keyIdx == keys.size()) {
		// The requested evaluation time is after the last keyframe.
		memcpy(dest, &data.back() - dataSize + 1, dataSize);
		return true;
	}

	// Between 2 keys.
	const float timeRange = keys[keyIdx] - keys[keyIdx - 1];
	const float subTime = time - keys[keyIdx - 1];
	const float interpolationCoeff = subTime / timeRange;

	[[maybe_unused]] const auto& debug_typeInfo = ParameterType::info(type);

	if ((type == ParameterType::Float) || (type == ParameterType::Float2) || (type == ParameterType::Float3) ||
	    (type == ParameterType::Float4)) {
		const float* const fData = (float*)data.data();
		float* const fResult = (float*)(dest);

		const int arity = dataSize / sizeof(float);
		for (int t = 0; t < arity; ++t) {
			const float a = fData[(keyIdx - 1) * arity + t];
			const float b = fData[(keyIdx)*arity + t];
			fResult[t] = lerp<float>(a, b, interpolationCoeff);
		}
	} else if (type == ParameterType::Quaternion) {
		const quatf* const qData = (quatf*)data.data();
		quatf& result = *(quatf*)(dest);
		result = slerp(qData[keyIdx - 1], qData[keyIdx], interpolationCoeff);
	} else {
		// Unimplemented type.
		sgeAssert(false);
	}

	return true;
}

void Parameter::Create(ParameterType::Enum const paramType, const void* staticValue) {
	Destroy();

	this->type = paramType;

	int const dataSize = ParameterType::SizeBytes(type);

	// Initialize the static(default) value. if the Inital data is null
	// initialize the data with something neutral.
	if (staticValue != NULL) {
		switch (type) {
			case ParameterType::Float:
				staticValue_float = *(const float*)staticValue;
				break;
			case ParameterType::Float2:
				staticValue_vec2f = *(const vec2f*)staticValue;
				break;
			case ParameterType::Float3:
				staticValue_vec3f = *(const vec3f*)staticValue;
				break;
			case ParameterType::Float4:
				staticValue_vec4f = *(const vec4f*)staticValue;
				break;
			case ParameterType::Quaternion:
				staticValue_quatf = *(const quatf*)staticValue;
				break;
			case ParameterType::String:
				staticValue_string = (const char*)staticValue;
				break;
			default: {
				sgeAssert(false);
				Destroy();
				return;
			}
		}
	} else {
		switch (type) {
			case ParameterType::Float:
				staticValue_float = 0.f;
				break;
			case ParameterType::Float2:
				staticValue_vec2f = vec2f(0.f);
				break;
			case ParameterType::Float3:
				staticValue_vec3f = vec3f(0.f);
				break;
			case ParameterType::Float4:
				staticValue_vec4f = vec4f(0.f);
				break;
			case ParameterType::Quaternion:
				staticValue_quatf = quatf::getIdentity();
				break;
			case ParameterType::String:
				staticValue_string = std::string();
				break;
			default: {
				sgeAssert(false);
				Destroy();
				return;
			}
		}
	}

	return;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void Parameter::Destroy() {
	staticValue_string = std::string();
	curves = vector_map<std::string, ParameterCurve>();
}

void Parameter::SetStaticValue(const void* const staticValue) {
	switch (type) {
		case ParameterType::Float:
			staticValue_float = *(const float*)staticValue;
			break;
		case ParameterType::Float2:
			staticValue_vec2f = *(const vec2f*)staticValue;
			break;
		case ParameterType::Float3:
			staticValue_vec3f = *(const vec3f*)staticValue;
			break;
		case ParameterType::Float4:
			staticValue_vec4f = *(const vec4f*)staticValue;
			break;
		case ParameterType::Quaternion:
			staticValue_quatf = *(const quatf*)staticValue;
			break;
		case ParameterType::String:
			staticValue_string = (const char*)staticValue;
			break;
		default: {
			sgeAssert(false);
			Destroy();
			return;
		}
	}

	return;
}

ParameterCurve* Parameter::GetCurve(const char* name) {
	int const index = GetCurveIndex(name);
	if (index < 0)
		return NULL;
	return GetCurve(index);
}

const ParameterCurve* Parameter::GetCurve(const char* name) const {
	int const index = GetCurveIndex(name);
	if (index < 0)
		return NULL;
	return GetCurve(index);
}

const void* Parameter::GetStaticValue() const {
	if (type != ParameterType::String)
		return &staticValue_float;
	return staticValue_string.c_str();
}

bool Parameter::CreateCurve(const char* const name) {
	if (GetCurveIndex(name) != -1) {
		sgeAssert(false); // Trying to create a curve that already exists for this parameter.
		return false;
	}

	curves[name] = ParameterCurve();
	curves[name].type = type;
	return true;
}

int Parameter::GetCurveIndex(const char* const name) const {
	if (name == nullptr) {
		return -1;
	}

	for (auto itr : curves) {
		if (itr.key() == name) {
			return (int)itr.idx;
		}
	}

	return -1;
}

void Parameter::Evalute(void* const resultData, const char* const curveName, const float sampleTime) const {
	sgeAssert(resultData);

	int const curveIndex = GetCurveIndex(curveName);

	// If the evaluation did not succeeded(usually becase there the curve has no keys)
	// or if the parameter doesn't have this curve just use the default.
	if (curveIndex == -1 || (curves.valueAtIdx(curveIndex).Evaluate(sampleTime, resultData) == false)) {
		// Copy the static value.
		if (type == ParameterType::String) {
			std::string& resultString = *(std::string*)resultData;
			resultString = staticValue_string;
		} else {
			const size_t elemByteSize = ParameterType::info(type).sizeBytes;
			sgeAssert(elemByteSize > 0);
			memcpy(resultData, &staticValue_float, elemByteSize);
		}
		return;
	}
}


//-------------------------------------------------------------------------
// ParameterBlock
//-------------------------------------------------------------------------
Parameter* ParameterBlock::FindParameter(const char* const name, const ParameterType::Enum typeIfMissing, const void* staticValue) {
	// Search if the parameter already exists, if not create a new one depending on the input type.
	auto itr = m_parameters.find(name);

	if (itr != m_parameters.end()) {
		return &itr->second;
	} else if (typeIfMissing != ParameterType::DontCreate) {
		Parameter& prm = m_parameters[name];
		prm.Create(typeIfMissing, staticValue);
		return &prm;
	}

	return nullptr;
}

const Parameter* ParameterBlock::FindParameter(const char* const name) const {
	auto itr = m_parameters.find(name);
	if (itr == m_parameters.end()) {
		return nullptr;
	}

	return &itr->second;
}

void ParameterBlock::AddParameter(const char* const name, const Parameter& param) {
	if (FindParameter(name))
		return;
	m_parameters[name] = std::move(param);
}

} // namespace sge
