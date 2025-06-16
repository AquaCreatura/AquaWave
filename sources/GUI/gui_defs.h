#pragma once
#include <utility>
#include <qimage.h>
#include "aqua_defines.h"
//#include "Interfaces/ark_defs.h"

using namespace fluctus;
namespace aqua_gui
{
typedef uint32_t argb_t;
//Height / Width info
template <typename W, typename H = W>
struct WH_Info
{
    WH_Info& operator=(const WH_Info<W, H>& right) 
    {
        horizontal = right.horizontal;
        vertical   = right.vertical;
        return *this;
    }
    const bool operator==(const WH_Info<W, H>& right) const
    {
        return (horizontal == right.horizontal) && (vertical == right.vertical);
    }
    bool operator!=(const WH_Info<W, H>& right) const
    {
        return !((*this) == right);
    }
    WH_Info<W, H> operator-(const WH_Info<W, H>& right)
    {
        WH_Info<W> res;
        res.horizontal  = {horizontal - right.horizontal};
        res.vertical    = {vertical   - right.vertical};
        return res;
    }
    WH_Info<W, H> operator+(const WH_Info<W, H>& right)
    {
        WH_Info<W> res;
        res.horizontal  = {horizontal + right.horizontal};
        res.vertical    = {vertical   + right.vertical};
        return res;
    }
    W horizontal = W();
    H vertical   = H();
};
template <typename T>
using WH_Bounds = WH_Info<Limits<T>>;
struct ChartScaleInfo
{   
    struct PixelScale
    {
        //size of widget
        WH_Info<int> widget_size_px;
        //size of widget without margin
        WH_Info<int> chart_size_px ;
        //size of margin
        WH_Info<int> margin_px      {50, 30};
    };
    struct ValueScale
    {
        //if bounds are changed
        WH_Bounds<double>                    min_max_bounds_;
        WH_Bounds<double>                    cur_bounds;
    };
    PixelScale pix_info_;
    ValueScale val_info_;
};
/*
Когда qimage выступает в роли обёртки
*/
struct dynamic_qimage
{   
    QImage                  qimage;
    std::vector<argb_t>     data;
    WH_Info<size_t>         size;
};

}