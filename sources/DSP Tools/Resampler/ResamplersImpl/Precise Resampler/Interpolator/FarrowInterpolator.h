// FarrowInterpolator.h
#ifndef FARROWINTERPOLATOR_H
#define FARROWINTERPOLATOR_H

#include <ipps.h>
#include <vector>
#include <array>

class FarrowInterpolator {
public:
	FarrowInterpolator();

	void SetResampleRatio(double resample_ratio);
	void process(const Ipp32fc* input, size_t input_size, std::vector<Ipp32fc>& output);
	void reset();

private:
	struct FarrowCoeffs {
		Ipp32fc a0, a1, a2, a3;
		FarrowCoeffs(const Ipp32fc* samples);
		Ipp32fc interpolate(float t) const;
	};

	bool can_interpolate(int base_index, size_t input_size) const;
	Ipp32fc get_sample(int index, const Ipp32fc* input, size_t input_size) const;
	void update_tail(const Ipp32fc* input, size_t input_size);

	double ratio_;
	double virtual_index_;

	// Всегда храним 4 последних семпла, даже если это нули
	std::array<Ipp32fc, 4> tail_;
};

#endif // FARROWINTERPOLATOR_H