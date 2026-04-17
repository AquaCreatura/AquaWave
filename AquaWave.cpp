#include <qmessagebox.h>
#include <QFile>
#include <QApplication>

#include "AquaWave.h"
#include "special_defs/file_souce_defs.h"
#include "Tools/qt_utility.h"
AquaWave::AquaWave(QWidget *parent, const QString& file_path)
    : QMainWindow(parent)
{
	//(QString&)file_path = "D:\\signals\\17.10.2025 16_41_59 1875.300000MHz 12800.000KHz.pcm";
    ui.setupUi(this); 
	file_src_				= ship_builder_.BuildNewShip(fluctus::kFileSource, this);
	spectral_viewer_	= ship_builder_.BuildNewShip(fluctus::kSpectralViewer);
	scope_analyser_		= ship_builder_.BuildNewShip(fluctus::kScopeAnalyser);

	ShipBuilder::Bind_SrcSink(file_src_, spectral_viewer_);
	ShipBuilder::Bind_SrcSink(file_src_, scope_analyser_);
	ShipBuilder::Bind_SrcSink(spectral_viewer_, scope_analyser_);

	
	connect(ui.new_file_menu_action, &QAction::triggered, [this]()
	{
		auto file_window = ShipBuilder::GetWindow(file_src_);
		file_window->setStyleSheet(styleSheet());
		file_window->show();
	});

	if (!file_path.isEmpty()) //Если запускали через файл - инициализируем файловый источник
	{
		auto file_dove = std::make_shared<file_source::FileSrcDove>(file_source::FileSrcDove::kSetFileName);
		file_dove->description = fluctus::SourceDescription();
		file_dove->description->file_name_ = file_path;
		file_src_->SendDove(file_dove);
	}

	this->ui.main_stacked->addWidget(ShipBuilder::GetWindow(spectral_viewer_));
	this->ui.main_stacked->addWidget(ShipBuilder::GetWindow(scope_analyser_));
	connect(ui.spectral_viewer_navigate_button, &QPushButton::clicked, [this](){
		ui.main_stacked->setCurrentWidget(ShipBuilder::GetWindow(spectral_viewer_));
		spectral_viewer_->SendDove(std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kActivate));
		scope_analyser_->SendDove(std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kDeactivate));
	});
	connect(ui.analyze_navigate_button, &QPushButton::clicked, [this]() {
		ui.main_stacked->setCurrentWidget(ShipBuilder::GetWindow(scope_analyser_));
		scope_analyser_->SendDove(std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kActivate));
		spectral_viewer_->SendDove(std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kDeactivate));
	});
	ui.spectral_viewer_navigate_button->click();
	//connect
	//this->ui.main_stacked->setCurrentWidget(ShipBuilder::GetWindow(spectral_viewer_));


	//default_theme
	//red_scheme
	QFile file(":/AquaWave/sources/GUI/CSS_Themes/red_scheme.qss"); 
	if (file.open(QFile::ReadOnly)) {
		QString style = file.readAll();
		setStyleSheet(style);
	}


	QIcon analyzeIcon = buildButtonIcon(
		":/buttons/button_images/analyze_icon.png",
		":/buttons/button_images/analyze_icon.png",
		":/buttons/button_images/analyze_icon.png",
		":/buttons/button_images/analyze_icon.png",
		":/buttons/button_images/analyze_icon.png"
	);

	ui.analyze_navigate_button->setIcon(analyzeIcon);
	ui.analyze_navigate_button->setIconSize(QSize(32, 32));


	QIcon spectrumIcon = buildButtonIcon(
		":/buttons/button_images/spectrum.png",
		":/buttons/button_images/spectrum.png",
		":/buttons/button_images/spectrum.png",
		":/buttons/button_images/spectrum.png",
		":/buttons/button_images/spectrum.png"
	);

	ui.spectral_viewer_navigate_button->setIcon(spectrumIcon);
	ui.spectral_viewer_navigate_button->setIconSize(QSize(32, 32));

}

AquaWave::~AquaWave()
{
	
}
