#include "ImageBackGround.h"
#include <QDebug> // Для отладочных сообщений, можно удалить в релизе

namespace aqua_gui
{

/**
 * @brief Constructor initializing with scale information.
 * @param scale_info Chart scaling and pixel/value range metadata.
 */
ImageBG::ImageBG(const ChartScaleInfo& scale_info)
    : scale_info_(scale_info)
{
}

/**
 * @brief Loads base image from specified path.
 * @param image_path Path to image file.
 * @return true if image loaded successfully, false otherwise.
 */
bool ImageBG::InitImage(const QString& image_path)
{
    base_image_.load(image_path); //
    if (base_image_.isNull()) return false; //


    // Set the base image in the zoomer
    return image_zoomer_.SetNewBase(&base_image_); //
}

/**
 * @brief Draws processed image onto painter.
 * @param passed_painter Reference to target painter.
 * @return true if drawing succeeded, false if base image missing.
 */
bool ImageBG::DrawImage(QPainter& passed_painter)
{
    if (base_image_.isNull()) { // Early exit if no base image
        return false;
    }

    // Only recalculate and redraw if needed
    if (ShouldRedraw()) {
        const WH_Info<Limits<double>> full_image_value_bounds = scale_info_.val_info_.min_max_bounds_;
        const WH_Info<Limits<double>> target_display_value_bounds = CalculateTargetDisplayValueBounds();
        
        // Get target size for the output pixmap
        WH_Info<int> target_image_size = {
            scale_info_.pix_info_.chart_size_px.horizontal + 1,
            scale_info_.pix_info_.chart_size_px.vertical + 1
        };

        // Use QimageZoomer to get the precise part, scaled and potentially cached
        pixmap_to_show_ = image_zoomer_.GetPrecisedPart(
            full_image_value_bounds, 
            target_display_value_bounds, 
            target_image_size, 
            need_good_quality_
        );
        ResetRedrawFlags(); // Reset flags after successful redraw
    }
    
    // Draw the cached pixmap
    passed_painter.drawPixmap(0, 0, pixmap_to_show_);

    return true;
}

/**
 * @brief Signals need for high quality redraw.
 */
void ImageBG::SetGoodQuality()
{
    need_redraw_ = true; //
    need_good_quality_ = true; //
}

// ======================== PRIVATE METHODS ======================== //

/**
 * @brief Determines if redraw is necessary based on state flags.
 */
bool ImageBG::ShouldRedraw() const
{
    return need_redraw_ || last_pixmap_size_!=scale_info_.pix_info_.chart_size_px || 
            last_scaled_val_bounds_ != scale_info_.val_info_.cur_bounds || last_base_val_bounds_ != scale_info_.val_info_.min_max_bounds_; //
}

template <typename T>
constexpr const T& clamp(const T& val, const T& low, const T& high) {
    return std::max(low, std::min(val, high));
}
/**
 * @brief Calculates the value bounds for the currently displayed portion of the chart.
 *
 * Algorithm:
 * 1. Determine the center of the current view from scale_info_.val_info_.
 * 2. Get two zoom coefficients (horizontal and vertical) from scale_info_.val_info_.
 * Calculate the average of these two as the "working zoom coefficient".
 * 3. Based on the working zoom coefficient and the full image value bounds (min_max_bounds_),
 * determine the new length and width of the target display rectangle.
 * This ensures that the proportions of the original image are maintained.
 * 4. Define the new rectangle based on the calculated center, width, and height.
 *
 * @return WH_Bounds<double> representing the target display value bounds.
 */
WH_Bounds<double> ImageBG::CalculateTargetDisplayValueBounds() const
{
    // 1. Определяем центр текущего отображения
    const auto& current_h_bounds = scale_info_.val_info_.cur_bounds.horizontal;
    const auto& current_v_bounds = scale_info_.val_info_.cur_bounds.vertical;
    double center_x = current_h_bounds.mid(); // Предполагаем наличие метода mid()
    double center_y = current_v_bounds.mid(); // Предполагаем наличие метода mid()

    // Полные размеры изображения, предотвращая деление на ноль
    const auto& full_h_bounds = scale_info_.val_info_.min_max_bounds_.horizontal;
    const auto& full_v_bounds = scale_info_.val_info_.min_max_bounds_.vertical;
    double full_image_span_x = std::max(full_h_bounds.delta(), 1e-9);
    double full_image_span_y = std::max(full_v_bounds.delta(), 1e-9);

    // 2. Текущие размеры отображения.
    // Предотвращаем деление на ноль, если текущий диапазон равен нулю.
    double current_span_x = current_h_bounds.delta();
    if (std::abs(current_span_x) < 1e-9) {
        current_span_x = full_image_span_x;
    }
    double current_span_y = current_v_bounds.delta();
    if (std::abs(current_span_y) < 1e-9) {
        current_span_y = full_image_span_y;
    }

    // Рабочий коэффициент зума с учетом весов
    double zoom_koeff_h = full_image_span_x / current_span_x - 1;
    double zoom_koeff_v = full_image_span_y / current_span_y - 1;
    double working_zoom_koeff = std::max(1 + (zoom_koeff_h * 0.07 + zoom_koeff_v * 0.03) / 2.0, 1.0);

    // 3. Новые размеры
    double new_span_x = full_image_span_x / working_zoom_koeff;
    double new_span_y = full_image_span_y / working_zoom_koeff;

    // 4. и 5. Корректировка центра, чтобы прямоугольник оставался в full_image_span
    // Если новый диапазон больше полного, центрируем его относительно полного.
    if (new_span_x > full_image_span_x) {
        center_x = full_h_bounds.mid();
    } else {
        // Вычисляем min/max для центра, чтобы границы не выходили за full_h_bounds
        double min_center_x = full_h_bounds.low + new_span_x / 2.0;
        double max_center_x = full_h_bounds.high - new_span_x / 2.0;
        center_x = clamp(center_x, min_center_x, max_center_x); // Используем нашу clamp
    }

    if (new_span_y > full_image_span_y) {
        center_y = full_v_bounds.mid();
    } else {
        double min_center_y = full_v_bounds.low + new_span_y / 2.0;
        double max_center_y = full_v_bounds.high - new_span_y / 2.0;
        center_y = clamp(center_y, min_center_y, max_center_y); // Используем нашу clamp
    }

    // Пересчитываем конечные границы с скорректированным центром
    Limits<double> target_h_bounds = {center_x - new_span_x / 2.0, center_x + new_span_x / 2.0};
    Limits<double> target_v_bounds = {center_y - new_span_y / 2.0, center_y + new_span_y / 2.0};

    return {target_h_bounds, target_v_bounds};
}

/**
 * @brief Resets state flags after redraw.
 */
void ImageBG::ResetRedrawFlags()
{
    need_redraw_            = false; //
    need_good_quality_      = false; //
    last_pixmap_size_       = scale_info_.pix_info_.chart_size_px;
    last_scaled_val_bounds_ = scale_info_.val_info_.cur_bounds;
    last_base_val_bounds_   = scale_info_.val_info_.min_max_bounds_;
}

} // namespace aqua_gui