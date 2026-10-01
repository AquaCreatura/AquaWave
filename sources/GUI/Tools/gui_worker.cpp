#include "gui_worker.h"
#include <math.h>
#include <algorithm>
using namespace aqua_gui;
bool aqua_gui::ZoomFromWheelDelta(ChartScaleInfo & scale_info, const int wheel_delta, const QPoint scale_point)
{
    auto& min_max_bounds         = scale_info.val_info_.min_max_bounds;
    auto& cur_bounds      = scale_info.val_info_.view_bounds;
    auto& px_info = scale_info.pix_info_;

    bool is_y_scale = scale_point.x() > px_info.chart_size_px.hor;
    bool is_x_scale = !is_y_scale;
    
    // Изменён коэффициент масштабирования для более плавного изменения
	double steps = wheel_delta / 120.;
	double scale_koeff = std::abs(1. - std::pow(1.1, steps * 0.8));

    const double direction = wheel_delta > 0 ? 1.0 : -1.0;

    // Вычисление коэффициентов масштабирования
    const double val_x_scale_koeff = (min_max_bounds.hor.delta()) / (cur_bounds.hor.delta());

    const double val_y_scale_koeff = (min_max_bounds.vert.delta()) / (cur_bounds.vert.delta());
    bool values_changed = false;
	const auto zoom_threshold = scale_info.val_info_.max_zoom_koeffs;
    // Масштабирование по оси X
    if (is_x_scale)
    {
        auto &cur_hor       = cur_bounds.hor;
        auto &min_max_hor   = min_max_bounds.hor;
        // Проверяем, можем ли мы увеличивать/уменьшать масштаб
        bool can_zoom_in = direction > 0 && val_x_scale_koeff < zoom_threshold.hor;
        bool can_zoom_out = direction < 0;
        
        if (can_zoom_in || can_zoom_out)
        {
            int x_mouse_pos_px = scale_point.x();
            double x_val_to_px_koeff = cur_hor.delta() / px_info.chart_size_px.hor;
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
                if (new_high - new_low >= std::numeric_limits<double>::epsilon())
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
        bool can_zoom_in = direction > 0 && val_y_scale_koeff < zoom_threshold.vert;
        bool can_zoom_out = direction < 0;
        auto &cur_vert      = cur_bounds.vert;
        auto &min_max_vert  = min_max_bounds.vert;
        if (can_zoom_in || can_zoom_out)
        {
            int y_mouse_pos_px = scale_point.y();
            double y_val_to_px_koeff = (cur_vert.delta()) / px_info.chart_size_px.vert;
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

bool aqua_gui::PanFromMouse(ChartScaleInfo& scale_info,
	const QPoint /*start_mouse_point*/,               // не используется, но оставлен для интерфейса
	const HV_Info<double, double>& world_pos,        // зафиксированная мировая точка
	const QPoint end_mouse_point)
{
	auto& min_max_bounds = scale_info.val_info_.min_max_bounds;
	auto& cur_bounds = scale_info.val_info_.view_bounds;
	const auto& px_info = scale_info.pix_info_;

	bool changed = false;

	// --- Ось X ---
	{
		auto& cur_limits = cur_bounds.hor;
		const auto& min_limits = min_max_bounds.hor;
		const double width = px_info.chart_size_px.hor;
		const double delta = cur_limits.delta(); // длина интервала не меняется

												 // Вычисляем положение курсора в долях от ширины (от 0 до 1)
		double t_x = std::clamp(double(end_mouse_point.x()) / width, 0.0, 1.0);

		// Новая нижняя граница: чтобы world_pos.hor соответствовало пикселю t_x
		double new_low = world_pos.hor - t_x * delta;
		double new_high = new_low + delta;

		// Ограничиваем, чтобы не выйти за глобальные пределы
		const double min_low = min_limits.low;
		const double max_high = min_limits.high;
		if (new_low < min_low) {
			new_low = min_low;
			new_high = min_low + delta;
		}
		if (new_high > max_high) {
			new_high = max_high;
			new_low = max_high - delta;
		}
		// Если new_low всё ещё меньше min_low или new_high больше max_high – корректируем
		if (new_low < min_low) {
			new_low = min_low;
			new_high = min_low + delta;
		}
		if (new_high > max_high) {
			new_high = max_high;
			new_low = max_high - delta;
		}

		// Применяем изменения
		if (std::abs(cur_limits.low - new_low) > std::numeric_limits<double>::epsilon() ||
			std::abs(cur_limits.high - new_high) > std::numeric_limits<double>::epsilon())
		{
			cur_limits.low = new_low;
			cur_limits.high = new_high;
			changed = true;
		}
	}

	// --- Ось Y ---
	{
		auto& cur_limits = cur_bounds.vert;
		const auto& min_limits = min_max_bounds.vert;
		const double height = px_info.chart_size_px.vert;
		const double delta = cur_limits.delta();

		// Положение курсора по Y: y=0 соответствует high, y=height соответствует low
		double t_y = std::clamp(double(end_mouse_point.y()) / height, 0.0, 1.0);
		// Доля от low до high: (1 - t_y) – при t_y=0 (верх) это 1 (high), при t_y=1 (низ) это 0 (low)
		double fraction_from_low = 1.0 - t_y;

		// Новая нижняя граница: world_pos.vert = new_low + fraction_from_low * delta
		double new_low = world_pos.vert - fraction_from_low * delta;
		double new_high = new_low + delta;

		// Ограничиваем
		const double min_low = min_limits.low;
		const double max_high = min_limits.high;
		if (new_low < min_low) {
			new_low = min_low;
			new_high = min_low + delta;
		}
		if (new_high > max_high) {
			new_high = max_high;
			new_low = max_high - delta;
		}
		if (new_low < min_low) {
			new_low = min_low;
			new_high = min_low + delta;
		}
		if (new_high > max_high) {
			new_high = max_high;
			new_low = max_high - delta;
		}

		if (std::abs(cur_limits.low - new_low) > std::numeric_limits<double>::epsilon() ||
			std::abs(cur_limits.high - new_high) > std::numeric_limits<double>::epsilon())
		{
			cur_limits.low = new_low;
			cur_limits.high = new_high;
			changed = true;
		}
	}

	return changed;
}
void aqua_gui::AdaptVertPowerBounds(ChartScaleInfo & scale_info)
{
	auto new_bounds = scale_info.power_bounds_;
	if (new_bounds.delta() == 0)
		return;

    // Получаем ссылку на текущие максимально допустимые (автоматические) границы шкалы
    auto &vert_min_max = scale_info.val_info_.min_max_bounds.vert;

	// Получаем ссылку на текущие отображаемые границы шкалы (которые видит пользователь)
	auto &vert_cur = scale_info.val_info_.view_bounds.vert;

	const double min_epsilon = new_bounds.delta() * 0.05;
    // Если автоматические границы изменились, обновляем шкалу
	if (std::abs(vert_min_max.low - new_bounds.low) > min_epsilon ||
		std::abs(vert_min_max.high - new_bounds.high) > min_epsilon)
	{


		//Корретируем отображаемое
		if (!scale_info.val_info_.need_reset_scale) {

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
			scale_info.val_info_.need_reset_scale = false;
		}
	}
	else
		int a = 1;
}

const double aqua_gui::GetAsimDrawPlace(const int iteration_counter)
{
	// 1) Определяем уровень (слой в бинарном дереве)
	const int power = static_cast<int>(std::log2(iteration_counter + 1));
	const int count = 1 << power;             // число элементов на этом уровне
	const int min_elem = count - 1;           // индекс первого элемента этого уровня

	// 2) Индекс внутри уровня от 0 до count-1
	const int j = iteration_counter - min_elem;

	// 3) Вычисляем "последовательный" индекс с чередованием краевых:
	//    j=0→0 (левый), j=1→count-1 (правый), j=2→1, j=3→count-2 и т.д.
	int seq;
	if ((j & 1) == 0) {
		// чётный j: 0,2,4… → слева направо 0,1,2…
		seq = j / 2;
	}
	else {
		// нечётный j: 1,3,5… → справа налево count-1, count-2…
		seq = (count - 1) - (j / 2);
	}

	// 4) Приводим seq в центр ячейки [0,1]:
	const double step = 1.0 / count;
	return step * seq + step * 0.5;
}


std::vector<int> aqua_gui::GetAssimLocationsVec(const int width)
{
	// Вектор пар (позиция, итерация)
	std::vector<std::pair<double, int>> place_iteration_pairs;
	for (int i = 0; i < width; ++i) {
		double place = GetAsimDrawPlace(i);
		place_iteration_pairs.push_back({ place, i });
	}

	// Сортировка по позициям, при равенстве — по номеру итерации
	std::sort(place_iteration_pairs.begin(), place_iteration_pairs.end());

	// Создание результирующего вектора
	std::vector<int> result(width);
	for (int k = 0; k < width; ++k) {
		int iteration = place_iteration_pairs[k].second;
		result[iteration] = k; // Индекс — итерация, значение — местоположение
	}

	return result;
}



FpsEstimator::FpsEstimator(double smoothingWindowSec)
	: m_smoothingWindowSec(smoothingWindowSec)
	, m_isRunning(false)
{
}

void FpsEstimator::MarkNewFrame()
{
	if (!m_isRunning) {
		m_timer.start();
		m_isRunning = true;
	}

	qint64 now = m_timer.elapsed();
	m_frameTimestamps.push_back(now);

	// Удаляем кадры, которые вышли за пределы окна усреднения
	qint64 cutoff = now - static_cast<qint64>(m_smoothingWindowSec * 1000);
	while (!m_frameTimestamps.empty() && m_frameTimestamps.front() < cutoff) {
		m_frameTimestamps.pop_front();
	}
}

double FpsEstimator::GetCurrentFps() const
{
	if (m_frameTimestamps.size() < 2) {
		return 0.0;
	}

	// Время между первым и последним кадром в окне
	qint64 timeSpanMs = m_frameTimestamps.back() - m_frameTimestamps.front();

	if (timeSpanMs <= 0) {
		return 0.0;
	}

	// Количество интервалов между кадрами
	int intervals = static_cast<int>(m_frameTimestamps.size()) - 1;

	// FPS = количество интервалов / время в секундах
	double timeSpanSec = static_cast<double>(timeSpanMs) / 1000.0;
	return static_cast<double>(intervals) / timeSpanSec;
}

double FpsEstimator::GetAvgFrameTimeMs() const
{
	double fps = GetCurrentFps();
	if (fps > 0) {
		return 1000.0 / fps;
	}
	return 0.0;
}
