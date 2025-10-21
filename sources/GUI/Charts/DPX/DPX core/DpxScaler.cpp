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
    auto& old_bounds = data_.val_bounds.vertical;
    
    if(old_bounds == new_bounds) return true;

    const size_t width = size.horizontal;
    const size_t old_height = size.vertical;
    const size_t new_height = size.vertical; // Высота остаётся прежней

    // Сохраняем старые границы перед обновлением
    const Limits<double> original_bounds = old_bounds;
    old_bounds = new_bounds;

    // Проверка нулевых размеров
    if (width == 0 || old_height == 0) {
        data_.need_redraw = true;
        return true;
    }
	// Обновляем данные
	tbb::spin_mutex::scoped_lock scoped_locker(data_.redraw_mutex);

    // Создаём новый массив, инициализированный нулями
    std::vector<dpx_data::DataType> new_data(new_height * width, 0);

    // Вычисляем шаг значений для нового массива
    const double new_val_step = (new_height > 1) 
        ? new_bounds.delta() / (new_height - 1) 
        : 0.0;

    // Вычисляем шаг значений для исходного массива
    const double old_val_step = original_bounds.delta() / (old_height - 1);
    
    // Обработка каждого столбца
    for (size_t x = 0; x < width; ++x) {
        // Обработка каждой строки в новом массиве
        for (size_t y = 0; y < new_height; ++y) {
            // Вычисляем значение для текущего пикселя
            const double value = new_bounds.low + y * new_val_step;
            
            // Проверка выхода за границы исходного диапазона
            if (value < original_bounds.low|| value > original_bounds.high) {
                continue; // Оставляем 0
            }
            
            // Вычисляем позицию в исходном массиве
            const double normalized = (value - original_bounds.low) / original_bounds.delta();
            double old_idx = normalized * (old_height - 1);
            
            
            // Корректировка для последнего элемента
            if (old_idx >= old_height - 1) {
                old_idx = old_height - 1 - 1e-9;
            }
            
            const size_t idx1 = static_cast<size_t>(old_idx);
            const size_t idx2 = (idx1 < old_height - 1) ? idx1 + 1 : idx1;
            const double t = old_idx - idx1;

            // Интерполяция значений
            if (idx1 == idx2) {
                new_data[y * width + x] = data_[idx1][x];
            } else {
                SlopeInterpolator interpolator(data_[idx1][x], data_[idx2][x]);
                new_data[y * width + x] = static_cast<dpx_data::DataType>(interpolator.Interpolate(t));
            }
        }
    }
    data_.data.swap(new_data);
    data_.need_redraw = true;
    return true;
}
