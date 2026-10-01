#pragma once
#include <vector>
#include <algorithm>
#include "GUI/gui_defs.h"
#include <QElapsedTimer>
#include <deque>

namespace aqua_gui
{

// Adjusts chart scale based on mouse wheel delta and scale point
bool ZoomFromWheelDelta(ChartScaleInfo& scale_info, const int wheel_delta, const QPoint scale_point);
void AdaptVertPowerBounds (ChartScaleInfo& scale_info);
bool PanFromMouse(ChartScaleInfo& scale_info, const QPoint start_mouse_point, 
									const HV_Info<double, double>& /*start_hor_ver*/, const QPoint end_mouse_point);

// Linear interpolator between two values
class SlopeInterpolator
{
public:
    // Initialize with first and next values, compute slope
    SlopeInterpolator(const double first_val, const double next_val)
        : first_val_(first_val), slope_koeff_(next_val - first_val) {}

    // Interpolate value at ratio [0, 1) between first and second value
    constexpr double Interpolate(double ratio) const noexcept
    {
        return first_val_ + (ratio * slope_koeff_);
    }

private:
    const double first_val_ = 0;    // First value
    const double slope_koeff_ = 0;  // Slope between first and next value
};

// Returns normalized position [0, 1] of element's center for given iteration
const double GetAsimDrawPlace(const int iteration_counter);


// Returns vector mapping iteration indices to unique pixel positions [0, width-1]
std::vector<int> GetAssimLocationsVec(const int width);




class FpsEstimator {
public:
    /**
     * @param smoothingWindowSec ќкно усреднени€ в секундах (например, 2.0)
     */
    explicit FpsEstimator(double smoothingWindowSec = 4.0);

    /**
     * ¬ызывать каждый раз, когда отрисован новый кадр.
     */
    void MarkNewFrame();

    /**
     * ¬озвращает текущий FPS, рассчитанный как среднее за окно.
     * ≈сли данных недостаточно, возвращает 0.
     */
    double GetCurrentFps() const;

    /**
     * ¬озвращает среднее врем€ кадра (в мс) за окно.
     */
    double GetAvgFrameTimeMs() const;

private:
    double m_smoothingWindowSec;

    // ’раним временные метки (в мс) начала каждого кадра относительно старта таймера
    std::deque<qint64> m_frameTimestamps;

    QElapsedTimer m_timer;
    bool m_isRunning;
};

} // namespace aqua_gui