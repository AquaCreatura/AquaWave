#include "gui_conversions.h"

#include <cmath>
using namespace aqua_gui;

LUT_HSV_Instance::LUT_HSV_Core::LUT_HSV_Core()
{
    argb_t* palette = &arr[0];
    const int count_of_channels = 4;

    for(int i = 0; i < hsv_table_size_c; i ++) 
    {
        float t = static_cast<float>(i) / (hsv_table_size_c - 1);
        
        // Плавное изменение оттенка от 240° (синий) до 0° (красный)
        float hue = 240.0f * (1.0f - t);
        float saturation = 1.0f; // Максимальная насыщенность
        float value = 1.0f;       // Максимальная яркость

        uint8_t* cur_color = (uint8_t*)&palette[i]; //Начинаем работать непосредственно с цветом

        //Little endian
        auto &opacity   = cur_color[3]; //Непрозрачность
        auto &red       = cur_color[2];
        auto &green     = cur_color[1];
        auto &blue      = cur_color[0];

        opacity = (i == 0) ? 0 : 255;
        HSV_2_RGB(hue, saturation, value, 
                    red, green, blue);
        //opacity = 255;
        //palette[i + 0] = 255;
        //palette[i + 1] = 255;
        //palette[i + 2] = 255;
        //palette[i + 3] = 255;
    }
}


void LUT_HSV_Instance::LUT_HSV_Core::HSV_2_RGB(float h, float s, float v, Ipp8u & r, Ipp8u & g, Ipp8u & b)
{
    float c = v * s;
    float x = c * (1 - fabs(fmod(h/60.0, 2) - 1));
    float m = v - c;
    
    float r_, g_, b_;
    
    if(h < 60)      { r_ = c; g_ = x; b_ = 0; }
    else if(h < 120) { r_ = x; g_ = c; b_ = 0; }
    else if(h < 180) { r_ = 0; g_ = c; b_ = x; }
    else if(h < 240) { r_ = 0; g_ = x; b_ = c; }
    else if(h < 300) { r_ = x; g_ = 0; b_ = c; }
    else            { r_ = c; g_ = 0; b_ = x; }
    
    r = static_cast<Ipp8u>((r_ + m) * 255);
    g = static_cast<Ipp8u>((g_ + m) * 255);
    b = static_cast<Ipp8u>((b_ + m) * 255);
}

argb_t* LUT_HSV_Instance::get_table_ptr()
{
    static LUT_HSV_Core core_;
    return &core_.arr[0];
}
