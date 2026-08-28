#pragma once
#include <ipps.h>
#include <vector>
#include <cstddef>

#include <limits>
#include "Interfaces/special_defs/spectral_viewer_defs.h"
using namespace dpx_core;

namespace peak_detection{
	//Находит N лучших пиков методом змейки, где качество пика определяется отношением значения самого пика к среднему значения в пределах зоны проверки. 
	//При этом минимальное расстояние между пиками = min_dist_ratio. Может выдать и ничего, если пиков нет.
	std::vector<size_t> FindPeaksToAvg(Ipp32f* data, size_t count_of_samples, int count_of_peaks, double check_area_ratio = 0.1, double min_dist_ratio = 0.1, double threshold = 2.0);

	//Сам класс
	class PeakDetector {
	public:
		PeakDetector();
		~PeakDetector() = default;

		// Инициализация: тип данных и частота дискретизации (Гц)
		void Init(kDpxChartType data_type, double sample_rate_hz, double carrier_hz = 0);

		// Полный сброс состояния
		void Reset();

		// Обработка нового кадра данных
		void Process(const Ipp32f* data, size_t count_of_samples);

		// Возвращает текущую оценку пика:
		//   - kACF: период в миллисекундах
		//   - kFFT: частота максимального пика в герцах
		//   - kPower4x: частота центра симметрии в герцах
		//   - kEnvelope, kPhasor: символьная скорость в герцах
		// Если данных недостаточно, возвращает NaN.
		double CalculatePeak() const;

		bool IsValid() const { return valid_; }

	private:
		// Обновление MaxHold для спектральных данных (с утечкой)
		void UpdateMaxHold(const Ipp32f* data, size_t count_of_samples);

		// Оценка частоты максимального пика для kFFT
		double GetPeakSpectrumMax() const;

		// Оценка центра симметрии для kPower4x
		double GetPeakSpectrumSymmetry() const;

		// Оценка периода для автокорреляции (ACF)
		double GetPeakAcf() const;

		// Оценка символьной скорости
		double GetPeakSymbolRate() const;

		// Параболическая интерполяция позиции пика
		double GetInterpolatedPeak(size_t peak_index, const std::vector<Ipp32f>& data) const;

		// Поиск фундаментальной частоты/периода
		size_t FindFundamental(const std::vector<size_t>& peaks,
			double tolerance_ratio,
			int max_harmonic) const;

		// Проверка кратности p относительно f
		bool IsHarmonicOf(size_t p, size_t f, double tolerance, int max_harmonic) const;

		kDpxChartType data_type_;
		double sample_rate_hz_;
		double carrier_hz_;
		int64_t				n_fft_;
		std::vector<Ipp32f> max_hold_;  // единый буфер: для ACF – последний кадр, для остальных – MaxHold с утечкой
		size_t count_of_samples_;
		bool valid_;

		int max_peaks_count_;
	};
};
