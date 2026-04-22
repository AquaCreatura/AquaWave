#pragma once
#include <utility>
#include <qimage.h>
#include "aqua_defines.h"
//#include "Interfaces/ark_defs.h"

using namespace fluctus;
namespace aqua_gui
{
typedef uint32_t argb_t;
//Height / Width descr
template <typename W, typename H = W>
struct HV_Info
{
    HV_Info& operator=(const HV_Info<W, H>& right) 
    {
        hor = right.hor;
        vert   = right.vert;
        return *this;
    }
    const bool operator==(const HV_Info<W, H>& right) const
    {
        return (hor == right.hor) && (vert == right.vert);
    }
    bool operator!=(const HV_Info<W, H>& right) const
    {
        return !((*this) == right);
    }
    HV_Info<W, H> operator-(const HV_Info<W, H>& right)
    {
        HV_Info<W> res;
        res.hor  = {hor - right.hor};
        res.vert    = {vert   - right.vert};
        return res;
    }
    HV_Info<W, H> operator+(const HV_Info<W, H>& right)
    {
        HV_Info<W> res;
        res.hor  = {hor + right.hor};
        res.vert    = {vert   + right.vert};
        return res;
    }
    W hor = W();
    H vert   = H();
};
template <typename T>
using HorVerLim = HV_Info<Limits<T>>;

enum class ChartDomainType
{
	kFreqDomain,    // Default АЧХ
	kTimeFrequency, // Частотно временная область
	kTimeDomain,    // Амплитудно временная область
	kAnalyzeDomain, // График для анализа
};


struct ChartScaleInfo
{   
	ChartScaleInfo(ChartDomainType passed_domain) { val_info_.domain_type = passed_domain; }
	ChartScaleInfo() = default;
    struct PixelScale
    {
        //size of widget
        HV_Info<int> widget_size_px;
        //size of widget without margin
        HV_Info<int> chart_size_px ;
        //size of margin
		HV_Info<int> margin_px{ 0, 0 };
    };
    struct ValueScale
    {
        //if bounds are changed
        HorVerLim<double>                    min_max_bounds_;
        HorVerLim<double>                    cur_bounds;
		bool								 need_reset_scale_{false};
		HV_Info<double>						 max_zoom_koeffs_{20., 20.};
		ChartDomainType						 domain_type;
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
	HV_Info<size_t>         size;
};

struct draw_data
{
    std::vector<float>  data;           //Данные, которые планируем отрисовать 
    Limits<double>      freq_bounds;    //Границы в частотной области наших данных
    double              time_pos;       //Временная метка наших данных
};

struct selection_info {
	Limits<double> freq_bounds;
	Limits<double> time_bounds;
	Limits<double> power_bounds;

	bool is_finished = true;
};


}