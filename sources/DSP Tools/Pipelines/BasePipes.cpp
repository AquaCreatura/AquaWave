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
//========================================================= EnvelopePipe ========================================

void pipes::EnvelopePipe::ProcessData(std::vector<Ipp32fc>& data_32fc)
{
	// 1. Получить размер входного блока N (data_32fc.size())
	// 2. Вычислить огибающую (модуль) каждого комплексного отсчёта:
	//    для i = 0..N-1: envelope[i] = sqrt(real^2 + imag^2)
	//    Использовать функцию вычисления магнитуды для вектора комплексных чисел.
	// 3. Подготовить буфер для спектра: размер спектра (N/2 + 1) для вещественного БПФ.
	// 4. Вычислить спектр мощности огибающей:
	//    - применить оконную функцию (опционально, но обычно Хэмминга или отсутствует)
	//    - выполнить БПФ вещественного массива envelope
	//    - вычислить квадрат модуля каждого БПФ-коэффициента
	// 5. Сохранить полученный спектр в processed_data_
}

std::vector<Ipp32f> pipes::EnvelopePipe::GetProcessed32f()
{
	return processed_data_;
}
//================================ PhasorPipe =============================

void pipes::PhasorPipe::ProcessData(std::vector<Ipp32fc>& data_32fc)
{
	// 1. Получить размер входного блока N
	// 2. Вычислить фазу каждого комплексного отсчёта: phase[i] = atan2(imag, real)
	//    Использовать функцию вычисления фазы для вектора комплексных чисел.
	// 3. Вычислить мгновенную частоту как разность фаз соседних отсчётов:
	//    для i = 0..N-2: freq_inst[i] = phase[i+1] - phase[i]
	//    Скорректировать разность на интервал [-pi, pi] (устранить скачки фазы)
	// 4. Получается массив вещественных чисел длины N-1.
	// 5. Вычислить спектр мощности этого массива:
	//    - возможно, применить окно
	//    - выполнить БПФ вещественного массива freq_inst
	//    - получить квадрат модуля коэффициентов (размер (N-1)/2 + 1)
	// 6. Сохранить спектр в processed_data_
}

std::vector<Ipp32f> pipes::PhasorPipe::GetProcessed32f()
{
	return processed_data_;
}
//============================== MulByItSelfPipe ===============================

void pipes::MulByItSelfPipe::ProcessData(std::vector<Ipp32fc>& data_32fc)
{
	// 1. Получить размер входного блока N
	// 2. Возвести каждый комплексный отсчёт в четвёртую степень:
	//    для i = 0..N-1: raised[i] = data[i] * data[i] * data[i] * data[i]
	//    (можно последовательно умножить на себя два раза)
	// 3. Получен новый комплексный массив raised длины N.
	// 4. Вычислить спектр мощности массива raised:
	//    - применить окно (по желанию)
	//    - выполнить комплексное БПФ
	//    - вычислить квадрат модуля каждого БПФ-коэффициента (размер N/2 + 1)
	// 5. Сохранить спектр в processed_data_
}

std::vector<Ipp32f> pipes::MulByItSelfPipe::GetProcessed32f()
{
	return processed_data_;
}

void pipes::SamplesDiffPipe::ProcessData(std::vector<Ipp32fc>& data_32fc)
{
}

std::vector<Ipp32f> pipes::SamplesDiffPipe::GetProcessed32f()
{
	return std::vector<Ipp32f>();
}
