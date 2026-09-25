#ifndef IMAGEBG_H
#define IMAGEBG_H

#include <qimage.h>
#include <qpixmap.h>
#include <qpainter.h>
#include "GUI/gui_defs.h"
#include "QimageZoomer.h"

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

    void EnableDarkMode(const bool is_dark);
    bool IsDarkMode() const { return is_dark_mode_; }
    void SetImagePaths(const QString& dark_path, const QString& light_path);

private:
    bool ShouldRedraw() const;
    HorVerLim<double> CalculateTargetDisplayValueBounds() const;
    void ResetRedrawFlags();
    bool LoadImage(const QString& image_path);

private:
    QImage                      base_image_;
    QPixmap                     pixmap_to_show_;
    int                         base_width_  {0};
    int                         base_height_ {0};
    bool                        need_redraw_       {false};
    const ChartScaleInfo&       scale_info_;
    bool                        is_dark_mode_{ true };
    QString                     dark_image_path_;
    QString                     light_image_path_;
    QString                     current_image_path_;

    HV_Info<int>                last_pixmap_size_;
    HorVerLim<double>           last_base_val_bounds_;
    HorVerLim<double>           last_scaled_val_bounds_;

    QimageZoomer                image_zoomer_;
};

}
#endif // IMAGEBG_H