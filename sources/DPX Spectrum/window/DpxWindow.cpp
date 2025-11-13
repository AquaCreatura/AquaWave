#include "DpxWindow.h"
DpxWindow::DpxWindow()
{
    ui_.setupUi(this);
};

void DpxWindow::SetChartWindow(QWidget * wigdet_ptr)
{
    ui_.DpxChart->layout()->addWidget(wigdet_ptr);
    
}
