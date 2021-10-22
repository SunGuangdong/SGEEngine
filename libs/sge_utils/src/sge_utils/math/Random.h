#pragma once

#include "vec3.h"
#include <random>

namespace sge {

/// A random number generator, non thread-safe.
struct Random {
	Random(unsigned int startSeed = 5489) { setSeed(startSeed); }

	void setSeed(unsigned int seed) { m_generator.seed(seed); }

	float next01() const {
		std::uniform_real_distribution<float> distribution(0.f, 1.f);

		// That is why the m_generator is mutable.
		// The seed with get incremented here.
		const float rnd = distribution(m_generator);
		return rnd;
	}

	/// Returns a random number in the [-1;1] range.
	float nextSnorm() const {
		std::uniform_real_distribution<float> distribution(-1.f, 1.f);
		const float rnd = distribution(m_generator);
		return rnd;
	}

	/// Returns a random non-negative integer.
	int nextInt() const {
		std::uniform_int_distribution<> distribution;
		const int rnd = distribution(m_generator);
		return rnd;
	}

	/// Returns an integer between [0; maxIntPlusOne).
	/// Notice it is an open interval maxIntPlusOne-1 is the max value.
	/// Useful for indexing an array.
	int nextIntBefore(int maxIntPlusOne) const { return maxIntPlusOne != 0 ? nextInt() % maxIntPlusOne : 0; }

	bool nextBool() const { return nextInt() % 2; }
	float nextFlipFLoat() const { return nextInt() % 2 ? 1.f : -1.f; }

	float nextInRange(float f) const { return next01() * f; }

	float nextInRange(float min, float max) const { return min + next01() * (max - min); }

	vec3f nextPoint3D(const vec3f& size) const { return vec3f(nextInRange(size[0]), nextInRange(size[1]), nextInRange(size[2])); }

  private:
	mutable std::mt19937 m_generator;
};

//---------------------------------------------------------------------------
// PerlinNoise1D
// [0;1] range perlin noise with the specified number of samples.
//---------------------------------------------------------------------------
struct PerlinNoise1D {
	void create(int numSamples, const int seed = 5489);

	/// param @samplePoint a value between 0-1 to be used as a sample position;
	float sample(const float numOctaves, float octaveMult = 0.5f) const;

  private:
	std::vector<float> m_samples; // samples in the range [0;1]
	int m_numOctaves = 0;
};

//---------------------------------------------------------------------------
// PerlinNoise3D
// 1x1x1 perlin nose cube.
//---------------------------------------------------------------------------
struct PerlinNoise3D {
	PerlinNoise3D(const int numPoints = 0, const int seed = 548) { create(numPoints, seed); }

	void create(const int numPoints, const int seed = 5489);

	int getNumPointsPerSide() const { return m_pointsPerSide; }

	float sample(vec3f sample) const;


  private:
	int m_pointsPerSide = 0;
	std::vector<PerlinNoise1D> m_noiseCaches;
};

} // namespace sge
