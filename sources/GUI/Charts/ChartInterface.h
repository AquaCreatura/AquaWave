#pragma once
#include <qtimer.h>
#include <vector>
#include <qgraphicsview.h>
#include <qevent.h>

#include "GUI/Drawers/AxisPainter.h"
#include "GUI/Drawers/ImageBackGround.h"
#include "GUI/basic tools/PowerManager.h"
#include "GUI\Drawers\SelectionDrawer.h"
#include <qlayout.h>
using namespace aqua_gui;
class ChartInterface : public QWidget
{
    Q_OBJECT
public: 
    //We pass 
    ChartInterface(QWidget* parrent, std::shared_ptr<SelectionHolder> selection_holder);
    ~ChartInterface();
    /*
        Set an image from the precised path on background
    */
    bool                            SetBackgroundImage  (const QString& image_path);
    /*
        Set start and end values for verical
    
    */
    virtual void                    SetVerticalMinMaxBounds(const Limits<double>& vertical_bounds);

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
        Clear data from the spectrum
    */
    virtual void                    ClearData           () = 0;

	virtual void					SetSelectionHolder(std::shared_ptr<SelectionHolder> selection_holder);
signals:
	void					SelectionIsReady();
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

	void leaveEvent(QEvent *event) override;

	void enterEvent(QEvent *event) override;
	

protected:
    // Update power bounds for chart scaling
    virtual void UpdateChartPowerBounds();

    // Update widget size information
    void UpdateWidgetSizeInfo();

    // Redraw widget on timer timeout
    virtual void OnTimeoutRedraw();

protected:
    // Chart scaling information
    aqua_gui::ChartScaleInfo scale_info_;
    
    // Manages axis rendering
    aqua_gui::AxisManager axis_man_;
    
    
    // Background image for chart
    aqua_gui::ImageBG bg_image_;
    
    // Current mouse position
	MouseDrawer mouse_man_;
    
    // Timer for periodic redraws
    QTimer redraw_timer_;
    
    // Manages power limits for chart
    PowerLimitMan power_man_;
	
	// Manages selections ot the spectrum
	SelectionDrawer selection_drawer_;

	HV_Info<double, double> max_scale_koeff_ = { 2 ,2 };
};