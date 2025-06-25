#include "DpxWindow.h"
DpxWindow::DpxWindow()
{
    ui_.setupUi(this);
    connect(ui_.do_something, &QPushButton::clicked, this, &DpxWindow::NeedDoSomething); 
};

void DpxWindow::SetChartWindow(QWidget * wigdet_ptr)
{
    ui_.DpxChart->layout()->addWidget(wigdet_ptr);
    
}
