#include <qmessagebox.h>
#include "AquaWave.h"
AquaWave::AquaWave(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    file_src_ = std::make_shared<file_source::FileSourceArk>();
    //file source
    {
        connect(ui.new_file_menu_action, &QAction::triggered, [this]()
        {
            fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>();
            req_dove->base_thought = fluctus::DoveParrent::DoveThought::kGetDialog;
            if (file_src_->SendDove(req_dove))
            {
                ((req_dove->show_widget))->show();// exec();
            }
        });
    }
    spectrum_chart_ = std::make_shared<dpx_core::SpectrumDPX>(ui.single_chart_widget);
    //Spectrum debug
    {
        ui.do_something;
        connect(ui.do_something, &QPushButton::clicked, [this]()
        {
            auto req_dove = std::make_shared<file_source::FileSrcDove>();
            req_dove->base_thought      = fluctus::DoveParrent::DoveThought::kSpecialThought;
            req_dove->special_thought   = file_source::FileSrcDove::kInitReaderInfo |  file_source::FileSrcDove::kAskSingleDataAround;
            req_dove->target_ark        = spectrum_chart_;
            req_dove->time_point_start  = 0.5;
            req_dove->data_size         = 1'024 * 32;
            if (!file_src_->SendDove(req_dove))
            {
                QMessageBox::warning(
                                    nullptr,                        // родительское окно (может быть this)
                                    "Cannot Send Data",            // заголовок окна
                                    "Do something with DPX or file source, or..."  // сообщение
                                );
            }
        });    
    
    }

    spectrum_chart_;
    
    std::shared_ptr<QWidget> spectrum_widget;

    
    fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>();
    req_dove->base_thought = fluctus::DoveParrent::kGetDialog;
    {
        if (!spectrum_chart_->SendDove(req_dove))
        {
            QMessageBox::warning(
                                        nullptr,                        // родительское окно (может быть this)
                                        "Cannot Get QDialog",            // заголовок окна
                                        "DPX spectrum doesn't return QWidget..."  // сообщение
                                    );
        };
        spectrum_widget = req_dove->show_widget;

        if(spectrum_widget)
        {
            this->ui.spectre_tab->layout()->replaceWidget(ui.single_chart_widget, spectrum_widget.get());
            ui.harmonics_viewer_tab_widget->setCurrentIndex(0);
            if(auto casted_widget = std::dynamic_pointer_cast<ChartInterface>(spectrum_widget))
            {
                casted_widget->SetBackgroundImage(":/AquaWave/third_party/background/dark_city_2_cut.jpg"); 
            }
        }
    }

    {
        spectrogram_ = std::make_shared<spg_core::Spectrogram>(ui.time_freq_frame);
        if (!spectrogram_->SendDove(req_dove))
        {
            QMessageBox::warning(
                                        nullptr,                        // родительское окно (может быть this)
                                        "Cannot Get QDialog",            // заголовок окна
                                        "Spectrogram doesn't return QWidget..."  // сообщение
                                    );
        };
        auto spg_widget = req_dove->show_widget;

        if(spg_widget)
        {
            this->ui.TimeFreqFrame->layout()->replaceWidget(ui.time_freq_frame, spg_widget.get());
            ui.harmonics_viewer_tab_widget->setCurrentIndex(0);
            if(auto casted_widget = std::dynamic_pointer_cast<ChartInterface>(spg_widget))
            {
                casted_widget->SetBackgroundImage(":/AquaWave/third_party/background/dark_city_2_cut.jpg"); 
            }
        }
        req_dove->base_thought = fluctus::DoveParrent::kTieBehind;
        req_dove->target_ark   = file_src_;
        if(!spectrogram_->SendDove(req_dove))
        {
            QMessageBox::warning(
                                            nullptr,                        // родительское окно (может быть this)
                                            "Cannot Tie File signal source",            // заголовок окна
                                            "Spectrogram doesn't return QWidget..."  // сообщение
                                        );
        }
    }
    
    

}

AquaWave::~AquaWave()
{}
