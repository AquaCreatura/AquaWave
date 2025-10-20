#pragma once
#include "GUI/gui_defs.h" // Assuming this contains WH_Info and Limits definitions
#include <qimage.h>
#include <qpixmap.h>
#include <qpainter.h> 
#include <algorithm> // ��� std::min, std::max
#include <qelapsedtimer.h>
namespace aqua_gui
{

class QimageZoomer
{
public:
    /**
     * @brief Sets a new base QImage. Does not take ownership.
     * @param base_qimage Pointer to the new base QImage.
     * @return true if valid image set, false otherwise.
     */
    bool SetNewBase(QImage *base_qimage);
    void MarkForUpdate();
    /**
     * @brief Releases the current base QImage. Ownership transferred to caller.
     * @return Pointer to released QImage, or nullptr.
     */
    QImage* ReleaseBase();

    /**
     * @brief Gets a precise part of the base image, cut and scaled based on value bounds.
     * Handles caching and redraw logic.
     * @param full_image_value_bounds Value bounds corresponding to the entire base image.
     * @param target_display_value_bounds Value bounds to display within the output pixmap.
     * @param target_output_size Desired final size of output pixmap in pixels.
     * @param high_quality If true, uses smooth scaling.
     * @return Reference to the cached QPixmap.
     */
    QPixmap& GetPrecisedPart(const WH_Info<Limits<double>>& full_image_value_bounds, 
                             const WH_Info<Limits<double>>& target_display_value_bounds,
                             const WH_Info<int>& target_output_size);

protected:
    /**
     * @brief Checks if a redraw of the cached QPixmap is needed.
     * @return true if redraw is necessary, false otherwise.
     */
    bool NeedRedraw() const;

    /**
     * @brief Updates the cached QPixmap by cutting and scaling the base image.
     * @return true if QPixmap updated successfully, false otherwise.
     */
    bool UpdateQPixmap();

private:
    /**
     * @brief Converts value bounds to pixel coordinates for image cropping.
     * @param img_width Width of the base image in pixels.
     * @param img_height Height of the base image in pixels.
     * @param full_val_bounds Value bounds of the entire image.
     * @param target_val_bounds Value bounds to be displayed.
     * @return Pixel bounds (min, max) for cropping.
     */
    WH_Info<Limits<int>> CalculatePixelCropBounds(
        int img_width, int img_height,
        const WH_Info<Limits<double>>& full_val_bounds,
        const WH_Info<Limits<double>>& target_val_bounds) const;
    QImage *base_image_ = nullptr;      // Pointer to the base image (not owned)
    QPixmap cached_pixmap_;             // The cached result QPixmap
    
    // Parameters from the last GetPrecisedPart call, for redraw checks
    WH_Info<Limits<double>> last_full_image_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}};
    WH_Info<Limits<double>> last_target_display_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}};
    WH_Info<int> last_target_output_size_ = {0, 0};

    // Parameters of the last successfully rendered pixmap, for robust NeedRedraw()
    WH_Info<Limits<double>> rendered_full_image_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}};
    WH_Info<Limits<double>> rendered_target_display_value_bounds_ = {{0.0, 0.0}, {0.0, 0.0}};
    WH_Info<int> rendered_target_output_size_ = {0, 0};
    bool is_rendered_high_quality_	= false;
    bool need_update_           = false;
	mutable bool need_high_quality_ = false;
	mutable QElapsedTimer good_quality_timer_; //
};

} // namespace aqua_gui�