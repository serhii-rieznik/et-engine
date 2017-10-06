#pragma once

#if !defined(PBR_BASIC_INCLUDES)
#	error This file should not be included directly
#endif

namespace et
{
template <size_t numSamples>
class PBRSpectrumSamples
{
public:
	PBRSpectrumSamples() = default;

	PBRSpectrumSamples(float value) {
		std::fill(std::begin(samples), std::end(samples), value);
	}

#define IMPLEMENT_OPERATORS(OP) \
	PBRSpectrumSamples& operator OP= (const PBRSpectrumSamples& r) { \
		for (size_t i = 0; i < numSamples; ++i) { samples[i] OP= r.samples[i]; } \
		return *this; } \
	PBRSpectrumSamples operator OP (const PBRSpectrumSamples& r) const { \
		PBRSpectrumSamples result; \
		for (size_t i = 0; i < numSamples; ++i) { result.samples[i] = samples[i] OP r.samples[i]; } \
		return result; }

	IMPLEMENT_OPERATORS(+);
	IMPLEMENT_OPERATORS(-);
	IMPLEMENT_OPERATORS(*);
	IMPLEMENT_OPERATORS(/);
#undef IMPLEMENT_OPERATORS

private:
	float samples[numSamples]{};
};
}