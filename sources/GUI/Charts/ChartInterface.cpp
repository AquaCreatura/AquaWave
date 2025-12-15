#include "ChartInterface.h"
#include <qsizepolicy.h>
#include "GUI/basic tools/gui_helper.h"
ChartInterface::ChartInterface(QWidget* parent, std::shared_ptr<SelectionHolder> selection_holder) :
    QWidget(parent), axis_man_(scale_info_), bg_image_(scale_info_), selection_drawer_(scale_info_)
{ 
	selection_drawer_.SetSelectionHolder(selection_holder);
    // Our widget params
    this->setMouseTracking(true);
    this->setMinimumSize(200, 100);
    // Set min max values for our chart
    SetHorizontalMinMaxBounds({50'000, 200'000});
    SetHorizontalSuffix("counts");

    SetVerticalSuffix("power");
    connect(&redraw_timer_, &QTimer::timeout, this, QOverload<>::of(&ChartInterface::update));
    redraw_timer_.start(200);
    SetBackgroundImage(":/AquaWave/third_party/background/black_mountain.jpg");

}

ChartInterface::~ChartInterface()
{
}

bool ChartInterface::SetBackgroundImage(const QString & image_path)
{
    const bool res_of_init  = bg_image_.InitImage(image_path);
    return res_of_init;
}

void ChartInterface::SetVerticalMinMaxBounds(const Limits<double>& vertical_bounds)
{
    auto& cur_min_max = scale_info_.val_info_.min_max_bounds_.vertical;
    // Если ничего не изменилось — выходим
    if (cur_min_max == vertical_bounds) return;

    cur_min_max = vertical_bounds;
    scale_info_.val_info_.cur_bounds.vertical = vertical_bounds;
}

// Установка границ по горизонтали
void ChartInterface::SetHorizontalMinMaxBounds(const Limits<double>& hor_bounds)
{
    auto& cur_val_info = scale_info_.val_info_.min_max_bounds_.horizontal;
    // Если ничего не изменилось — выходим
    if (cur_val_info == hor_bounds) return;

    cur_val_info = hor_bounds;
    scale_info_.val_info_.cur_bounds.horizontal = hor_bounds;
    power_man_.SetNewViewBounds(hor_bounds);
}

void ChartInterface::SetHorizontalSuffix(const QString & suffix)
{
    //Only axis pixmap should be redrawn
    axis_man_.InitHorizontal(suffix);
}


// Установка границ по вертикали
void ChartInterface::SetPowerBounds(const Limits<double>& power_bounds, const bool is_adaptive)
{
    power_man_.EnableAdaptiveMode(is_adaptive);
    power_man_.SetPowerBounds(power_bounds);
    if(domain_type_ != ChartDomainType::kTimeFrequency) //В случае, когда мощность - это вертикальная шкала
    {
        SetVerticalMinMaxBounds(power_bounds);
    }


}

void ChartInterface::SetVerticalSuffix(const QString & suffix)
{
    //Only axis pixmap should be redrawn
    axis_man_.InitVertical(suffix);
}



void ChartInterface::mousePressEvent(QMouseEvent * mouse_event)
{
	selection_drawer_.EditableEvent(mouse_event->pos(), SelectionDrawer::kPressed);
}


void ChartInterface::SetSelectionHolder(std::shared_ptr<SelectionHolder> selection_holder)
{
}

void ChartInterface::mouseMoveEvent(QMouseEvent * mouse_event)
{
	selection_drawer_.EditableEvent(mouse_event->pos(), SelectionDrawer::kMove);
	update();
}

void ChartInterface::mouseReleaseEvent(QMouseEvent * mouse_event)
{
	selection_drawer_.EditableEvent(mouse_event->pos(), SelectionDrawer::kReleased);
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
    UpdateWidgetSizeInfo();     //if widget size was changed
    if(domain_type_ != ChartDomainType::kTimeFrequency) //Для линейно-частотной интерпретации не используем
        UpdateChartPowerBounds(); //Необходимо знать акутальные границы мощности

    
    QPainter new_frame_painter(this);
    //Background Image
    {
        //Draw our background
        bg_image_.DrawImage(new_frame_painter);
    }
    //Axis + Grid
    {
        axis_man_.DrawAxis(new_frame_painter);
    }
    //Data
    {
        DrawData(new_frame_painter);
    }
    //Selection
    {
		selection_drawer_.DrawSelections(new_frame_painter);
    }
    //Mouse Pos
    {
    
    }
}

void ChartInterface::UpdateChartPowerBounds()
{
    // Получаем текущие "автоматические" границы мощности от отрисовщика DPX
    auto new_bounds = power_man_.GetPowerBounds();
	if (domain_type_ != ChartDomainType::kTimeFrequency) //Для линейно-частотной интерпретации своя логика
	{ 
		aqua_gui::AdaptPowerBounds(scale_info_, new_bounds);
	}
    
}

void ChartInterface::UpdateWidgetSizeInfo()
{
    aqua_gui::HV_Info<int> cur_size = {this->width(), this->height()};
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
