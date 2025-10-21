#include "gui_helper.h"
#include <math.h>


bool aqua_gui::ZoomFromWheelDelta(ChartScaleInfo & scale_info, const int wheel_delta, const QPoint scale_point)
{
    auto& min_max_bounds         = scale_info.val_info_.min_max_bounds_;
    auto& cur_bounds      = scale_info.val_info_.cur_bounds;
    auto& px_info = scale_info.pix_info_;

    bool is_y_scale = scale_point.x() > px_info.chart_size_px.horizontal;
    bool is_x_scale = !is_y_scale;
    
    // Изменён коэффициент масштабирования для более плавного изменения
    const double scale_koeff = std::abs(wheel_delta) / 5000.0; // Уменьшен коэффициент для более плавного масштабирования
    const double direction = wheel_delta > 0 ? 1.0 : -1.0;

    // Вычисление коэффициентов масштабирования
    const double val_x_scale_koeff = (min_max_bounds.horizontal.delta()) / (cur_bounds.horizontal.delta());

    const double val_y_scale_koeff = (min_max_bounds.vertical.delta()) / (cur_bounds.vertical.delta());
    bool values_changed = false;
	const auto zoom_threshold = scale_info.val_info_.max_zoom_koeffs_;
    // Масштабирование по оси X
    if (is_x_scale)
    {
        auto &cur_hor       = cur_bounds.horizontal;
        auto &min_max_hor   = min_max_bounds.horizontal;
        // Проверяем, можем ли мы увеличивать/уменьшать масштаб
        bool can_zoom_in = direction > 0 && val_x_scale_koeff < zoom_threshold.horizontal;
        bool can_zoom_out = direction < 0;
        
        if (can_zoom_in || can_zoom_out)
        {
            int x_mouse_pos_px = scale_point.x();
            double x_val_to_px_koeff = cur_hor.delta() / px_info.chart_size_px.horizontal;
            double x_mouse_pos_val = cur_hor.low + x_mouse_pos_px * x_val_to_px_koeff;

            // Вычисляем новые границы с учётом направления масштабирования
            double left_change = (x_mouse_pos_val - cur_hor.low) * scale_koeff * direction;
            double right_change = (cur_hor.high - x_mouse_pos_val) * scale_koeff * direction;

            // Применяем изменения
            double new_low = (cur_hor.low + left_change);
            double new_high =(cur_hor.high - right_change);

            // Проверяем, чтобы новые границы были валидными
            if (new_low < new_high)
            {
                // Ограничиваем минимальный и максимальный масштаб
                new_low = std::max(new_low, min_max_hor.low);
                new_high = std::min(new_high, min_max_hor.high);

                // Проверяем минимальный размах (не меньше 1 единицы)
                if (new_high - new_low >= 0.01)
                {
                    cur_hor = {new_low, new_high};
                    values_changed = true;
                }
            }
        }
    }

    // Масштабирование по оси Y (аналогично оси X)
    if (is_y_scale)
    {
        bool can_zoom_in = direction > 0 && val_y_scale_koeff < zoom_threshold.vertical;
        bool can_zoom_out = direction < 0;
        auto &cur_vert      = cur_bounds.vertical;
        auto &min_max_vert  = min_max_bounds.vertical;
        if (can_zoom_in || can_zoom_out)
        {
            int y_mouse_pos_px = scale_point.y();
            double y_val_to_px_koeff = (cur_vert.delta()) / px_info.chart_size_px.vertical;
            double y_mouse_pos_val = cur_vert.high - y_mouse_pos_px * y_val_to_px_koeff;

            double bottom_change = (y_mouse_pos_val - cur_vert.low) * scale_koeff * direction;
            double top_change = (cur_vert.high - y_mouse_pos_val) * scale_koeff * direction;

            double new_low  = cur_vert.low + bottom_change;
            double new_high = cur_vert.high - top_change;

            if (new_low < new_high)
            {
                new_low = std::max(new_low, min_max_vert.low);
                new_high = std::min(new_high, min_max_vert.high);

                if (new_high - new_low >= std::numeric_limits<double>::epsilon())
                {
                    cur_vert = {new_low, new_high};
                    values_changed = true;
                }
            }
        }
    }
    return values_changed;
}

void aqua_gui::AdaptPowerBounds(ChartScaleInfo & scale_info, const Limits<double>& new_bounds)
{
    // Получаем ссылку на текущие максимально допустимые (автоматические) границы шкалы
    auto &vert_min_max = scale_info.val_info_.min_max_bounds_.vertical;

	const double min_epsilon = new_bounds.delta() * 0.05;
    // Если автоматические границы изменились, обновляем шкалу
    if(std::abs(vert_min_max.low - new_bounds.low) > min_epsilon || 
		std::abs(vert_min_max.high - new_bounds.high) > min_epsilon )
    {

        // Получаем ссылку на текущие отображаемые границы шкалы (которые видит пользователь)
        auto &vert_cur = scale_info.val_info_.cur_bounds.vertical;
		//Корретируем отображаемое
		if (!scale_info.val_info_.need_reset_scale_) {

			// Вычисляем текущий коэффициент масштабирования (зума) по вертикали
			const double zoom_vert_koeff = vert_cur.delta() / vert_min_max.delta();
			// Рассчитываем новую высоту (диапазон) для отображаемой шкалы, сохраняя зум
			const double new_height = (new_bounds.high - new_bounds.low) * zoom_vert_koeff;
			// Вычисляем текущий центр отображаемой шкалы
			double zoom_centre = (vert_cur.high + vert_cur.low) / 2;
			// Корректируем центр зума, чтобы отображаемый диапазон не вышел за новые автоматические границы
			zoom_centre = qBound(new_bounds.low + new_height / 2, zoom_centre, new_bounds.high - new_height / 2);

			// Обновляем максимально допустимые (автоматические) границы
			vert_min_max = new_bounds;
			// Обновляем текущие отображаемые границы с учетом нового центра и высоты
			vert_cur = { zoom_centre - new_height / 2, zoom_centre + new_height / 2 };
		}
		else {
			// Обновляем максимально допустимые (автоматические) границы
			vert_min_max = new_bounds;
			vert_cur = new_bounds;
			scale_info.val_info_.need_reset_scale_ = false;
		}        
    }
}

