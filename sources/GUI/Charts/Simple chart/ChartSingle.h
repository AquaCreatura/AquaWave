#pragma once
#include "..\ChartInterface.h"
class ChartSingle : public ChartInterface
{
public:
    ChartSingle(QWidget* parrent);
    virtual void DrawData   (QPainter& painter          ) override;
    virtual void PushData   (const draw_data& draw_data ) override;
    
};

