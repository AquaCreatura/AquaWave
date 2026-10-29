#pragma once
#include "DSP Tools/Pipelines/BasePipes.h"
#include "Interfaces/special_defs/spectral_viewer_defs.h"

#include <ipps.h>

#include <cstddef>
#include <vector>

using namespace dpx_core;

namespace peak_detection{
	//Находит N лучших пиков методом змейки, где качество пика определяется отношением значения самого пика к среднему значения в пределах зоны проверки. 
	//При этом минимальное расстояние между пиками = min_dist_ratio. Может выдать и ничего, если пиков нет.
	std::vector<size_t> FindPeaksToAvg(const Ipp32f* data, size_t count_of_samples, int count_of_peaks, double check_area_ratio = 0.1, double min_dist_ratio = 0.1, double threshold = 2.0);

	//Сам класс
	class PeakDetector {
	public:
		PeakDetector();
		~PeakDetector() = default;

		// Инициализация: тип данных и частота дискретизации (Гц)
		void Init(kDpxChartType data_type, int n_fft, double sample_rate_hz, double carrier_hz = 0);

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
		std::vector<double> CalculatePeak() const;

		bool IsValid() const { return valid_; }

	private:
		struct estim_params {
			int	   zero_shift = 0;
			double norm_ratio = 1.;
			double add_before_norm = 0.;
			double add_after_norm = 0.;
		};
		// Обновление MaxHold для спектральных данных (с утечкой)
		void UpdateMaxHold(const Ipp32f* data, size_t count_of_samples);


		// Параболическая интерполяция позиции пика
		double GetInterpolatedPeak(size_t peak_index, const std::vector<Ipp32f>& data) const;

		kDpxChartType data_type_;
		double sample_rate_hz_;
		double carrier_hz_;
		std::vector<Ipp32f> max_hold_;  // единый буфер: для ACF – последний кадр, для остальных – MaxHold с утечкой
		
		size_t n_fft_;
		bool valid_;
		estim_params estim_params_;
		int max_peaks_count_;

		mutable std::vector<Ipp32f> db_scaled_;
		mutable pipes::PowerToDbPipe power_to_db_pipe_;		
	};
};
