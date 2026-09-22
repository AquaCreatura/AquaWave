#include <QFile>
#include <QApplication>
#include "AquaWave.h"
#include "special_defs/file_souce_defs.h"
#include "Utilities/qt_utility.h"
#include "GUI/External/Window/QMainMenuHelper.h"
#include "GUI/External/Window/QWindowKitHelper.h"

AquaWave::AquaWave(QWidget *parent, const QString& file_path)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    setWindowIcon(QIcon(":/AquaWave/sources/GUI/External/icons/wave.ico"));

    m_mainMenuHelper = new QMainMenuHelper(this);
    m_windowKitHelper = new QWindowKitHelper(this);

    setupWindowAgent();

    QFile file(":/AquaWave/sources/GUI/External/Themes/space_scheme.qss");
    if (file.open(QFile::ReadOnly)) {
        QString style = file.readAll();
        setStyleSheet(style);
    }

    file_src_ = ship_builder_.BuildNewShip(fluctus::kFileSource, this);
    spectral_viewer_ = ship_builder_.BuildNewShip(fluctus::kSpectralViewer);
    selection_writer_ = ship_builder_.BuildNewShip(fluctus::kSelectionWriter);
    demodulator_ = ship_builder_.BuildNewShip(fluctus::kBaseDemodulator);

    ShipBuilder::Bind_SrcSink(file_src_, spectral_viewer_);
    ShipBuilder::Bind_SrcSink(file_src_, selection_writer_);
    ShipBuilder::Bind_SrcSink(file_src_, demodulator_);
    ShipBuilder::Bind_SrcSink(spectral_viewer_, selection_writer_);

    auto file_window = ShipBuilder::GetWindow(file_src_);
    qApp->setStyleSheet(styleSheet());

    if (!file_path.isEmpty()) {
        auto file_dove = std::make_shared<file_source::FileSrcDove>(file_source::FileSrcDove::kSetFileName);
        file_dove->description = fluctus::SourceDescription();
        file_dove->description->file_name_ = file_path;
        file_src_->PostDove(file_dove);
    }

    pages_ = {
        {
            ShipBuilder::GetWindow(spectral_viewer_),
            [this](auto d) { spectral_viewer_->PostDove(d); },
            "Spectral",
            ":/buttons/button_images/spectrum.png"
        },
        {
            ShipBuilder::GetWindow(demodulator_),
            [this](auto d) { demodulator_->PostDove(d); },
            "Demodulator",
            ":/buttons/button_images/analyze_icon.png"
        },
    };

    for (auto& p : pages_) {
        QIcon icon = buildButtonIcon(p.iconPath, p.iconPath, p.iconPath, p.iconPath, p.iconPath);
        ui.main_tab_widget->addTab(p.widget, icon, p.title);
    }

    auto applyTabState = [this]() {
        QWidget* current = ui.main_tab_widget->currentWidget();
        for (auto& p : pages_) {
            auto dove = std::make_shared<fluctus::DoveParrent>(
                (p.widget == current) ? fluctus::DoveParrent::kActivate
                                      : fluctus::DoveParrent::kDeactivate);
            p.postDove(dove);
        }
    };

    connect(ui.main_tab_widget, &QTabWidget::currentChanged, this, [applyTabState](int) {
        applyTabState();
    });

    if (!pages_.empty() && ui.main_tab_widget->currentWidget() != nullptr) {
        applyTabState();
    }
}

AquaWave::~AquaWave() = default;

void AquaWave::setupWindowAgent()
{
    QMenuBar* menuBar = m_mainMenuHelper->createMenuBar(this);

    connect(m_mainMenuHelper, &QMainMenuHelper::openFileRequested, this, [this]() {
        auto file_window = ShipBuilder::GetWindow(file_src_);
        file_window->show();
    });

    m_windowKitHelper->setupTitleBar(menuBar);
}
