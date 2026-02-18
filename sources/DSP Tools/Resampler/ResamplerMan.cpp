#include <algorithm>
#include <corecrt_math_defines.h>
#include "ResamplerMan.h"
#include "ResamplersImpl/Precise Resampler/PreciseResampler.h"
#include "ResamplersImpl/MR Resampler/MultiRateResampler.h"
constexpr double max_error = 1.e-4;
using namespace aqua_resampler;

// Конструктор менеджера ресэмплинга.
// Здесь устанавливаются базовые значения настроек ресэмплера.
ResamplerManager::ResamplerManager() 
{
    settings_.filter_length = 512; // Длина фильтра по умолчанию.
    settings_.max_denom = 100;       // Максимальное значение знаменателя для аппроксимации отношения.
}

// Деструктор менеджера ресэмплинга.
// Освобождает все выделенные ресурсы при уничтожении объекта.
ResamplerManager::~ResamplerManager() 
{
    FreeResources();
}

int get_fir_power_of_two(double resample_ratio, double util_factor) {
	if (resample_ratio <= 0 || util_factor >= 1.0 || util_factor <= 0) return 0;

	const double A = 60.0; // Затухание в дБ

						   // 1. Находим ширину полосы пропускания W
	double W = resample_ratio;

	// 2. Находим ширину переходной полосы Delta_f
	double delta_f = W * (1.0 - util_factor);

	// 3. Считаем порядок N по формуле Кайзера
	double N = (A - 8.0) / (14.36 * delta_f);

	// 4. Длина фильтра L = N + 1
	double L = N + 1.0;

	// 5. Находим следующую степень двойки
	// log2(L) дает дробную степень, ceil округляет вверх, pow возводит в степень
	int power_of_two = static_cast<int>(std::pow(2, std::ceil(std::log2(L))));

	return power_of_two;
}
// Метод инициализации ресэмплера.
// base_params  - базовые параметры обработки, содержащие частоту дискретизации исходного сигнала.
// target_params- параметры целевого сигнала, частота дискретизации которого может быть изменена.
// precise      - флаг, указывающий на необходимость точного расчёта коэффициентов ресэмплинга.
// Возвращает true, если инициализация прошла успешно, иначе false.
bool ResamplerManager::Init(const fluctus::freq_params& base_params, fluctus::freq_params& target_params, bool precise)
{
    // Освобождаем ранее выделенные ресурсы, если таковые были.
    FreeResources();

    const int64_t base_rate = base_params.samplerate_hz;   // Исходная частота дискретизации.
    int64_t target_rate = target_params.samplerate_hz;       // Желаемая частота дискретизации.

    // Проверяем корректность входных параметров: частоты должны быть положительными.
    if (base_rate <= 0 || target_rate <= 0) return false;
    // Если исходная и целевая частоты равны, ресэмплинг не требуется.
     

    // Вычисляем отношение целевой частоты к базовой.
	const double target_ratio = static_cast<double>(target_rate) / base_rate;
	freq_shifter_.Init(base_params.carrier_hz, target_params.carrier_hz, base_params.samplerate_hz);
	resample_ratio_ = target_ratio;
	if (resample_ratio_ == 1)
		return true;
	try
	{
		// Пара для представления аппроксимированного отношения в виде дроби.
		std::pair<int64_t, int64_t> pq = FindBestFraction(target_ratio, settings_.max_denom, true);
		bool use_multirate = false;      // Флаг, указывающий на необходимость использования многоскоростного ресэмплинга.
		if (precise && (target_ratio < 1.f))  //Интерполяция только с помощью MR, т.к. иначе возникают отзеркаливание спектра
		{
			// Если требуется точный ресэмплинг, находим дробь, максимально приближенную к target_ratio,
			const double approx_ratio = static_cast<double>(pq.first) / pq.second;
			// Вычисляем разницу между аппроксимированным и целевым отношением.
			const double diff = std::abs(approx_ratio - target_ratio);
			// Если разница не превышает 0.001, считаем, что можно использовать многоскоростной ресэмплинг.
			use_multirate = (diff <= max_error);
		}
		else
		{
			// Если точность не является критичной, аппроксимируем отношение с помощью округления.
			use_multirate = true;
		}
		int64_t new_target_rate; // Новая частота дискретизации, вычисленная на основе аппроксимированного отношения.
		if (use_multirate)
		{
			int64_t numerator;
			numerator = base_rate * pq.first; // Вычисляем числитель нового отношения.            

			new_target_rate = numerator / pq.second; // Вычисляем новую частоту дискретизации.
			// Создаем объект ресэмплера для многоскоростного ресэмплинга.
			resampler_ = std::make_unique<MultiRateResampler>(); //MultiRateResampler
		}
		else
		{
			new_target_rate = target_rate; // Если многоскоростной ресэмплинг не используется, целевая частота остается неизменной.
			// Создаем объект ресэмплера для точного ресэмплинга.
			resampler_ = std::make_unique<PreciseResampler>(); //PreciseResampler
		}

		// Устанавливаем настройки ресэмплера.
		{
			int fir_length = get_fir_power_of_two(double(new_target_rate) / base_params.samplerate_hz, settings_.filter_koeff); //Коэффициент фильтрации
			fir_length = qBound(16, fir_length, 1024);
			settings_.filter_length = fir_length;
			resampler_->SetSettings(settings_);
		}
		

		// Инициализируем ресэмплер с базовой и новой целевой частотой.
		if (!resampler_->Init(base_rate, new_target_rate))
		{
			resampler_.reset(); // Если инициализация не удалась, освобождаем ресурсы.
			return false;
		}

		// Обновляем параметры целевого сигнала с новой частотой дискретизации.
		target_params.samplerate_hz = new_target_rate;
		return true;
	}
	catch (const std::exception& e)
	{
		return false;
	}

}

// Метод обработки блока данных.
// input_data - указатель на входной массив данных (комплексные значения).
// size       - размер входного блока данных.
// Возвращает true, если данные обработаны успешно, иначе false.
bool ResamplerManager::ProcessBlock(const Ipp32fc* input_data, size_t size) 
{
    // Проверяем, что ресэмплер и входные данные существуют, а размер блока больше нуля.
    // Затем передаем данные в метод ProcessData ресэмплера, который записывает результат в processed_data_.
    if(!input_data || size <= 0)
        return false;
    shifted_data_.resize(size);
    freq_shifter_.ProcessBlock(input_data,shifted_data_.data(), size);

	if (resample_ratio_ == 1) {
		processed_data_.swap(shifted_data_);
		return true;
	}
	if (!resampler_) 
		return false;
    bool res = resampler_->ProcessData(shifted_data_.data(), shifted_data_.size(), processed_data_);
    return res;
}

// Метод получения обработанных данных.
// Возвращает ссылку на вектор, содержащий обработанные комплексные данные.
std::vector<Ipp32fc>& ResamplerManager::GetProcessedData() 
{
    return processed_data_;
}

// Метод освобождения ресурсов, используемых ресэмплером и буфером обработанных данных.
void ResamplerManager::FreeResources() 
{
    if (resampler_) 
    {
        resampler_->Clear(); // Очищаем внутреннее состояние ресэмплера.
        resampler_.reset();  // Удаляем объект ресэмплера.
    }
    processed_data_.clear(); // Очищаем вектор с обработанными данными.
}




