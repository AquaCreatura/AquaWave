#include "gui_conversions.h"

#include <cmath>
using namespace aqua_gui;


LUT_HSV_Instance::LUT_HSV_Core::LUT_HSV_Core()
{
    argb_t* palette = &arr[0];
    const int count_of_channels = 4;

	struct ColorNode {
		float t;
		uint8_t r, g, b;
	};

	const int num_nodes = 11;
	const ColorNode nodes[num_nodes] = {
		{ 0.0f,   0,   0,   4 }, // Уровень шума (глубокий чёрно-синий)
		{ 0.1f,  22,  11,  57 }, // Тёмно-фиолетовый
		{ 0.2f,  66,  10, 104 }, // Фиолетовый
		{ 0.3f, 106,  23, 110 }, // Пурпурный
		{ 0.4f, 147,  38, 103 }, // Малиновый
		{ 0.5f, 188,  55,  84 }, // Тёмно-красный
		{ 0.6f, 225,  81,  64 }, // Красно-оранжевый
		{ 0.7f, 248, 122,  42 }, // Оранжевый
		{ 0.8f, 252, 174,  18 }, // Жёлто-оранжевый
		{ 0.9f, 240, 229,  27 }, // Жёлтый
		{ 1.0f, 255, 255, 255 }  // Перегруз / Пик (чистый белый)
	};

	for (int i = 0; i < hsv_table_size_c; i++)
	{
		// Базовая нормировка от 0.0 до 1.0
		float t = static_cast<float>(i) / (hsv_table_size_c - 1);
		t = pow(t,0.85);

		// Защита от выхода за границы при математических погрешностях
		if (t <= 0.0f) t = 0.0f;
		if (t >= 1.0f) t = 1.0f;

		// Поиск интервала между опорными точками
		int idx = 0;
		for (int j = 0; j < num_nodes - 1; ++j) {
			if (t >= nodes[j].t && t <= nodes[j + 1].t) {
				idx = j;
				break;
			}
		}

		// Линейная интерполяция внутри найденного интервала
		float t_low = nodes[idx].t;
		float t_high = nodes[idx + 1].t;
		float factor = (t - t_low) / (t_high - t_low);

		uint8_t r = static_cast<uint8_t>(nodes[idx].r + factor * (nodes[idx + 1].r - nodes[idx].r));
		uint8_t g = static_cast<uint8_t>(nodes[idx].g + factor * (nodes[idx + 1].g - nodes[idx].g));
		uint8_t b = static_cast<uint8_t>(nodes[idx].b + factor * (nodes[idx + 1].b - nodes[idx].b));

		// Прямая запись в память цвета
		uint8_t* cur_color = (uint8_t*)&palette[i];

		// Формат Little Endian (BGRA в памяти)
		cur_color[0] = b; // Синий
		cur_color[1] = g; // Зелёный
		cur_color[2] = r; // Красный
		cur_color[3] = (i == 0) ? 0 : 255; // Непрозрачность (полностью прозрачный для абсолютного нуля)
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

argb_t aqua_gui::LUT_HSV_Instance::DensityToRGB(double density)
{

	if (density <= 0)
		return 0; //Полностью прозрачный
	argb_t* color_palette = LUT_HSV_Instance::get_table_ptr();
	// Calculate palette index and clamp within valid range
	int color_index = static_cast<int>(density * (hsv_table_size_c - 1));
	color_index = qBound(0, color_index + 1, hsv_table_size_c - 1);
	// Return RGBA color from palette 
	return color_palette[color_index];
}

int aqua_gui::LUT_HSV_Instance::RgbToDensityFast(const argb_t rgb_color)
{
	uint8_t* rgb_array = (uint8_t*)&rgb_color;
	const uint8_t max_val = std::max({ rgb_array[0], std::max(rgb_array[2], rgb_array[1]) });
	return max_val;
}
