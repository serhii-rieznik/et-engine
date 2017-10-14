#pragma once

#if !defined(PBR_BASIC_INCLUDES)
#	error This file should not be included directly
#endif

namespace et {

struct PBRSpectrumBase
{
	enum : size_t
	{
		WavelengthStart = 400,
		WavelengthEnd = 700,
		SamplesCount = 30,
	};
};

template <size_t numSamples>
class PBRSpectrumSamples : public PBRSpectrumBase
{
public:
	PBRSpectrumSamples() = default;

	explicit PBRSpectrumSamples(float value) {
		std::fill(std::begin(samples), std::end(samples), value);
	}

	PBRSpectrumSamples& operator += (const PBRSpectrumSamples& r) {
		for (size_t i = 0; i < numSamples; ++i)
			samples[i] += r.samples[i];
		return *this;
	}

	PBRSpectrumSamples operator + (const PBRSpectrumSamples& r) const {
		PBRSpectrumSamples result;
		for (size_t i = 0; i < numSamples; ++i)
			result.samples[i] = samples[i] + r.samples[i];
		return result;
	}

	PBRSpectrumSamples& operator -= (const PBRSpectrumSamples& r) {
		for (size_t i = 0; i < numSamples; ++i)
			samples[i] -= r.samples[i];
		return *this;
	}
	
	PBRSpectrumSamples operator - (const PBRSpectrumSamples& r) const {
		PBRSpectrumSamples result;
		for (size_t i = 0; i < numSamples; ++i)
			result.samples[i] = samples[i] - r.samples[i];
		return result;
	}

	PBRSpectrumSamples& operator *= (const PBRSpectrumSamples& r) {
		for (size_t i = 0; i < numSamples; ++i)
			samples[i] *= r.samples[i];
		return *this;
	}
	
	PBRSpectrumSamples operator * (const PBRSpectrumSamples& r) const {
		PBRSpectrumSamples result;
		for (size_t i = 0; i < numSamples; ++i)
			result.samples[i] = samples[i] * r.samples[i];
		return result;
	}

	PBRSpectrumSamples& operator /= (const PBRSpectrumSamples& r) {
		for (size_t i = 0; i < numSamples; ++i)
			samples[i] /= r.samples[i];
		return *this;
	}

	PBRSpectrumSamples operator / (const PBRSpectrumSamples& r) const {
		PBRSpectrumSamples result;
		for (size_t i = 0; i < numSamples; ++i)
			result.samples[i] = samples[i] / r.samples[i];
		return result;
	}

	PBRSpectrumSamples clamp(const PBRSpectrumSamples& s1, float lo, float hi) {
		PBRSpectrumSamples result;
		for (size_t i = 0; i < numSamples; ++i)
			result.samples[i] = clamp(samples[i], lo, hi);
		return result;
	}

	bool black() const {
		bool result = true;
		for (size_t i = 0; i < numSamples; ++i)
		{
			if (samples[i] > std::numeric_limits<float>::epsilon())
			{
				result = false;
				break;
			}
		}
		return result;
	}

	bool valid() const {
		bool result = true;
		for (size_t i = 0; i < numSamples; ++i)
		{
			if (isnan(samples[i]))
			{
				result = false;
				break;
			}
		}
		return result;
	}

private:
	float samples[numSamples]{};
};

template<size_t n>
inline PBRSpectrumSamples<n> lerp(const PBRSpectrumSamples<n>& s1, const PBRSpectrumSamples<n>& s2, float t) {
	return s1 + (s2 - s1) * t;
}

}