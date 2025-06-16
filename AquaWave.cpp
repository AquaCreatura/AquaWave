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

        //fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>();
        //req_dove->base_thought = fluctus::DoveParrent::kGetDialog;
        //if (file_src_->SendDove(req_dove))
        //{
        //    auto &res = req_dove->show_widget.value();
        //    if (res)
        //        res->exec();
        //    else
        //        throw std::runtime_error("something happened!(");
        //}
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
            req_dove->data_size         = 1'024 * 16;
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
    if (!spectrum_chart_->SendDove(req_dove))
    {
        QMessageBox::warning(
                                    nullptr,                        // родительское окно (может быть this)
                                    "Cannot Get QDialog",            // заголовок окна
                                    "DPX spectrum doesn't return QWidget..."  // сообщение
                                );
    };
    spectrum_widget = req_dove->show_widget;



    //spectrum_chart_->setLayout(new QVBoxLayout());
    //spectrum_chart_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if(spectrum_widget)
    {
        this->ui.spectre_groupbox->layout()->replaceWidget(ui.single_chart_widget, spectrum_widget.get());
        ui.harmonics_viewer_tab_widget->setCurrentIndex(0);
        if(auto casted_widget = std::dynamic_pointer_cast<ChartInterface>(spectrum_widget))
        {
            casted_widget->SetBackgroundImage("D:\\AquaCreatura\\AquaWave\\third_party\\background\\foxy_dark_cut.png"); 
        }
    }

}

AquaWave::~AquaWave()
{}
