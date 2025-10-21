#include "gui_helper.h"
#include <math.h>


bool aqua_gui::ZoomFromWheelDelta(ChartScaleInfo & scale_info, const int wheel_delta, const QPoint scale_point)
{
    auto& min_max_bounds         = scale_info.val_info_.min_max_bounds_;
    auto& cur_bounds      = scale_info.val_info_.cur_bounds;
    auto& px_info = scale_info.pix_info_;

    bool is_y_scale = scale_point.x() > px_info.chart_size_px.horizontal;
    bool is_x_scale = !is_y_scale;
    
    // ������ ����������� ��������������� ��� ����� �������� ���������
    const double scale_koeff = std::abs(wheel_delta) / 5000.0; // �������� ����������� ��� ����� �������� ���������������
    const double direction = wheel_delta > 0 ? 1.0 : -1.0;

    // ���������� ������������� ���������������
    const double val_x_scale_koeff = (min_max_bounds.horizontal.delta()) / (cur_bounds.horizontal.delta());

    const double val_y_scale_koeff = (min_max_bounds.vertical.delta()) / (cur_bounds.vertical.delta());
    bool values_changed = false;
	const auto zoom_threshold = scale_info.val_info_.max_zoom_koeffs_;
    // ��������������� �� ��� X
    if (is_x_scale)
    {
        auto &cur_hor       = cur_bounds.horizontal;
        auto &min_max_hor   = min_max_bounds.horizontal;
        // ���������, ����� �� �� �����������/��������� �������
        bool can_zoom_in = direction > 0 && val_x_scale_koeff < zoom_threshold.horizontal;
        bool can_zoom_out = direction < 0;
        
        if (can_zoom_in || can_zoom_out)
        {
            int x_mouse_pos_px = scale_point.x();
            double x_val_to_px_koeff = cur_hor.delta() / px_info.chart_size_px.horizontal;
            double x_mouse_pos_val = cur_hor.low + x_mouse_pos_px * x_val_to_px_koeff;

            // ��������� ����� ������� � ������ ����������� ���������������
            double left_change = (x_mouse_pos_val - cur_hor.low) * scale_koeff * direction;
            double right_change = (cur_hor.high - x_mouse_pos_val) * scale_koeff * direction;

            // ��������� ���������
            double new_low = (cur_hor.low + left_change);
            double new_high =(cur_hor.high - right_change);

            // ���������, ����� ����� ������� ���� ���������
            if (new_low < new_high)
            {
                // ������������ ����������� � ������������ �������
                new_low = std::max(new_low, min_max_hor.low);
                new_high = std::min(new_high, min_max_hor.high);

                // ��������� ����������� ������ (�� ������ 1 �������)
                if (new_high - new_low >= 0.01)
                {
                    cur_hor = {new_low, new_high};
                    values_changed = true;
                }
            }
        }
    }

    // ��������������� �� ��� Y (���������� ��� X)
    if (is_y_scale)
    {
        bool can_zoom_in = direction > 0 && val_y_scale_koeff < zoom_threshold.vertical;
        bool can_zoom_out = direction < 0;
        auto &cur_vert      = cur_bounds.vertical;
        auto &min_max_vert  = min_max_bounds.vertical;
        if (can_zoom_in || can_zoom_out)
        {
            int y_mouse_pos_px = scale_point.y();
            double y_val_to_px_koeff = (cur_vert.delta()) / px_info.chart_size_px.vertical;
            double y_mouse_pos_val = cur_vert.high - y_mouse_pos_px * y_val_to_px_koeff;

            double bottom_change = (y_mouse_pos_val - cur_vert.low) * scale_koeff * direction;
            double top_change = (cur_vert.high - y_mouse_pos_val) * scale_koeff * direction;

            double new_low  = cur_vert.low + bottom_change;
            double new_high = cur_vert.high - top_change;

            if (new_low < new_high)
            {
                new_low = std::max(new_low, min_max_vert.low);
                new_high = std::min(new_high, min_max_vert.high);

                if (new_high - new_low >= std::numeric_limits<double>::epsilon())
                {
                    cur_vert = {new_low, new_high};
                    values_changed = true;
                }
            }
        }
    }
    return values_changed;
}

void aqua_gui::AdaptPowerBounds(ChartScaleInfo & scale_info, const Limits<double>& new_bounds)
{
    // �������� ������ �� ������� ����������� ���������� (��������������) ������� �����
    auto &vert_min_max = scale_info.val_info_.min_max_bounds_.vertical;

	const double min_epsilon = new_bounds.delta() * 0.05;
    // ���� �������������� ������� ����������, ��������� �����
    if(std::abs(vert_min_max.low - new_bounds.low) > min_epsilon || 
		std::abs(vert_min_max.high - new_bounds.high) > min_epsilon )
    {

        // �������� ������ �� ������� ������������ ������� ����� (������� ����� ������������)
        auto &vert_cur = scale_info.val_info_.cur_bounds.vertical;
		//����������� ������������
		if (!scale_info.val_info_.need_reset_scale_) {

			// ��������� ������� ����������� ��������������� (����) �� ���������
			const double zoom_vert_koeff = vert_cur.delta() / vert_min_max.delta();
			// ������������ ����� ������ (��������) ��� ������������ �����, �������� ���
			const double new_height = (new_bounds.high - new_bounds.low) * zoom_vert_koeff;
			// ��������� ������� ����� ������������ �����
			double zoom_centre = (vert_cur.high + vert_cur.low) / 2;
			// ������������ ����� ����, ����� ������������ �������� �� ����� �� ����� �������������� �������
			zoom_centre = qBound(new_bounds.low + new_height / 2, zoom_centre, new_bounds.high - new_height / 2);

			// ��������� ����������� ���������� (��������������) �������
			vert_min_max = new_bounds;
			// ��������� ������� ������������ ������� � ������ ������ ������ � ������
			vert_cur = { zoom_centre - new_height / 2, zoom_centre + new_height / 2 };
		}
		else {
			// ��������� ����������� ���������� (��������������) �������
			vert_min_max = new_bounds;
			vert_cur = new_bounds;
			scale_info.val_info_.need_reset_scale_ = false;
		}        
    }
}

