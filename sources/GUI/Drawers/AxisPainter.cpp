#include "AxisPainter.h"
using namespace aqua_gui;
constexpr int hatch_size_px_c_expr  =  10; //Length of hatch (штрих)


AxisManager::AxisManager(const ChartScaleInfo & base_scale_info):
scale_info_(base_scale_info)
{
}

void AxisManager::InitHorizontal(const QString & ox_name)
{
    //need be updated, only if there is a new unique text
    if(!ox_name.isEmpty() && (ox_name != axis_.horizontal.qstr_suffix)) 
    {
        axis_.horizontal.qstr_suffix = ox_name;
        need_be_updated_             = true;
    }
}

void AxisManager::InitVertical(const QString & oy_name)
{
    //need be updated, only if there is a new unique text
    if(!oy_name.isEmpty() && (oy_name != axis_.vertical.qstr_suffix)) 
    {
        axis_.vertical.qstr_suffix   = oy_name;
        need_be_updated_             = true;
    }
}
bool AxisManager::DrawAxis(QPainter& passed_painter)
{
    // Если ничего не изменилось — рисуем кэшированное изображение
    if (!ShouldRedraw()) 
    {
        passed_painter.drawPixmap(0, 0, cache_pixmap_);
        return true;
    }

    // Пересоздаём Pixmap, если размеры изменились
    if (last_widget_size_ != scale_info_.pix_info_.widget_size_px)
    {
        cache_pixmap_ = QPixmap(scale_info_.pix_info_.widget_size_px.horizontal,
                                scale_info_.pix_info_.widget_size_px.vertical);
        last_widget_size_ = scale_info_.pix_info_.widget_size_px;
    }
    // Заполняем прозрачным фоном
    cache_pixmap_.fill(Qt::transparent);
    // Привязываем QPainter к Pixmap
    QPainter cur_painter(&cache_pixmap_);
	if(scale_info_.val_info_.cur_bounds.vertical.high == 100) 
		cache_pixmap_.fill(Qt::transparent);
    // Инициализация перьев
    QPen text_pen;
    text_pen.setColor("gray");

    QPen grid_pen;
    grid_pen.setColor(QColor::fromRgb(0, 40, 25, 255));

    QPen frame_pen;
    frame_pen.setColor(grid_pen.color());
    frame_pen.setWidth(3);

    DrawMarginBackGround(cur_painter, frame_pen);

    // Горизонтальная ось
    {
        Limits<double> scaled_bounds = scale_info_.val_info_.cur_bounds.horizontal; 

        axis_.horizontal.grid_info.FillLineVectors(scale_info_.pix_info_.chart_size_px.horizontal, scaled_bounds);
        
        auto& cur_line_info = axis_.horizontal.grid_info;
        const int axis_height = scale_info_.pix_info_.chart_size_px.vertical;
        const int axis_width = scale_info_.pix_info_.chart_size_px.horizontal;
        // Получаем порядок для форматирования чисел с плавающей точкой
        const int dot_power_string = axis_.horizontal.grid_info.GetFloatStringPower();

        // Подпись оси
        {
            cur_painter.setPen(text_pen);
            cur_painter.drawText(5, scale_info_.pix_info_.widget_size_px.vertical - 5, 
                                axis_.horizontal.qstr_suffix);
        }

        // Проходим по всем линиям сетки
        for (const auto& hor_line : cur_line_info.grid_lines_)
        {
            // Точка на оси
            const QPoint axis_point = {hor_line.pixel_number_, axis_height};
            // Рисуем линию сетки
            {
                const QPoint end_point = {hor_line.pixel_number_, 0};
                cur_painter.setPen(grid_pen);
                cur_painter.drawLine(axis_point, end_point);
            }
            // Рисуем метку
            {
                const QPoint mark_point = {hor_line.pixel_number_, axis_height + hatch_size_px_c_expr};
                cur_painter.setPen(frame_pen);
                cur_painter.drawLine(axis_point, mark_point);
            }
            // Если линия сопровождается текстом — отображаем его
            if (hor_line.is_text_line_)
            {
                cur_painter.setPen(text_pen);
				auto casted_string = aqua_parse_tools::ValueToString(hor_line.value_, dot_power_string);
				QString str_value(casted_string.c_str());
                cur_painter.drawText(QPoint(axis_point.x() - 10, axis_point.y() + 20), str_value);
            }
        }
    }

    // Вертикальная ось
    {
        axis_.vertical.grid_info.FillLineVectors(scale_info_.pix_info_.chart_size_px.vertical, 
                                                        scale_info_.val_info_.cur_bounds.vertical);
        auto& cur_line_info = axis_.vertical.grid_info;
        const int axis_height = scale_info_.pix_info_.chart_size_px.vertical;
        const int axis_width = scale_info_.pix_info_.chart_size_px.horizontal;
        // Получаем порядок для форматирования чисел с плавающей точкой
        const int dot_power_string = axis_.vertical.grid_info.GetFloatStringPower();

        // Подпись оси
        {
            cur_painter.setPen(text_pen);
            cur_painter.drawText(axis_width + 5, 15, axis_.vertical.qstr_suffix);
        }
        // Проходим по всем линиям сетки
        for (const auto& vert_line : cur_line_info.grid_lines_)
        {
            // Точка на оси
            const QPoint axis_point = {axis_width, scale_info_.pix_info_.chart_size_px.vertical - vert_line.pixel_number_};

            // Рисуем линию сетки
            {
                const QPoint end_point = {0, axis_point.y()};
                cur_painter.setPen(grid_pen);
                cur_painter.drawLine(axis_point, end_point);
            }
            // Рисуем метку
            {
                const QPoint mark_point = {axis_width + hatch_size_px_c_expr, axis_point.y()};
                cur_painter.setPen(frame_pen);
                cur_painter.drawLine(axis_point, mark_point);
            }
            // Если линия сопровождается текстом — отображаем его
            if (vert_line.is_text_line_)
            {
                cur_painter.setPen(text_pen);
				auto casted_string = aqua_parse_tools::ValueToString(vert_line.value_, dot_power_string);
				QString str_value(casted_string.c_str());
                cur_painter.drawText(QPoint(axis_point.x() + 5, axis_point.y() - 5), str_value);
            }
        }
    }

    passed_painter.drawPixmap(0, 0, cache_pixmap_);
    need_be_updated_        = false;
    last_val_scaled_bounds_ = scale_info_.val_info_.cur_bounds;
    return true;
}

bool aqua_gui::AxisManager::ShouldRedraw() const
{
    return need_be_updated_ || last_widget_size_!=scale_info_.pix_info_.widget_size_px 
                || last_val_scaled_bounds_ != scale_info_.val_info_.cur_bounds;
}

bool AxisManager::DrawMarginBackGround(QPainter& passed_painter, const QPen& frame_pen)
{   

    QGradient strange_grad(QGradient::PremiumDark);
    const auto &pix_info  = scale_info_.pix_info_;
    QPainterPath polyg;
/*                              
                       ____D__
                      |       |
                      |       |
                     C|       |
                      |       |E
     _____B___________|       |
    |                         |
  H |                         |
    |_________________________|
                    F
*/


    //start point (H <--> F)
    polyg.moveTo(0                                 , pix_info.widget_size_px.vertical);
    //H stage
    polyg.lineTo(0                                 , pix_info.chart_size_px .vertical);
    //B stage
    polyg.lineTo(pix_info.chart_size_px .horizontal, pix_info.chart_size_px .vertical);
    //C stage    
    polyg.lineTo(pix_info.chart_size_px .horizontal, 0                               );
    //D stage    
    polyg.lineTo(pix_info.widget_size_px.horizontal, 0                               );
    //E stage    
    polyg.lineTo(pix_info.widget_size_px.horizontal, pix_info.widget_size_px.vertical);
    //F stage
    polyg.lineTo(0                                 , pix_info.widget_size_px.vertical);
    //Fill this polygon
    passed_painter.fillPath(polyg, strange_grad);
    //Frame around margin axises 
    {
        passed_painter.setPen  (frame_pen);
        passed_painter.drawLine(pix_info.chart_size_px.horizontal, pix_info.chart_size_px.vertical , 0                                , pix_info.chart_size_px.vertical);
        passed_painter.drawLine(pix_info.chart_size_px.horizontal, pix_info.chart_size_px.vertical , pix_info.chart_size_px.horizontal, 0                              );

    }
        
    return true;
}


//Is our value_ in bound 
template<typename W>
bool IsInBound(const W left_val, const W passed_val, const W right_val)
{
    return (passed_val > left_val) && (passed_val < right_val);
}

bool AxisManager::LinesInfo::FillLineVectors(const int distance_px_, const Limits<double>& val_bounds)
{
	grid_lines_.clear();

	// ---------------------------------------------------------------------
	// 1. Подготовительные вычисления
	// ---------------------------------------------------------------------
	const bool is_inverted = val_bounds.high < val_bounds.low;
	const double low = is_inverted ? val_bounds.high : val_bounds.low;
	const double high = is_inverted ? val_bounds.low : val_bounds.high;
	const double delta_val = high - low;
	if (delta_val <= 0.0 || distance_px_ <= 0)
		return false;                       // некорректные данные

											// Максимально допустимое количество линий, исходя из пиксельного шага
	const int max_lines_count = distance_px_ / range_between_lines_px_ + 1;

	// ---------------------------------------------------------------------
	// 2. Выбор оптимального шага между линиями (step)
	// ---------------------------------------------------------------------
	// Желаемое количество линий (обычно 5-10, но не больше max_lines_count)
	const int desired_lines_count = std::min(10, max_lines_count);
	const double rough_step = delta_val / desired_lines_count;

	// Функция подбирает "красивый" шаг, кратный 1,2,5 * 10^n
	auto nice_step = [](double x) -> double {
		if (x <= 0.0) return 1.0;
		double exponent = floor(log10(x));
		double mantissa = x / pow(10, exponent);
		// выбираем ближайшее из набора {1, 2, 5}
		if (mantissa < 1.5)      mantissa = 1.0;
		else if (mantissa < 3.0) mantissa = 2.0;
		else if (mantissa < 7.0) mantissa = 5.0;
		else                     mantissa = 10.0;
		return mantissa * pow(10, exponent);
	};

	double step = nice_step(rough_step);

	// Убедимся, что при этом шаге количество линий не превышает максимум
	int lines_count = static_cast<int>(delta_val / step) + 1;
	if (lines_count > max_lines_count) {
		// Если линий слишком много, увеличиваем шаг до следующего "красивого"
		step = nice_step(step * 1.1);   // небольшое смещение, чтобы гарантированно перейти на следующий уровень
		lines_count = static_cast<int>(delta_val / step) + 1;
	}

	// ---------------------------------------------------------------------
	// 3. Определение шага для текстовых подписей (text_step)
	// ---------------------------------------------------------------------
	const int lines_per_text = (lines_count >= 5) ? 2 : 1; 
	const double text_step = step * lines_per_text;

	grid_lines_delta_ = text_step;   // сохраняем для внешнего использования

									 // ---------------------------------------------------------------------
									 // 4. Генерация линий
									 // ---------------------------------------------------------------------
	const double x_scale = distance_px_ / delta_val;
	const int right_text_limit = (distance_px_ > empty_margin_.second)
		? distance_px_ - empty_margin_.second
		: 0;

	// Находим первую линию, не меньшую low и кратную step
	double first_line = low;
	double remainder = fmod(low, step);
	if (std::abs(remainder) > 1e-12) {               // учитываем погрешность
		if (remainder < 0)
			first_line = low - remainder;            // для отрицательных: low - (-rem) = low + |rem|
		else
			first_line = low + (step - remainder);
	}

	// Проходим по всем линиям от first_line до high включительно
	for (double value = first_line; value <= high + 1e-12; value += step) {
		// Контроль выхода за границы из-за погрешностей
		if (value < low - 1e-12 || value > high + 1e-12)
			continue;

		// Пиксельная координата
		int pixel = static_cast<int>((value - low) * x_scale);
		if (!IsInBound(5, pixel, distance_px_ - 5))
			continue;   // слишком близко к краям

		LineInfo info;
		info.pixel_number_ = pixel;

		// Проверка, должна ли линия иметь текстовую подпись
		// Линия считается текстовой, если она близка к целому кратному text_step
		double rem = fmod(value, text_step);
		// Приводим остаток к положительному значению в [0, text_step)
		if (rem < 0) rem += text_step;
		double dist_to_multiple = std::min(rem, text_step - rem);
		if (dist_to_multiple < text_step * 0.1) {   // допуск 10% от шага
													// Дополнительно проверяем, попадает ли подпись в область без отступов
			if (IsInBound(empty_margin_.first, pixel, right_text_limit)) {
				info.is_text_line_ = true;
				info.value_ = value;                 // сохраняем значение для подписи
			}
		}

		grid_lines_.push_back(info);
	}

	return true;
}
int AxisManager::LinesInfo::GetFloatStringPower()
{
    return std::max(0., -1 * log10(grid_lines_delta_) + 1);
}
