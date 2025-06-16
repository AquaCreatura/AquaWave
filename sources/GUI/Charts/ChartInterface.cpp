#include "ChartInterface.h"
#include <qsizepolicy.h>
ChartInterface::ChartInterface(QWidget* parent) : 
    QWidget(parent), axis_man_(scale_info_), bg_image_(scale_info_)
{ 
    // Our widget params
    this->setMouseTracking(true);
    this->setMinimumSize(200, 100);
    // Set min max values for our chart
    SetHorizontalMinMaxBounds(50'000, 200'000);
    SetHorizontalSuffix("counts");

    SetVerticalMinMaxBounds(0, 100, true);
    SetVerticalSuffix("power");
    connect(&redraw_timer_, &QTimer::timeout, this, &ChartInterface::OnTimeoutRedraw);
    redraw_timer_.start(100);

}

ChartInterface::~ChartInterface()
{
}

bool ChartInterface::SetBackgroundImage(const QString & image_path)
{
    const bool res_of_init  = bg_image_.InitImage(image_path);
    return res_of_init;
}

// Установка границ по горизонтали
void ChartInterface::SetHorizontalMinMaxBounds(const double min_val, const double end_val)
{
    Limits<double> new_horizontal_bounds = {min_val, end_val};
    auto& cur_val_info = scale_info_.val_info_.min_max_bounds_.horizontal;
    // Если ничего не изменилось — выходим
    if (cur_val_info == new_horizontal_bounds) return;

    cur_val_info = new_horizontal_bounds;
    scale_info_.val_info_.cur_bounds.horizontal = new_horizontal_bounds;
}

void ChartInterface::SetHorizontalSuffix(const QString & suffix)
{
    //Only axis pixmap should be redrawn
    axis_man_.InitHorizontal(suffix);
}


// Установка границ по вертикали
void ChartInterface::SetVerticalMinMaxBounds(const double min_val, const double end_val, const bool is_adaptive)
{
    Limits<double> new_vertical_bounds = {min_val, end_val};
    auto& cur_val_info = scale_info_.val_info_.min_max_bounds_.vertical;
    // Если ничего не изменилось — выходим
    if (cur_val_info == new_vertical_bounds) return;

    cur_val_info = new_vertical_bounds;
    scale_info_.val_info_.cur_bounds.vertical = new_vertical_bounds;
}

void ChartInterface::SetVerticalSuffix(const QString & suffix)
{
    //Only axis pixmap should be redrawn
    axis_man_.InitVertical(suffix);
}


std::shared_ptr<ChartSelection> ChartInterface::GetSelection()
{
    return chart_selection_;
}

void ChartInterface::mousePressEvent(QMouseEvent * mouse_event)
{
}


void ChartInterface::mouseMoveEvent(QMouseEvent * mouse_event)
{
    mouse_pos_ = mouse_event->pos();
    //this->update();
}

void ChartInterface::mouseReleaseEvent(QMouseEvent * mouse_event)
{
}

void ChartInterface::wheelEvent(QWheelEvent* wheel_event)
{
    auto& min_max_bounds         = scale_info_.val_info_.min_max_bounds_;
    auto& cur_bounds      = scale_info_.val_info_.cur_bounds;
    auto& px_info = scale_info_.pix_info_;

    QPoint scale_point = wheel_event->pos();   
    bool is_y_scale = scale_point.x() > px_info.chart_size_px.horizontal;
    bool is_x_scale = !is_y_scale;
    const auto wheel_delta = wheel_event->angleDelta().y();
    
    // Изменён коэффициент масштабирования для более плавного изменения
    const double scale_koeff = std::abs(wheel_delta) / 5000.0; // Уменьшен коэффициент для более плавного масштабирования
    const double direction = wheel_delta > 0 ? 1.0 : -1.0;

    // Вычисление коэффициентов масштабирования
    const double val_x_scale_koeff = (min_max_bounds.horizontal.delta()) / (cur_bounds.horizontal.delta());

    const double val_y_scale_koeff = (min_max_bounds.vertical.delta()) / (cur_bounds.vertical.delta());
    bool values_changed = false;
    // Масштабирование по оси X
    if (is_x_scale)
    {
        auto &cur_hor       = cur_bounds.horizontal;
        auto &min_max_hor   = min_max_bounds.horizontal;
        // Проверяем, можем ли мы увеличивать/уменьшать масштаб
        bool can_zoom_in = direction > 0 && val_x_scale_koeff < 20;
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
                if (new_high - new_low >= 1)
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
        bool can_zoom_in = direction > 0 && val_y_scale_koeff < 20;
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

    if (values_changed)
    {
        update();
    }
}

void ChartInterface::resizeEvent(QResizeEvent * resize_event)
{   
    //resize_event->
    //this->setFixedSize(resize_event->size());
}

void ChartInterface::paintEvent(QPaintEvent * paint_event)
{
    //if widget size was changed
    UpdateWidgetSizeInfo();
    UpdatePowerBounds();

    
    QPainter new_frame_painter(this);
    //Background Image
    {
        //Draw our background
        bg_image_.DrawImage(new_frame_painter);
    }
    //Axis + Grid
    {
        bool res = axis_man_.DrawAxis(new_frame_painter);
    }
    //Data
    {
        DrawData(new_frame_painter);
    }
    //Static selections
    {
    
    
    }
    //Selection
    {
    
    }
    //Mouse Pos
    {
    
    }
}

void ChartInterface::UpdatePowerBounds()
{
}

void ChartInterface::UpdateWidgetSizeInfo()
{
    aqua_gui::WH_Info<int> cur_size = {this->width(), this->height()};
    auto        &pix_info = scale_info_.pix_info_;
    //if data is not changed - go away (don't touch anything)
    if(pix_info.widget_size_px == cur_size) return;

    pix_info.widget_size_px = cur_size;
    pix_info.chart_size_px  = pix_info.widget_size_px  - pix_info.margin_px;    
}

void ChartInterface::OnTimeoutRedraw()
{
    update();
}
