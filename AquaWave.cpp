#include <qmessagebox.h>
#include "AquaWave.h"
AquaWave::AquaWave(QWidget *parent, const QString& file_path)
    : QMainWindow(parent)
{
    ui.setupUi(this); // Инициализация пользовательского интерфейса для текущего виджета.
    file_src_ = std::make_shared<file_source::FileSourceArk>(); // Создание источника файлов.

    // Обработчик события для действия меню "Новый файл".
    {
        connect(ui.new_file_menu_action, &QAction::triggered, [this]()
        {
            // Создание запроса на получение диалогового окна.
            fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>();
            req_dove->base_thought = fluctus::DoveParrent::DoveThought::kGetDialog;

            // Отправка запроса источнику файлов.
            if (file_src_->SendDove(req_dove))
            {
                // Отображение полученного виджета (диалогового окна).
                ((req_dove->show_widget))->show();// exec();
            }
        });
    }

    spectrum_chart_ = std::make_shared<dpx_core::SpectrumDPX>(); // Создание компонента для спектрального графика.
    spectrogram_ = std::make_shared<spg_core::Spectrogram>(); // Создание компонента для спектрограммы.

    // Запрос и добавление виджета спектрального графика на вкладку "Spectre".
    fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>(); // Повторное использование или создание нового запроса.
    req_dove->base_thought = fluctus::DoveParrent::kGetDialog; // Установка типа запроса: получить диалог/виджет.
    {
        spectrum_chart_->SendDove(req_dove); // Отправка запроса компоненту спектрального графика.
        auto spectrum_widget = req_dove->show_widget; // Получение виджета графика из запроса.
        // Добавление полученного виджета на вкладку "Spectre".
        this->ui.spectre_tab->layout()->addWidget(spectrum_widget.get());
    }

    // Запрос и добавление виджета спектрограммы на Frame "TimeFreqFrame".
    {
        spectrogram_->SendDove(req_dove); // Отправка того же запроса компоненту спектрограммы.
        auto spg_widget = req_dove->show_widget; // Получение виджета спектрограммы из запроса.
        // Добавление полученного виджета на Frame "TimeFreqFrame".
        this->ui.TimeFreqFrame->layout()->addWidget(spg_widget.get());
    }

    // Установка связей между компонентами (например, "привязка" источника данных).
    {
        req_dove->base_thought = fluctus::DoveParrent::kTieBehind; // Изменение типа запроса: привязать.
        req_dove->target_ark = file_src_; // Указание цели для привязки (источник файлов).
        spectrogram_->SendDove(req_dove); // Отправка запроса на привязку к спектрограмме.
        spectrum_chart_->SendDove(req_dove); // Отправка запроса на привязку к спектральному графику.

        req_dove->base_thought = fluctus::DoveParrent::kTieFront;

        req_dove->target_ark = spectrogram_;
        file_src_->SendDove(req_dove); // Отправка запроса на привязку к спектрограмме.

        req_dove->target_ark = spectrum_chart_;
        file_src_->SendDove(req_dove); // Отправка запроса на привязку к спектральному графику.
    }

    if(!file_path.isEmpty()) //Если запускали через файл
    {
        auto file_dove = std::make_shared<file_source::FileSrcDove>();
        file_dove->special_thought = file_source::FileSrcDove::kSetFileName;
        file_dove->file_info = file_source::file_params();
        (*file_dove->file_info).file_name_ = file_path;
        file_src_->SendDove(file_dove);
    }
    ui.harmonics_viewer_tab_widget->setCurrentIndex(0); // Установка активной вкладки по умолчанию.
}

AquaWave::~AquaWave()
{}
