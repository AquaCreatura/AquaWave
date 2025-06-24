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
    virtual void                    PushData            (const draw_data& draw_data ) = 0 ;
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
    // Handle mouse wheel events for zooming or scrolling
    virtual void wheelEvent(QWheelEvent* wheel_event) override;

    /*
        Handle widget resize events
    */
    virtual void resizeEvent(QResizeEvent *event) override;

    // Handle widget repaint requests
    virtual void paintEvent(QPaintEvent* paint_event) override;

protected:
    // Update power bounds for chart scaling
    virtual void UpdatePowerBounds();

    // Update widget size information
    void UpdateWidgetSizeInfo();

    // Redraw widget on timer timeout
    virtual void OnTimeoutRedraw();

protected:
    // Chart scaling information
    aqua_gui::ChartScaleInfo scale_info_;
    
    // Manages axis rendering
    aqua_gui::AxisManager axis_man_;
    
    // Manages chart selection
    std::shared_ptr<ChartSelection> chart_selection_;
    
    // Background image for chart
    aqua_gui::ImageBG bg_image_;
    
    // Current mouse position
    QPoint mouse_pos_;
    
    // Timer for periodic redraws
    QTimer redraw_timer_;
    
    // Manages power limits for chart
    PowerLimitMan power_man_;
};