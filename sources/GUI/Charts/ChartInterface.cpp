#include "ChartInterface.h"
#include <qsizepolicy.h>
#include "GUI/basic tools/gui_helper.h"
ChartInterface::ChartInterface(QWidget* parent) : 
    QWidget(parent), axis_man_(scale_info_), bg_image_(scale_info_)
{ 
    // Our widget params
    this->setMouseTracking(true);
    this->setMinimumSize(200, 100);
    // Set min max values for our chart
    SetHorizontalMinMaxBounds(50'000, 200'000);
    SetHorizontalSuffix("counts");

    SetVerticalMinMaxBounds(20, 40, true);
    SetVerticalSuffix("power");
    connect(&redraw_timer_, &QTimer::timeout, this, QOverload<>::of(&ChartInterface::update));
    redraw_timer_.start(30);

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
    power_man_.SetNewViewBounds(new_horizontal_bounds);
}

void ChartInterface::SetHorizontalSuffix(const QString & suffix)
{
    //Only axis pixmap should be redrawn
    axis_man_.InitHorizontal(suffix);
}


// Установка границ по вертикали
void ChartInterface::SetVerticalMinMaxBounds(const double min_val, const double end_val, const bool is_adaptive)
{
    power_man_.EnableAdaptiveMode(is_adaptive);
    Limits<double> new_vertical_bounds = {min_val, end_val};
    auto& cur_min_max = scale_info_.val_info_.min_max_bounds_.vertical;
    // Если ничего не изменилось — выходим
    if (cur_min_max == new_vertical_bounds) return;

    cur_min_max = new_vertical_bounds;
    scale_info_.val_info_.cur_bounds.vertical = new_vertical_bounds;
    power_man_.SetPowerBounds({min_val, end_val});
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
    if (aqua_gui::ZoomFromWheelDelta(scale_info_, wheel_event->delta(), wheel_event->pos()))
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
    if(domain_type_ != ChartDomainType::kTimeFrequency) //Для ЛЧМ не используем
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
    // Получаем текущие "автоматические" границы мощности от отрисовщика DPX
    auto new_bounds = power_man_.GetPowerBounds();
    aqua_gui::UpdatePowerBounds(scale_info_, new_bounds);
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
