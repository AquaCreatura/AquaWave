#pragma once

#include "aqua_defines.h"
#include <vector>
#include <atomic>

using namespace fluctus;

namespace aqua_gui
{

class PowerLimitMan
{
public:
    PowerLimitMan();  // �����������: ������������� �������� �������� � ������

    // ��������� ����� ������ ������������� ���������
    void SetNewViewBounds(const Limits<double>& x_bounds);

    // ������ ��������� ������ ��������
    void SetPowerBounds(const Limits<double>& power_bounds);

    // ��������� ����� (margins) ��� ���������� ������ ��������
    void SetPowerMargins(const Limits<double>& power_margins);

    // ��������� ������� ������ ��������
    Limits<double> GetPowerBounds() const;

    // ���������� ������ �������� �� ������ ������ � �� ���������
    void UpdateBounds(const std::vector<float>& data, const Limits<double>& data_bounds);

    // ��������, ������� �� ���������� �����
    bool IsAdaptiveMode() const;

    // ���������/���������� ����������� ������
    void EnableAdaptiveMode(const bool is_true);

	void ResetBounds();

	bool NeedRelevantBounds() const;

protected:
    Limits<double> x_bounds_;                         // ��������������� (�� ������������ � ����������)
    std::atomic_bool is_adaptive_mode_ = false;       // ���� ����������� ������
	std::atomic_bool need_reset_bounds_		= false;       
	std::atomic_bool need_update_max_		= false;       

    std::atomic<Limits<double>> power_bounds_;        // ���������� ������� ��������
    std::atomic<Limits<double>> view_bounds_;         // ������� ������� (������������ ��� ���������)
    Limits<double> power_margins_;                    // ������ (margins) ��� ��������� ������ ��������
};

}
