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
    SetHorizontalMinMaxBounds({50'000, 200'000});
    SetHorizontalSuffix("counts");

    SetVerticalSuffix("power");
    connect(&redraw_timer_, &QTimer::timeout, this, QOverload<>::of(&ChartInterface::update));
    redraw_timer_.start(200);
    SetBackgroundImage(":/AquaWave/third_party/background/dark_city_2_cut.jpg");

}

ChartInterface::~ChartInterface()
{
}

bool ChartInterface::SetBackgroundImage(const QString & image_path)
{
    const bool res_of_init  = bg_image_.InitImage(image_path);
    return res_of_init;
}

void ChartInterface::SetVerticalBounds(const Limits<double>& vertical_bounds)
{
    auto& cur_min_max = scale_info_.val_info_.min_max_bounds_.vertical;
    // ���� ������ �� ���������� � �������
    if (cur_min_max == vertical_bounds) return;

    cur_min_max = vertical_bounds;
    scale_info_.val_info_.cur_bounds.vertical = vertical_bounds;
}

// ��������� ������ �� �����������
void ChartInterface::SetHorizontalMinMaxBounds(const Limits<double>& hor_bounds)
{
    auto& cur_val_info = scale_info_.val_info_.min_max_bounds_.horizontal;
    // ���� ������ �� ���������� � �������
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


// ��������� ������ �� ���������
void ChartInterface::SetPowerBounds(const Limits<double>& power_bounds, const bool is_adaptive)
{
    power_man_.EnableAdaptiveMode(is_adaptive);
    power_man_.SetPowerBounds(power_bounds);
    if(domain_type_ != ChartDomainType::kTimeFrequency) //� ������, ����� �������� - ��� ������������ �����
    {
        SetVerticalBounds(power_bounds);
    }


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
    UpdateWidgetSizeInfo();     //if widget size was changed
    if(domain_type_ != ChartDomainType::kTimeFrequency) //��� �������-��������� ������������� �� ����������
        UpdateChartPowerBounds(); //���������� ����� ���������� ������� ��������

    
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

void ChartInterface::UpdateChartPowerBounds()
{
    // �������� ������� "��������������" ������� �������� �� ����������� DPX
    auto new_bounds = power_man_.GetPowerBounds();
	if (domain_type_ != ChartDomainType::kTimeFrequency) //��� �������-��������� ������������� ���� ������
	{ 
		aqua_gui::AdaptPowerBounds(scale_info_, new_bounds);
	}
    
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
