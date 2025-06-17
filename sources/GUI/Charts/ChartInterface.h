#pragma once
#include <qtimer.h>
#include <vector>
#include <qgraphicsview.h>
#include <qevent.h>

#include "GUI/basic tools/ChartSelection.h"
#include "GUI/Drawers/AxisPainter.h"
#include "GUI/Drawers/ImageBackGround.h"
#include "GUI/basic tools/PowerManager.h"
#include <qlayout.h>
using namespace aqua_gui;
class ChartInterface : public QWidget
{
    Q_OBJECT
public: 
    //We pass 
    ChartInterface(QWidget* parrent);
    ~ChartInterface();
    /*
        Set an image from the precised path on background
    */
    bool                            SetBackgroundImage  (const QString& image_path);

    /*
        Set start and end values for horizontal axis 
    */ 
    virtual void                    SetHorizontalMinMaxBounds (const double min_val  , const double end_val);
    /*
        Set horizontal axis suffix
    */
    virtual void                    SetHorizontalSuffix (const QString& suffix);
    /*
        Set start and end values for verica; axis and suffix of the oxis, if "is_adaptive" factor is switched =>
        We adapt Limits for passed values
    */ 
    virtual void                    SetVerticalMinMaxBounds   (const double min_val, const double end_val, const bool is_adaptive = true);
    /*
        Set horizontal axis suffix
    */
    virtual void                    SetVerticalSuffix   (const QString& suffix);
    /*
        Pushing new portion of data
    */
    virtual void                    PushData            (std::vector<float>& data, const Limits<double>& data_bounds) = 0 ;
    /*
        Draw values
    */
    virtual void                    DrawData            (QPainter& painter) = 0;
    /*
        Ask for selection 
    */
    std::shared_ptr<ChartSelection> GetSelection();
protected slots:
    /*
        Reaction on mouse clicking. 
    */
    virtual void mousePressEvent    (QMouseEvent* mouse_event) override;

    /*
        Reaction on mouse moving. 
    */
    virtual void mouseMoveEvent     (QMouseEvent* mouse_event) override;
    /*
        Reaction on mouse release. 
    */
    virtual void mouseReleaseEvent  (QMouseEvent* mouse_event) override;
    /*
        Reaction on scroll event
    */
    virtual void wheelEvent         (QWheelEvent* wheel_event) override;
    /*
        Default update event
    */
    virtual void resizeEvent        (QResizeEvent *event)              override;      
    virtual void paintEvent         (QPaintEvent* paint_event) override;     
protected:
    virtual void    UpdatePowerBounds   ();
    void    UpdateWidgetSizeInfo();
    virtual void OnTimeoutRedraw     ();
protected:  
    //info, which contains info about scaling
    aqua_gui::ChartScaleInfo                  scale_info_;
    //our managers which redraw images
    aqua_gui::AxisManager           axis_man_;
    std::shared_ptr<ChartSelection> chart_selection_;
    aqua_gui::ImageBG               bg_image_;
    QPoint                          mouse_pos_;
    QTimer                          redraw_timer_;
    PowerLimitMan                   power_man_;
};
