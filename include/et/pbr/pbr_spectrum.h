#pragma once

#if !defined(PBR_BASIC_INCLUDES)
#	error This file should not be included directly
#endif

namespace et {

namespace pbr {

extern void initSpectrumInternalValues();

struct SpectrumBase
{
	enum : size_t
	{
		WavelengthStart = 400,
		WavelengthEnd = 700,
		WavelengthSamples = 30,
	};

	static float averageSamples(const float wavelengths[], const float values[], size_t count, float wavelengthBegin, float wavelengthEnd);
};

template <size_t numSamples>
class SpectrumSamples : public SpectrumBase
{
public:
	enum : size_t
	{
		SamplesCount = numSamples,
	};

public:
	SpectrumSamples() {
		initSpectrumInternalValues();
	}

	explicit SpectrumSamples(float value) {
		initSpectrumInternalValues();
		std::fill(std::begin(samples), std::end(samples), value);
	}

	SpectrumSamples& operator += (const SpectrumSamples& r) {
		for (size_t i = 0; i < SamplesCount; ++i)
			samples[i] += r.samples[i];
		return *this;
	}

	SpectrumSamples operator + (const SpectrumSamples& r) const {
		SpectrumSamples result;
		for (size_t i = 0; i < SamplesCount; ++i)
			result.samples[i] = samples[i] + r.samples[i];
		return result;
	}

	SpectrumSamples& operator -= (const SpectrumSamples& r) {
		for (size_t i = 0; i < SamplesCount; ++i)
			samples[i] -= r.samples[i];
		return *this;
	}

	SpectrumSamples operator - (const SpectrumSamples& r) const {
		SpectrumSamples result;
		for (size_t i = 0; i < SamplesCount; ++i)
			result.samples[i] = samples[i] - r.samples[i];
		return result;
	}

	SpectrumSamples& operator *= (const SpectrumSamples& r) {
		for (size_t i = 0; i < SamplesCount; ++i)
			samples[i] *= r.samples[i];
		return *this;
	}

	SpectrumSamples operator * (const SpectrumSamples& r) const {
		SpectrumSamples result;
		for (size_t i = 0; i < SamplesCount; ++i)
			result.samples[i] = samples[i] * r.samples[i];
		return result;
	}

	SpectrumSamples& operator /= (const SpectrumSamples& r) {
		for (size_t i = 0; i < SamplesCount; ++i)
			samples[i] /= r.samples[i];
		return *this;
	}

	SpectrumSamples operator / (const SpectrumSamples& r) const {
		SpectrumSamples result;
		for (size_t i = 0; i < SamplesCount; ++i)
			result.samples[i] = samples[i] / r.samples[i];
		return result;
	}

	SpectrumSamples clamp(const SpectrumSamples& s1, float lo, float hi) {
		SpectrumSamples result;
		for (size_t i = 0; i < SamplesCount; ++i)
			result.samples[i] = clamp(samples[i], lo, hi);
		return result;
	}

	bool black() const {
		bool result = true;
		for (size_t i = 0; i < SamplesCount; ++i)
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
		for (size_t i = 0; i < SamplesCount; ++i)
		{
			if (isnan(samples[i]))
			{
				result = false;
				break;
			}
		}
		return result;
	}

protected:
	float samples[SamplesCount]{};

private:
	friend void initSpectrumInternalValues();
};

class DefaultSpectrumSamples : public SpectrumSamples<SpectrumBase::WavelengthSamples>
{
public:
	using BaseClass = SpectrumSamples<SpectrumBase::WavelengthSamples>;

public:
	DefaultSpectrumSamples() = default;
	
	explicit DefaultSpectrumSamples(float v) :
		BaseClass(v) {
	}

	float* mutableSamples() {
		return samples;
	}

	void toXYZ(float xyz[3]);
	void toRGB(float rgb[3]);
};

inline void XYZToRGB(const float xyz[3], float rgb[3]) {
	rgb[0] = 3.240479f * xyz[0] - 1.537150f * xyz[1] - 0.498535f * xyz[2];
	rgb[1] = -0.969256f * xyz[0] + 1.875991f * xyz[1] + 0.041556f * xyz[2];
	rgb[2] = 0.055648f * xyz[0] - 0.204043f * xyz[1] + 1.057311f * xyz[2];
}

inline void RGBToXYZ(const float rgb[3], float xyz[3]) {
	xyz[0] = 0.412453f * rgb[0] + 0.357580f * rgb[1] + 0.180423f * rgb[2];
	xyz[1] = 0.212671f * rgb[0] + 0.715160f * rgb[1] + 0.072169f * rgb[2];
	xyz[2] = 0.019334f * rgb[0] + 0.119193f * rgb[1] + 0.950227f * rgb[2];
}

}

template<size_t n>
inline pbr::SpectrumSamples<n> lerp(const pbr::SpectrumSamples<n>& s1, const pbr::SpectrumSamples<n>& s2, float t) {
	return s1 + (s2 - s1) * t;
}
}
