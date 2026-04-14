#include <ippvm.h>
#include <algorithm>
#include "PipeInterface.h"
#include "BasePipes.h"
using namespace pipes;
//===================================================== FFt ====================================================
void FFtPipe::ProcessData(PipeHolder::sptr meta_data)
{
	auto &passed_data = meta_data->complex_float_data;
	auto &processed_data = meta_data->float_data;

	if (passed_data.empty()) {
		if (processed_data.empty()) return;
		passed_data.resize(processed_data.size());
		ippsRealToCplx_32f(processed_data.data(), nullptr, passed_data.data(), passed_data.size());
	}

	auto &transformed_data = meta_data->buffer_32fc; 
	transformed_data.resize(passed_data.size());
	fft_worker_.EnableSwapOnForward(true);
	if (!fft_worker_.ForwardFFT(passed_data, transformed_data))
		return;
	passed_data.swap(transformed_data);
	transformed_data.clear();
	processed_data.resize(passed_data.size());
	ippsPowerSpectr_32fc(passed_data.data(), processed_data.data(), processed_data.size());
	if (next_) next_->ProcessData(meta_data);
}

//========================================================== Acf ===========================================================
void AcfPipe::ProcessData(PipeHolder::sptr meta_data)
{
	auto &passed_data = meta_data->complex_float_data;
	auto &processed_data = meta_data->float_data;
	if (passed_data.size() < 16) return;

	acf_worker_.Process_32fc_32f( passed_data, processed_data, ippAlgAuto | ippsNormA );
	const int samples_to_zero =  std::max(2ui64, processed_data.size() / 100); //Чтобы лучше нормировалось
	ippsZero_32f(processed_data.data(), samples_to_zero);
	if (next_) next_->ProcessData(meta_data);
}
//========================================================= EnvelopePipe ========================================

void pipes::EnvelopePipe::ProcessData(PipeHolder::sptr meta_data)
{
	auto &complex_data = meta_data->complex_float_data;
	auto &float_data = meta_data->float_data;

	float_data.resize(complex_data.size());
	ippsMagnitude_32fc(complex_data.data(), float_data.data(), float_data.size());
	complex_data.clear(); //Больше неактуально
	if (next_) next_->ProcessData(meta_data);
}

//================================ PhasorPipe =============================

void pipes::PhasorPipe::ProcessData(PipeHolder::sptr meta_data)
{
	auto &complex_data = meta_data->complex_float_data;
	auto &float_data = meta_data->float_data;

	float_data.resize(complex_data.size());
	ippsPhase_32fc(complex_data.data(), float_data.data(), float_data.size());
	complex_data.clear();

	ippsAbs_32f_I(float_data.data(), float_data.size()); 
	//ippsMulC_32f_I(1, float_data.data(), float_data.size());
	if (next_) next_->ProcessData(meta_data);
}

//============================== MulByItSelfPipe ===============================

void pipes::MulByItSelfPipe::ProcessData(PipeHolder::sptr meta_data)
{
	auto &complex_data = meta_data->complex_float_data;
	ippsSqr_32fc_I(complex_data.data(), complex_data.size()); //2
	if (next_) next_->ProcessData(meta_data);
}



void pipes::SamplesDiffPipe::ProcessData(PipeHolder::sptr meta_data)
{
	auto &base = meta_data->complex_float_data;

	const int N = static_cast<int>(base.size());
	if (N < 2) return;
	auto &res = meta_data->buffer_32fc;
	res.resize(N);

	ippsMulByConj_32fc_A11(
		base.data() + 1, // src1: x[i]
		base.data(),     // src2: x[i-1]
		res.data() + 1, // dst
		N - 1
	);
	res.swap(base);
	base[0] = { 0.f, 0.f };

	if (next_) next_->ProcessData(meta_data);
}

void pipes::PowerToDbPipe::ProcessData(PipeHolder::sptr meta_data)
{
	auto &processed_data = meta_data->float_data;
	// Вычисляем магнитуду (амплитуду) комплексных чисел.
	ippsAddC_32f_I(0.0001, processed_data.data(), processed_data.size());
	ippsLog10_32f_A11(processed_data.data(), processed_data.data(), processed_data.size());
	ippsMulC_32f_I(10, processed_data.data(), processed_data.size());

	if (next_) next_->ProcessData(meta_data);
}

pipes::PrecisedPartSaver::PrecisedPartSaver(const int parts_count, const int need_part): parts_count_(parts_count), need_part_(need_part)
{
}

void pipes::PrecisedPartSaver::ProcessData(PipeHolder::sptr meta_data)
{
	auto process = [this](auto& vec)
	{
		const size_t total = vec.size();
		if (total == 0 || parts_count_ <= 0) return;

		const size_t part_size = total / parts_count_;
		const size_t start = part_size * need_part_;

		// защита от выхода за границы
		if (start >= total) {
			vec.clear();
			return;
		}

		const size_t actual_size = std::min(part_size, total - start);

		// если это не первая часть — сдвигаем
		if (start > 0) {
			std::move(vec.begin() + start,
				vec.begin() + start + actual_size,
				vec.begin());
		}

		vec.resize(actual_size);
	};

	process(meta_data->float_data);
	process(meta_data->complex_float_data);
	if (next_) next_->ProcessData(meta_data);
}

pipes::ZeroFirstSamples::ZeroFirstSamples(const double zero_ratio) : zero_ratio_(zero_ratio)
{
}

void pipes::ZeroFirstSamples::ProcessData(PipeHolder::sptr meta_data)
{
	ippsZero_32f(meta_data->float_data.data(), meta_data->float_data.size() * zero_ratio_);
	ippsZero_32fc(meta_data->complex_float_data.data(), meta_data->complex_float_data.size() * zero_ratio_);
	if (next_) next_->ProcessData(meta_data);
}
