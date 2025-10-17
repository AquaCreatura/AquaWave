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
                QString str_value = QString::number(hor_line.value_, 'f', dot_power_string);
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
                QString str_value = QString::number(vert_line.value_, 'f', dot_power_string);
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
    // Если значения инвертированы
    const bool is_inverted = val_bounds.high < val_bounds.low;
    // Разница между началом и концом
    const double delta_val = is_inverted ? val_bounds.low - val_bounds.high : val_bounds.high - val_bounds.low;
    // Максимальное количество меток
    const int max_marks_count = (distance_px_ / range_between_lines_px_) + (distance_px_ % range_between_lines_px_ > 0);
    // Минимальная разница значений на метку
    double values_in_mark_min = delta_val / max_marks_count;        

    // Предполагаемый порядок 10 (значение должно быть больше минимального)
    const int adapted_ext = int(log10(values_in_mark_min)) + (values_in_mark_min >= 1.0);
    // Шаг между значениями
    double adapted_step = pow(10, adapted_ext);
    if (!adapted_step) return false;
    // Получили адаптированный шаг
    
    int denominator = 1;
    // Находим наиболее подходящий делитель
    if (adapted_step / 2 >= values_in_mark_min)
    {   
        if (adapted_step / 4 >= values_in_mark_min)
        {
            denominator = 4;
        }
        else
        {
            denominator = 2;
        }   
    }

    adapted_step /= denominator; 
    const double res_count_of_lines = delta_val / adapted_step;
    // Если линий недостаточно — увеличиваем количество текстовых меток
    const int lines_per_text = (res_count_of_lines >= 5) ? 2 : 1;
    const double string_step = adapted_step * lines_per_text;

    grid_lines_delta_ = string_step;

    // Получаем начальное значение, с которого начинаем рисовать
    double start_shift = fmod(val_bounds.low, adapted_step);
    // Если есть остаток — переходим к ближайшему значению
    if (start_shift) start_shift = is_inverted ? -start_shift : adapted_step - start_shift;
    double start_value = val_bounds.low + start_shift;

    // Из значений в пиксели
    const double x_scale_koeff = distance_px_ / delta_val; 
    
    // Правая граница для текста
    const int right_text_margin_pos = distance_px_ > empty_margin_.second ? distance_px_ - empty_margin_.second : 0;
    // Проходим по всем значениям
    double end_value = is_inverted ? val_bounds.high : val_bounds.high;
    double step_direction = is_inverted ? -adapted_step : adapted_step;
    for (double value_iterator = start_value; 
         is_inverted ? value_iterator > end_value : value_iterator < end_value; 
         value_iterator += step_direction)
    {
        // Получаем пиксельное значение
        const int cur_pixel = static_cast<int>((value_iterator - val_bounds.low) * x_scale_koeff);
        // Если линия на границе — пропускаем
        if (!IsInBound(5, cur_pixel, distance_px_ - 5))
            continue;
        // Является ли текущая линия текстовой
        LineInfo cur_line_info;
        cur_line_info.is_text_line_ = 
        (
            (std::min(fmod(value_iterator, string_step), string_step - fmod(value_iterator, string_step)) < (string_step / 10)) &&
            IsInBound(empty_margin_.first, cur_pixel, right_text_margin_pos)
        );

        cur_line_info.pixel_number_ = cur_pixel;
        // Инициализируем поле значения
        if (cur_line_info.is_text_line_)
            cur_line_info.value_ = value_iterator;
        // Добавляем в вектор
        grid_lines_.push_back(cur_line_info);
    }
    return true;
}

int AxisManager::LinesInfo::GetFloatStringPower()
{
    return std::max(1., -1 * log10(grid_lines_delta_) + 1);
}
