#pragma once
#include <qstring.h>
#include <qgraphicsview.h>
#include <qgraphicsitem.h>
#include <qpainter.h>
#include "Tools\parse_tools.h"
#include "GUI/gui_defs.h"
namespace aqua_gui
{
/*
Class, which works with the scale
    and grid ________  
            /___/___/ - Mhz
           /___/___/ - sec
*/
class AxisManager
{
public:
    AxisManager(const ChartScaleInfo& base_scale_info);
    /*
        Init suffix of the oxis (empty, to stay same)
    */ 
    void InitHorizontal  (const QString &ox_name);
    /*
        Init suffix of the oxis (empty, to stay same)
    */ 
    void InitVertical    (const QString &oy_name);
    /*
        Main function, which redraw the grid
    */
    bool DrawAxis(QPainter& passed_painter);
   
    
private:
    bool ShouldRedraw() const;
    /*
        drawing margins with the text
    */
    bool DrawMarginBackGround(QPainter& passed_painter, const QPen& frame_pen);    
//Info about lines on axis
struct LinesInfo
{
    std::pair<int   , int   >   empty_margin_ {10,10}       ; //margins at the begining and at the end
    int                         range_between_lines_px_ {40}; //minimum range between lines
    //Info about line of the grid
    struct LineInfo
    {
        int     pixel_number_;
        double  value_;
        bool    is_text_line_{false};             
    }; 
    std::vector<LineInfo>       grid_lines_      ; //Set of our lines to draw
    double                      grid_lines_delta_; //Delta between lines
    //Process to fill grid lines vector, according data
    bool FillLineVectors(const int distance_px, const Limits<double>& val_bounds);
    //Return extension of power of 10 to show flaot value
    int  GetFloatStringPower();
};

struct GridInfo
{
    LinesInfo               grid_info;
    QString                 qstr_suffix;
};
    HV_Info<GridInfo>       axis_;
    bool                    need_be_updated_ {true}; //flag of necessaty to redraw pixmap
    QPixmap                 cache_pixmap_;
    const ChartScaleInfo&   scale_info_;
    HV_Info<Limits<double>> last_val_scaled_bounds_;
    HV_Info<int>            last_widget_size_;
};

}