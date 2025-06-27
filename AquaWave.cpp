#include <qmessagebox.h>
#include "AquaWave.h"
AquaWave::AquaWave(QWidget *parent, const QString& file_path)
    : QMainWindow(parent)
{
    ui.setupUi(this); // ������������� ����������������� ���������� ��� �������� �������.
    file_src_ = std::make_shared<file_source::FileSourceArk>(); // �������� ��������� ������.

    // ���������� ������� ��� �������� ���� "����� ����".
    {
        connect(ui.new_file_menu_action, &QAction::triggered, [this]()
        {
            // �������� ������� �� ��������� ����������� ����.
            fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>();
            req_dove->base_thought = fluctus::DoveParrent::DoveThought::kGetDialog;

            // �������� ������� ��������� ������.
            if (file_src_->SendDove(req_dove))
            {
                // ����������� ����������� ������� (����������� ����).
                ((req_dove->show_widget))->show();// exec();
            }
        });
    }

    spectrum_chart_ = std::make_shared<dpx_core::SpectrumDPX>(); // �������� ���������� ��� ������������� �������.
    spectrogram_ = std::make_shared<spg_core::Spectrogram>(); // �������� ���������� ��� �������������.

    // ������ � ���������� ������� ������������� ������� �� ������� "Spectre".
    fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>(); // ��������� ������������� ��� �������� ������ �������.
    req_dove->base_thought = fluctus::DoveParrent::kGetDialog; // ��������� ���� �������: �������� ������/������.
    {
        spectrum_chart_->SendDove(req_dove); // �������� ������� ���������� ������������� �������.
        auto spectrum_widget = req_dove->show_widget; // ��������� ������� ������� �� �������.
        // ���������� ����������� ������� �� ������� "Spectre".
        this->ui.spectre_tab->layout()->addWidget(spectrum_widget.get());
    }

    // ������ � ���������� ������� ������������� �� Frame "TimeFreqFrame".
    {
        spectrogram_->SendDove(req_dove); // �������� ���� �� ������� ���������� �������������.
        auto spg_widget = req_dove->show_widget; // ��������� ������� ������������� �� �������.
        // ���������� ����������� ������� �� Frame "TimeFreqFrame".
        this->ui.TimeFreqFrame->layout()->addWidget(spg_widget.get());
    }

    // ��������� ������ ����� ������������ (��������, "��������" ��������� ������).
    {
        req_dove->base_thought = fluctus::DoveParrent::kTieBehind; // ��������� ���� �������: ���������.
        req_dove->target_ark = file_src_; // �������� ���� ��� �������� (�������� ������).
        spectrogram_->SendDove(req_dove); // �������� ������� �� �������� � �������������.
        spectrum_chart_->SendDove(req_dove); // �������� ������� �� �������� � ������������� �������.

        req_dove->base_thought = fluctus::DoveParrent::kTieFront;

        req_dove->target_ark = spectrogram_;
        file_src_->SendDove(req_dove); // �������� ������� �� �������� � �������������.

        req_dove->target_ark = spectrum_chart_;
        file_src_->SendDove(req_dove); // �������� ������� �� �������� � ������������� �������.
    }

    if(!file_path.isEmpty()) //���� ��������� ����� ����
    {
        auto file_dove = std::make_shared<file_source::FileSrcDove>();
        file_dove->special_thought = file_source::FileSrcDove::kSetFileName;
        file_dove->file_info = file_source::file_params();
        (*file_dove->file_info).file_name_ = file_path;
        file_src_->SendDove(file_dove);
    }
    ui.harmonics_viewer_tab_widget->setCurrentIndex(0); // ��������� �������� ������� �� ���������.
}

AquaWave::~AquaWave()
{}
