#include <qmessagebox.h>
#include "AquaWave.h"
#include "special_defs/file_souce_defs.h"

AquaWave::AquaWave(QWidget *parent, const QString& file_path)
    : QMainWindow(parent)
{
	//(QString&)file_path = "D:\\signals\\17.10.2025 16_41_59 1875.300000MHz 12800.000KHz.pcm";
    ui.setupUi(this); 
	file_src_				= ship_builder_.BuildNewShip(fluctus::kFileSource);
	spectral_viewer	= ship_builder_.BuildNewShip(fluctus::kSpectralViewer);
	scope_analyser		= ship_builder_.BuildNewShip(fluctus::kScopeAnalyser);

	ShipBuilder::Bind_SrcSink(file_src_, spectral_viewer);
	ShipBuilder::Bind_SrcSink(file_src_, scope_analyser);

	
	connect(ui.new_file_menu_action, &QAction::triggered, [this]()
	{
		auto file_window = ShipBuilder::GetWindow(file_src_);
		file_window->show();
	});

	if (!file_path.isEmpty()) //Если запускали через файл - инициализируем файловый источник
	{
		auto file_dove = std::make_shared<file_source::FileSrcDove>();
		file_dove->special_thought = file_source::FileSrcDove::kSetFileName;
		file_dove->file_info = file_source::file_params();
		(*file_dove->file_info).file_name_ = file_path;
		file_src_->SendDove(file_dove);
	}

	this->ui.main_stacked->addWidget(ShipBuilder::GetWindow(spectral_viewer));
	this->ui.main_stacked->addWidget(ShipBuilder::GetWindow(scope_analyser));
	connect(ui.spectral_viewer_navigate_button, &QPushButton::clicked, [this](){
		ui.main_stacked->setCurrentWidget(ShipBuilder::GetWindow(spectral_viewer));
	});
	connect(ui.analyze_navigate_button, &QPushButton::clicked, [this]() {
		ui.main_stacked->setCurrentWidget(ShipBuilder::GetWindow(scope_analyser));
	});
	
	//connect
	this->ui.main_stacked->setCurrentWidget(ShipBuilder::GetWindow(spectral_viewer));
}

AquaWave::~AquaWave()
{
	
}
