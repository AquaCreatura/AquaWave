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
    enum ChartDomainType
    {
        kFreqDomain,    // Default АЧХ
        kTimeFrequency, // Частотно временная область
        kTimeDomain,    // Амплитудно временная область
        kCountsDomain,  // Ось X - это отсчёты
    };
    //We pass 
    ChartInterface(QWidget* parrent);
    ~ChartInterface();
    /*
        Set an image from the precised path on background
    */
    bool                            SetBackgroundImage  (const QString& image_path);
    /*
        Set start and end values for verical
    
    */
    virtual void                    SetVerticalBounds(const Limits<double>& vertical_bounds);

    /*
        Set start and end values for horizontal axis 
    */
    virtual void                    SetHorizontalMinMaxBounds (const Limits<double>& hor_bounds);
    /*
        Set horizontal axis suffix
    */
    virtual void                    SetHorizontalSuffix (const QString& suffix);
    /*
        if "is_adaptive" factor is switched =>
        We adapt Limits for passed values
    */ 
    virtual void                    SetPowerBounds   (const Limits<double>& power_bounds, const bool is_adaptive = true);
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
    /*
        Clear data from the spectrum
    */
    virtual void                    ClearData           () = 0;
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

    ChartDomainType domain_type_;
};