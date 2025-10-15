#include "PowerManager.h"
#include <ipp.h>
#include <algorithm>

using namespace aqua_gui;

PowerLimitMan::PowerLimitMan()
{
	need_reset_bounds_ = true;
    is_adaptive_mode_ = true;              // ���������� ����� ������� �� ���������
	power_bounds_ = { 0., 1. };
    power_margins_ = {-0.15, 0.15};         // ����� (margin) �� ���������� �������� ��������
}

void PowerLimitMan::SetNewViewBounds(const Limits<double>& x_bounds) {
    view_bounds_ = x_bounds;               // ��������� ����� ������ ������� �������
}

void PowerLimitMan::SetPowerBounds(const Limits<double>& power_bounds) {
	need_reset_bounds_ = false;
    power_bounds_ = power_bounds;          // ��������� ������ �������� �������
}

void aqua_gui::PowerLimitMan::SetPowerMargins(const Limits<double>& power_margins)
{
    power_margins_ = power_margins;        // ��������� ����� ��� ���������� ������ ��������
}

Limits<double> PowerLimitMan::GetPowerBounds() const {
    return power_bounds_.load();           // ��������� ������� ������ ��������
}

void PowerLimitMan::UpdateBounds(const std::vector<float>& data, const Limits<double>& data_bounds) {
    if (!IsAdaptiveMode() || data.empty()) {
        return;                            // �����, ���� ����� �� ���������� ��� ������ �����
    }

    // ����������� ������� ������ � ������ ������
    Limits<double> view = view_bounds_.load();
    double intersect_low = std::max(data_bounds.low, view.low);
    double intersect_high = std::min(data_bounds.high, view.high);
    
    if (intersect_low >= intersect_high) {
        return;                            // ��� �����������
    }

    // ���������� �������� ������� ������, ��������������� �������� �����������
    double data_range = data_bounds.delta();
    if (data_range <= 0.0) {
        return;                            // ������������ �������� ������
    }

    size_t data_size = data.size();
    size_t start_idx = static_cast<size_t>((intersect_low - data_bounds.low) * (data_size - 1) / data_range);
    size_t end_idx = static_cast<size_t>((intersect_high - data_bounds.low) * (data_size - 1) / data_range);
    
    if (start_idx >= end_idx || end_idx >= data_size) {
        return;                            // ������������ �������
    }

    // ���������� 5% ������� �� ���� ���������
    size_t margin = static_cast<size_t>((end_idx - start_idx) * 0.05);
    start_idx = std::min(start_idx + margin, end_idx - 1);
    end_idx = std::max(start_idx + 1, end_idx - margin);

    // ����� ������������ � ������������� �������� � �������������� IPP
    float mean_val, max_val;
    IppStatus status = ippsMin_32f(&data[start_idx], static_cast<int>(end_idx - start_idx), &mean_val/*, IppHintAlgorithm::ippAlgHintFast*/);
    if (status != ippStsNoErr) {
        return;                            // ������ ��� ������� ��������
    }
    
    status = ippsMax_32f(&data[start_idx], static_cast<int>(end_idx - start_idx), &max_val);
    if (status != ippStsNoErr) {
        return;                            // ������ ��� ������� ���������
    }




    // ������������ ����� ������ � ������ �������
    Limits<double> new_bounds{static_cast<double>(mean_val), static_cast<double>(max_val)};
    double delta = new_bounds.delta();
    new_bounds.low -= delta * power_margins_.low;   // �������� ������ �������
    new_bounds.high += delta * power_margins_.high; // ��������� ������� �������

    // ����������� � �������� ���������: ���������� ������ ������
    Limits<double> current_bounds = power_bounds_.load();
	if (!need_reset_bounds_) {
		new_bounds.low	= std::min(current_bounds.low, new_bounds.low);
		new_bounds.high = std::max(new_bounds.high, current_bounds.high);
	}	
    // ��������� ���������� ������ ��������
    power_bounds_ = new_bounds;
	need_reset_bounds_ = false;
}

bool PowerLimitMan::IsAdaptiveMode() const {
    return is_adaptive_mode_.load();       // �������� ������ ������������
}

void PowerLimitMan::EnableAdaptiveMode(const bool is_true)
{
    is_adaptive_mode_ = is_true;           // ���������/���������� ����������� ������
}

void aqua_gui::PowerLimitMan::ResetBounds()
{
	need_reset_bounds_ = true;
}

bool aqua_gui::PowerLimitMan::NeedRelevantBounds() const
{
	return need_reset_bounds_;
}
