#ifndef IMAGEBG_H
#define IMAGEBG_H

#include <qimage.h>
#include <qpixmap.h>
#include <qpainter.h>
#include "GUI/gui_defs.h" // Assuming this contains WH_Info and ChartScaleInfo definitions
#include "QimageZoomer.h" // Include the new class header

namespace aqua_gui
{

class ImageBG
{
public:
    /**
     * @brief Constructor initializing with scale information.
     * @param scale_info Chart scaling and pixel/value range metadata.
     */
    explicit ImageBG(const ChartScaleInfo& scale_info);

    /**
     * @brief Loads base image from specified path.
     * @param image_path Path to image file.
     * @return true if image loaded successfully, false otherwise.
     */
    bool InitImage(const QString& image_path);

    /**
     * @brief Draws processed image onto painter.
     * @param painter Reference to target painter.
     * @return true if drawing succeeded, false if base image missing.
     */
    bool DrawImage(QPainter& painter);


private:
    // Determines if redraw is necessary based on state flags.
    bool ShouldRedraw() const;

    // Calculates the value bounds for the currently displayed portion of the chart.
    // This is derived from the ChartScaleInfo's cur_bounds.
    WH_Bounds<double> CalculateTargetDisplayValueBounds() const;
    
    // Resets state flags after redraw.
    void ResetRedrawFlags();

private:
    QImage                      base_image_; // The raw base image
    QPixmap                     pixmap_to_show_; // The final pixmap to draw
    int                         base_width_  {0}; //
    int                         base_height_ {0}; //
    bool                        need_redraw_       {false}; //
    const ChartScaleInfo&       scale_info_; //

    WH_Info<int>                last_pixmap_size_;
    WH_Bounds<double>           last_base_val_bounds_;
    WH_Bounds<double>           last_scaled_val_bounds_;

    QimageZoomer                image_zoomer_; // QimageZoomer instance
};

}
#endif // IMAGEBG_H