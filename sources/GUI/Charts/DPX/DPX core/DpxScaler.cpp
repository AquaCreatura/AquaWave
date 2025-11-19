#include "DpxScaler.h"
#include <algorithm>
#include <cmath>
#include <cstring> // Для memcpy
#include <stdexcept> // Для исключений
using namespace dpx_core;
using namespace aqua_gui;

DpxDataScaler::DpxDataScaler(dpx_data &init_val)
    : data_(init_val)
{
}

bool DpxDataScaler::UpdateBounds_x(const Limits<double>& new_bounds) {
    // Сохраняем ссылки для удобства
    auto& size = data_.size;
    auto& old_bounds = data_.val_bounds.horizontal;
    const size_t height = size.vertical;
    const size_t old_width = size.horizontal;
    const size_t new_width = size.horizontal; // Ширина остаётся прежней

    // Сохраняем старые границы перед обновлением
    const Limits<double> original_bounds = old_bounds;
    old_bounds = new_bounds;

    // Проверка нулевых размеров
    if (height == 0 || old_width == 0) {
        data_.need_redraw = true;
        return true;
    }
	// Обновляем данные
	tbb::spin_mutex::scoped_lock scoped_locker(data_.redraw_mutex);

    // Создаём новый массив, инициализированный нулями
    std::vector<dpx_data::DataType> new_data(height * new_width, 0);

    // Вычисляем шаг значений для нового массива
    const double new_val_step = (new_width > 1) 
        ? new_bounds.delta() / (new_width - 1) 
        : 0.0;

    // Вычисляем шаг значений для исходного массива
    const double old_val_step = original_bounds.delta() / (old_width - 1);
    
    // Обработка каждой строки
    for (size_t y = 0; y < height; ++y) {
        // Обработка каждого столбца в новом массиве
        for (size_t x = 0; x < new_width; ++x) {
            // Вычисляем значение для текущего пикселя
            const double value = new_bounds.low + x * new_val_step;
            
            // Проверка выхода за границы исходного диапазона
            if (value < original_bounds.low || value > original_bounds.high) {
                continue; // Оставляем 0
            }
            
            // Вычисляем позицию в исходном массиве
            const double normalized = (value - original_bounds.low) / original_bounds.delta();
            double old_idx = normalized * (old_width - 1);
            
            
            // Корректировка для последнего элемента
            if (old_idx >= old_width - 1) {
                old_idx = old_width - 1 - 1e-9;
            }
            
            const size_t idx1 = static_cast<size_t>(old_idx);
            const size_t idx2 = (idx1 < old_width - 1) ? idx1 + 1 : idx1;
            const double t = old_idx - idx1;

            // Интерполяция значений
            if (idx1 == idx2) {
                new_data[y * new_width + x] = data_[y][idx1];
            } else {
                SlopeInterpolator interpolator(data_[y][idx1], data_[y][idx2]);
                new_data[y * new_width + x] = static_cast<dpx_data::DataType>(interpolator.Interpolate(t));
            }
        }
    }

    // Обновляем данные
    data_.data.swap(new_data);
    
    // Пересчитываем веса столбцов
    data_.column_weight.assign(new_width, 0);
    for (size_t x = 0; x < new_width; ++x) {
        for (size_t y = 0; y < height; ++y) {
            data_.column_weight[x] += data_[y][x];
        }
    }

    data_.need_redraw = true;
    return true;
}


bool DpxDataScaler::UpdateBounds_y(const Limits<double>& new_bounds)
{
    // Сохраняем ссылки для удобства
    auto& size = data_.size;
    auto& ref_bounds = data_.val_bounds.vertical;
    
    if(ref_bounds == new_bounds) return true;

    const size_t width = size.horizontal;
    const size_t height = size.vertical;

    // Сохраняем старые границы перед обновлением
    const Limits<double> old_bounds = ref_bounds;
    ref_bounds = new_bounds;

    // Проверка нулевых размеров
    if (width == 0 || height == 0) {
        data_.need_redraw = true;
        return true;
    }
	// Обновляем данные
	tbb::spin_mutex::scoped_lock scoped_locker(data_.redraw_mutex);

    // Создаём новый массив, инициализированный нулями
    std::vector<dpx_data::DataType> new_data(height * width, 0);



    // Вычисляем шаг значений для нового массива
	const double new_val_step = new_bounds.delta() / (height);

    // Вычисляем шаг значений для исходного массива
    const double old_val_step = old_bounds.delta() / (height);
    
    // Обработка каждого столбца
    for (size_t x = 0; x < width; ++x) {
        // Обработка каждой строки в новом массиве
        for (size_t y = 0; y < height; ++y) {
            // Вычисляем значение в центре текущего пикселя
            const double value = new_bounds.low + (y + 0.5) * new_val_step;
            
            // Проверка выхода за границы исходного диапазона
            if (value < old_bounds.low|| value > old_bounds.high) {
                continue; // Оставляем 0
            }
            
            // Вычисляем позицию в старом массиве
            const double normalized = (value - old_bounds.low) / old_bounds.delta();
            double pos_norm = normalized * (height) ;
			pos_norm = qBound(0., pos_norm, height - 1.);
            
            
            const size_t left_idx = static_cast<size_t>(pos_norm);
            const size_t right_idx = (left_idx < height - 1) ? left_idx + 1 : left_idx;
            const double dist_to_center = pos_norm - left_idx;

	
            // Интерполяция значений
            SlopeInterpolator interpolator(data_[left_idx][x], data_[right_idx][x]);
            new_data[y * width + x] = static_cast<dpx_data::DataType>(interpolator.Interpolate(dist_to_center));
        }
    }
    data_.data.swap(new_data);
    data_.need_redraw = true;
    return true;
}
