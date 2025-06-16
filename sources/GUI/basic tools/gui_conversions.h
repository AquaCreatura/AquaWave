#pragma once
#include <ipps.h>
#include <stdint.h>
#include "GUI/gui_defs.h"
namespace aqua_gui
{
constexpr int hsv_table_size_c = 512;

//Look - Up - Table from density to the warm - cold RGB
class LUT_HSV_Instance
{
public:
    static argb_t*   get_table_ptr   ();

protected:

class LUT_HSV_Core
{
public:
    LUT_HSV_Core();
        
    // Функция конвертации HSV в RGB (H: 0-360, S/H: 0.0-1.0)
    static void     HSV_2_RGB       (float h, float s, float v, Ipp8u & r, Ipp8u & g, Ipp8u & b);
    argb_t arr[hsv_table_size_c];
};

};

}