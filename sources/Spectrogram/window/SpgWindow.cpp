#include "SpgWindow.h"

SpgWindow::SpgWindow()
{
    ui_.setupUi(this);
};
void SpgWindow::SetChartWindow(QWidget * wigdet_ptr)
{
    ui_.SpgChart->layout()->addWidget(wigdet_ptr);
}

