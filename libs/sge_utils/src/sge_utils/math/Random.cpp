#include "Random.h"

namespace sge {

//---------------------------------------------------------------------------
// PerlinNoise1D
//---------------------------------------------------------------------------
void PerlinNoise1D::create(int numOctaves, const int seed)
{
	Random rnd(seed);

	if (numOctaves > 1) {
		m_numOctaves = numOctaves;
		int numSamples = numOctaves == 1 ? 2 : 1 << numOctaves;
		m_samples.resize(numSamples);
		for (int t = 0; t < numSamples; ++t) {
			m_samples[t] = rnd.next01();
		}
	}
}

float PerlinNoise1D::sample(const float samplePoint, float octaveMult) const
{
	// Find the fractional part of the samplePoint, and wrap it around [0; 1] range
	float frac = samplePoint - trunc(samplePoint);
	if (frac < 0.f) {
		frac += 1.f;
	}

	const int numRandomNumbers = int(m_samples.size());

	float fResult = 0.f;

	for (int iOct = 0; iOct < m_numOctaves; ++iOct) {
		const int step = numRandomNumbers >> iOct;
		const int numPointsInOct = numRandomNumbers / step + 1;
		const int numLineSegmentsInOct = numPointsInOct - 1;

		const float fSegmentLen = 1.f / numLineSegmentsInOct;
		const float iSegment = float(int(frac / fSegmentLen));
		const float lerpCoeff = (frac - iSegment * fSegmentLen) / fSegmentLen;

		const int iP0 = int(iSegment) * step % int(m_samples.size());
		const int iP1 = (int(iSegment) + 1) * step % int(m_samples.size());

		const float p0 = m_samples[iP0];
		const float p1 = m_samples[iP1];

		const float f = lerp(p0, p1, lerpCoeff) * powf(octaveMult, float(iOct + 1));
		fResult += f;
	}

	return fResult;
}
//---------------------------------------------------------------------------
// PerlinNoise3D
//---------------------------------------------------------------------------
void PerlinNoise3D::create(const int numPoints, const int seed)
{
	m_pointsPerSide = numPoints;
	const int numPerlins = m_pointsPerSide * m_pointsPerSide;


	m_noiseCaches.resize(numPerlins);
	for (int t = 0; t < numPerlins; ++t) {
		m_noiseCaches[t].create(numPoints, seed + t * 7);
	}
}

float PerlinNoise3D::sample(vec3f sample) const
{
	sample.x = sample.x - trunc(sample.x);
	sample.y = sample.y - trunc(sample.y);
	sample.z = sample.z - trunc(sample.z);

	if (sample.x < 0.f) {
		sample.x += 1.f;
	}

	if (sample.y < 0.f) {
		sample.y += 1.f;
	}

	if (sample.z < 0.f) {
		sample.z += 1.f;
	}

	// Assume that perlins are oriented along X.
	const int yPerlin0 = int(sample.y * m_pointsPerSide) % m_pointsPerSide;
	const int yPerlin1 = (yPerlin0 + 1) % m_pointsPerSide;

	const int zPerlin0 = int(sample.z * m_pointsPerSide) % m_pointsPerSide;
	const int zPerlin1 = (zPerlin0 + 1) % m_pointsPerSide;

	float yp0 = m_noiseCaches[yPerlin0 * m_pointsPerSide + zPerlin0].sample(sample.x);
	float yp1 = m_noiseCaches[yPerlin1 * m_pointsPerSide + zPerlin0].sample(sample.x);
	float zp0 = m_noiseCaches[yPerlin0 * m_pointsPerSide + zPerlin1].sample(sample.x);
	float zp1 = m_noiseCaches[yPerlin1 * m_pointsPerSide + zPerlin1].sample(sample.x);

	float fDistBetweenPerlins = 1.f / float(m_pointsPerSide);
	float yCoeff = (sample.y - fDistBetweenPerlins * int(sample.y / fDistBetweenPerlins)) / fDistBetweenPerlins;
	float zCoeff = (sample.z - fDistBetweenPerlins * int(sample.z / fDistBetweenPerlins)) / fDistBetweenPerlins;

	float yp = lerp(yp0, yp1, yCoeff);
	float zp = lerp(zp0, zp1, yCoeff);


	float res = lerp(yp, zp, zCoeff);
	return res;
}

} // namespace sge
