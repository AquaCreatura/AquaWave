#include "QimageZoomer.h"
#include <algorithm> // For std::min and std::max
#include <QDebug> // ƒл€ отладочных сообщений, можно удалить в релизе

namespace aqua_gui
{

/**
 * @brief Sets a new base QImage. Does not take ownership.
 * @param base_qimage Pointer to the new base QImage.
 * @return true if valid image set, false otherwise.
 */
bool QimageZoomer::SetNewBase(QImage *base_qimage)
{

    if (base_image_ == base_qimage && base_qimage != nullptr && !base_qimage->isNull()) {
        return true; // No change needed
    }

    base_image_ = base_qimage;
    // Force redraw on next GetPrecisedPart call
    cached_pixmap_ = QPixmap(); 
    last_full_image_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}}; // Reset
    last_target_display_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}}; // Reset
    last_target_output_size_ = {0, 0};
    last_high_quality_ = true; 
    rendered_full_image_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}}; // Reset rendered state
    rendered_target_display_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}}; // Reset rendered state
    rendered_target_output_size_ = {0, 0};
    rendered_high_quality_ = true;
    MarkForUpdate();
    return (base_image_ != nullptr && !base_image_->isNull());
}

void QimageZoomer::MarkForUpdate()
{
    need_update_ = true;
}

/**
 * @brief Releases the current base QImage. Ownership transferred to caller.
 * @return Pointer to released QImage, or nullptr.
 */
QImage* QimageZoomer::ReleaseBase()
{
    QImage* released_image = base_image_;
    base_image_ = nullptr;
    cached_pixmap_ = QPixmap(); // Clear cached pixmap
    // Reset all state to force redraw
    last_full_image_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}}; 
    last_target_display_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}};
    last_target_output_size_ = {0, 0};
    last_high_quality_ = true;
    rendered_full_image_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}};
    rendered_target_display_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}};
    rendered_target_output_size_ = {0, 0};
    rendered_high_quality_ = true;
    return released_image;
}

/**
 * @brief Gets a precise part of the base image, cut and scaled based on value bounds.
 * Handles caching and redraw logic.
 * @param full_image_value_bounds Value bounds corresponding to the entire base image.
 * @param target_display_value_bounds Value bounds to display within the output pixmap.
 * @param target_output_size Desired final size of output pixmap in pixels.
 * @param high_quality If true, uses smooth scaling.
 * @return Reference to the cached QPixmap.
 */
QPixmap& QimageZoomer::GetPrecisedPart(const WH_Info<Limits<double>>& full_image_value_bounds, 
                                       const WH_Info<Limits<double>>& target_display_value_bounds,
                                       const WH_Info<int>& target_output_size,
                                       bool high_quality)
{
    // Update current request parameters
    last_full_image_value_bounds_ = full_image_value_bounds;
    last_target_display_value_bounds_ = target_display_value_bounds;
    last_target_output_size_ = target_output_size;
    last_high_quality_ = high_quality;

    if (NeedRedraw()) {
        UpdateQPixmap();
    }
    return cached_pixmap_;
}

/**
 * @brief Checks if a redraw of the cached QPixmap is needed.
 * @return true if redraw is necessary, false otherwise.
 */
bool QimageZoomer::NeedRedraw() const
{
    // Always redraw if no base image or invalid
    if (need_update_ || !base_image_ || base_image_->isNull()) {
        return true;
    }

    // Always redraw if cached pixmap is null or empty
    if (cached_pixmap_.isNull() || cached_pixmap_.width() == 0 || cached_pixmap_.height() == 0) {
        return true;
    }

    // Check if output size has changed from last rendered
    if (rendered_target_output_size_ != last_target_output_size_) {
        return true;
    }

    // Check if full image value bounds have changed from last rendered
    if (rendered_full_image_value_bounds_ != last_full_image_value_bounds_) {
        return true;
    }

    // Check if target display value bounds have changed from last rendered
    if (rendered_target_display_value_bounds_ != last_target_display_value_bounds_) {
        return true;
    }
    
    // Check if quality setting has changed from last rendered
    if (rendered_high_quality_ != last_high_quality_) {
        return true;
    }

    return false; // No changes requiring redraw
}

/**
 * @brief Updates the cached QPixmap by cutting and scaling the base image.
 * @return true if QPixmap updated successfully, false otherwise.
 */
bool QimageZoomer::UpdateQPixmap()
{
    if (!base_image_ || base_image_->isNull()) {
        cached_pixmap_ = QPixmap(); 
        return false;
    }
    const auto base_image_width  = base_image_->width();
    const auto base_image_height = base_image_->height();
    // Calculate pixel coordinates for cropping based on value bounds
    WH_Info<Limits<int>> basic_pixel_crop_bounds = CalculatePixelCropBounds(
        base_image_width, base_image_height,
        last_full_image_value_bounds_,
        last_target_display_value_bounds_
    );

    int src_width = basic_pixel_crop_bounds.horizontal.delta();
    int src_height = basic_pixel_crop_bounds.vertical.delta();
    int src_x = basic_pixel_crop_bounds.horizontal.low;
    int src_y = base_image_height - basic_pixel_crop_bounds.vertical.high;
    // Ensure valid crop dimensions
    if (src_width <= 0 || src_height <= 0) {
        cached_pixmap_ = QPixmap(); // Invalid crop area
        return false;
    }

    // Extract the portion of the image using QImage::copy()
    QImage cropped_image = base_image_->copy(src_x, src_y, src_width, src_height);

    // Determine scaling quality
    Qt::TransformationMode mode = last_high_quality_ ? Qt::SmoothTransformation : Qt::FastTransformation;

    // Scale to the desired size and convert to QPixmap
    cached_pixmap_ = QPixmap::fromImage(cropped_image.scaled(
        last_target_output_size_.horizontal,
        last_target_output_size_.vertical,
        Qt::IgnoreAspectRatio, 
        mode
    ));

    // Store parameters used for this successful render
    rendered_full_image_value_bounds_ = last_full_image_value_bounds_;
    rendered_target_display_value_bounds_ = last_target_display_value_bounds_;
    rendered_target_output_size_ = last_target_output_size_;
    rendered_high_quality_ = last_high_quality_;
    need_update_            = false;
    return !cached_pixmap_.isNull();
}

/**
 * @brief Converts value bounds to pixel coordinates for image cropping.
 * @param img_width Width of the base image in pixels.
 * @param img_height Height of the base image in pixels.
 * @param full_val_bounds Value bounds of the entire image.
 * @param target_val_bounds Value bounds to be displayed.
 * @return Pixel bounds (min, max) for cropping.
 */
WH_Info<Limits<int>> QimageZoomer::CalculatePixelCropBounds(
    int img_width, int img_height,
    const WH_Info<Limits<double>>& full_val_bounds,
    const WH_Info<Limits<double>>& target_val_bounds) const
{
    // Clamp values to prevent division by zero or extreme ratios if ranges are invalid.
    auto safe_span = [](double low, double high) {
        double span = high - low;
        return (span == 0.0) ? 1.0 : span; // Prevent division by zero
    };

    double full_h_span = safe_span(full_val_bounds.horizontal.low, full_val_bounds.horizontal.high);
    double full_v_span = safe_span(full_val_bounds.vertical.low, full_val_bounds.vertical.high);

    // Calculate pixel coordinates for the target display value bounds
    // Horizontal
    // The ratio of the target range within the full range.
    double h_start_ratio = (target_val_bounds.horizontal.low - full_val_bounds.horizontal.low) / full_h_span;
    double h_end_ratio = (target_val_bounds.horizontal.high - full_val_bounds.horizontal.low) / full_h_span;

    int pixel_h_low = static_cast<int>(h_start_ratio * img_width);
    int pixel_h_high = static_cast<int>(h_end_ratio * img_width);

    // Vertical
    // For QImage, Y-axis typically increases downwards. If your value system has Y increasing upwards,
    // you might need to adjust the vertical calculation:
    // (full_val_bounds.vertical.high - target_val_bounds.vertical.high) / full_v_span for start
    // (full_val_bounds.vertical.high - target_val_bounds.vertical.low) / full_v_span for end
    // Assuming Y-axis in values also increases downwards for simplicity.
    double v_start_ratio = (target_val_bounds.vertical.low - full_val_bounds.vertical.low) / full_v_span;
    double v_end_ratio = (target_val_bounds.vertical.high - full_val_bounds.vertical.low) / full_v_span;

    int pixel_v_low = static_cast<int>(v_start_ratio * img_height);
    int pixel_v_high = static_cast<int>(v_end_ratio * img_height);

    // Ensure valid pixel bounds within the image dimensions
    auto clamp_pixel = [](int val, int max_dim) {
        return std::min(std::max(0, val), max_dim);
    };

    pixel_h_low = clamp_pixel(pixel_h_low, img_width);
    pixel_h_high = clamp_pixel(pixel_h_high, img_width);
    if (pixel_h_high < pixel_h_low) std::swap(pixel_h_low, pixel_h_high); // Ensure low <= high
    if (pixel_h_high == pixel_h_low) pixel_h_high = std::min(pixel_h_low + 1, img_width); // Ensure at least 1px width, if possible

    pixel_v_low = clamp_pixel(pixel_v_low, img_height);
    pixel_v_high = clamp_pixel(pixel_v_high, img_height);
    if (pixel_v_high < pixel_v_low) std::swap(pixel_v_low, pixel_v_high); // Ensure low <= high
    if (pixel_v_high == pixel_v_low) pixel_v_high = std::min(pixel_v_low + 1, img_height); // Ensure at least 1px height, if possible


    return { {pixel_h_low, pixel_h_high}, {pixel_v_low, pixel_v_high} };
}


} // namespace aqua_gui