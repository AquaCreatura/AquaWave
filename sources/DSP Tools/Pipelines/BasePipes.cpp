#include <ippvm.h>
#include "PipeInterface.h"
#include "BasePipes.h"
using namespace pipes;
//===================================================== FFt ====================================================
void FFtPipe::ProcessData(std::vector<Ipp32fc>& passed_data)
{
	std::vector<Ipp32fc> transformed_data(passed_data.size());
	fft_worker_.EnableSwapOnForward(true);
	if (!fft_worker_.ForwardFFT(passed_data, transformed_data))
		return;
	processed_data_.resize(transformed_data.size());
	// Вычисляем магнитуду (амплитуду) комплексных чисел.
	ippsPowerSpectr_32fc(transformed_data.data(), processed_data_.data(), passed_data.size());
	ippsAddC_32f_I(0.0001, processed_data_.data(), processed_data_.size());
	ippsLog10_32f_A11(processed_data_.data(), processed_data_.data(), processed_data_.size());
	ippsMulC_32f_I(10, processed_data_.data(), processed_data_.size());

	if (next_) next_->ProcessData(processed_data_);
}

std::vector<Ipp32f> FFtPipe::GetProcessed32f()
{
	return processed_data_;
}
//========================================================== Acf ===========================================================
void AcfPipe::ProcessData(std::vector<Ipp32fc>& data_32fc)
{
	if (data_32fc.empty())
	{
		processed_data_.clear();
		return;
	}
	acf_worker_.ProcessMagn( data_32fc, processed_data_, ippAlgAuto | ippsNormA );
	const int samples_to_zero = processed_data_.size() / 100; //Чтобы лучше нормировалось
	ippsZero_32f(processed_data_.data(), samples_to_zero);
	if (next_) next_->ProcessData(processed_data_);
}

std::vector<Ipp32f> AcfPipe::GetProcessed32f()
{
	return processed_data_;
}

