#include  <qmessagebox.h>
#include <QFile>
#include <QApplication>
#include "AquaWave.h"
#include "special_defs/file_souce_defs.h"
#include "Utilities/qt_utility.h"

#include <qtoolbar.h>
#include <qpushbutton.h>
#include <qtabbar.h>
#include "GUI/External/Window/FramelessHelper.h"
AquaWave::AquaWave(QWidget *parent, const QString& file_path)
    : QMainWindow(parent)
{
	ui.setupUi(this);

	auto* h = new FramelessHelper(this);
	h->setTitleBar(ui.main_button_panel);

	//(QString&)file_path = "D:\\signals\\17.10.2025 16_41_59 1875.300000MHz 12800.000KHz.pcm";
	//red_scheme
	QFile file(":/AquaWave/sources/GUI/External/Themes/space_scheme.qss");
	if (file.open(QFile::ReadOnly)) {
		QString style = file.readAll();
		setStyleSheet(style);
	}


	file_src_			= ship_builder_.BuildNewShip(fluctus::kFileSource, this);
	spectral_viewer_	= ship_builder_.BuildNewShip(fluctus::kSpectralViewer);
	scope_analyser_		= ship_builder_.BuildNewShip(fluctus::kScopeAnalyser);
	selection_writer_	= ship_builder_.BuildNewShip(fluctus::kSelectionWriter);

	ShipBuilder::Bind_SrcSink(file_src_, spectral_viewer_);
	ShipBuilder::Bind_SrcSink(file_src_, scope_analyser_);
	ShipBuilder::Bind_SrcSink(file_src_, selection_writer_);
	ShipBuilder::Bind_SrcSink(spectral_viewer_, scope_analyser_);
	ShipBuilder::Bind_SrcSink(spectral_viewer_, selection_writer_);

	

	{
		auto file_window = ShipBuilder::GetWindow(file_src_);
		qApp->setStyleSheet(styleSheet());
		//file_window->setStyleSheet(styleSheet());
	}

	connect(ui.new_file_menu_action, &QAction::triggered, [this]()
	{
		auto file_window = ShipBuilder::GetWindow(file_src_);
		//file_window->setStyleSheet(styleSheet());
		file_window->show();
	});
	connect(ui.actionHot_keys, &QAction::triggered, this, [this]()
	{
		QMessageBox::information(
			this,
			tr("Hot Keys"),
			tr(
				"<pre>"
				"Enter      - Zoom In\n"
				"Backspace  - Zoom Out\n"
				"Ctrl+T     - Change Time Domain\n"
				"Ctrl+S     - Save Precise Selection"
				"</pre>"
			)
		);
	}); //actionContact_us
	connect(ui.actionContact_us, &QAction::triggered, this, [this]()
	{
		QMessageBox::information(
			this,
			tr("Contact us"),
			tr(
				"<pre>"
				"e-mail : AquaCreatura@gmail.com\n"
				"tg     : @AquaCreatura\n"
				"Phone  : +7 (921) - 6453 - 763\n"
				"If you have any problems or ideas\n - send request in text view"
				"</pre>"
			)
		);
	});


	if (!file_path.isEmpty()) //Если запускали через файл - инициализируем файловый источник
	{
		auto file_dove = std::make_shared<file_source::FileSrcDove>(file_source::FileSrcDove::kSetFileName);
		file_dove->description = fluctus::SourceDescription();
		file_dove->description->file_name_ = file_path;
		file_src_->PostDove(file_dove);
	}

	pages_ = {
		{
			ShipBuilder::GetWindow(spectral_viewer_),
			[this](auto d) { spectral_viewer_->PostDove(d); },
			("Spectral"),
			(":/buttons/button_images/spectrum.png")
		},
		{
			ShipBuilder::GetWindow(scope_analyser_),
			[this](auto d) { scope_analyser_->PostDove(d); },
			("Analyze"),
			(":/buttons/button_images/analyze_icon.png")
		},
		// future pages go here...
	};

	// 2. Регистрируем вкладки + иконки — один цикл на всё.
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

	connect(ui.main_tab_widget, &QTabWidget::currentChanged,
		this, [applyTabState](int) { applyTabState(); });

	// Начальное состояние — вызвать явно
	if (!pages_.empty() && ui.main_tab_widget->currentWidget() != nullptr) {
		applyTabState();
	}

}

AquaWave::~AquaWave()
{
	
}