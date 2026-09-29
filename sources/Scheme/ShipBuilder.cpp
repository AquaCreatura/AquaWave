#include "ShipBuilder.h"
#include "Arks/Spectral Viewer/SpectralViewer.h"
#include "Arks/File Source/FileSource.h"
#include "Arks/Scope Analyzer/ScopeAnalyzer.h"
#include "Arks/SelectionWriter/SelectionWriter.h"
#include "Arks/Demodulation/AquaDemod.h"
aqua::ArkSptr ShipBuilder::BuildNewShip(aqua::ArkType ship_type, QWidget* main)
{
	aqua::ArkSptr ark;
	switch (ship_type)
	{
	case aqua::kFileSource:		ark = std::make_shared<file_source::FileSourceArk>(main);
		break;
	case aqua::kSpectralViewer:	ark = std::make_shared<spectral_viewer::SpectralViewer>();
		break;
	case aqua::kScopeAnalyser:	ark = std::make_shared<scope_analyzer::ScopeAnalyzer>();
		break;
	case aqua::kSelectionWriter:	ark = std::make_shared<file_writer::SelectionWriter>();
		break;
	case aqua::kBaseDemodulator:	ark = std::make_shared<demodulation::AquaDemod>();
		break;
	default:
		break;
	};
	if (ark) fleet.push_back(ark);
	return ark;
}

bool ShipBuilder::Bind_SrcSink(aqua::ArkSptr source_ark, aqua::ArkSptr sink_ark)
{
	aqua::DoveSptr req_dove = std::make_shared<aqua::DoveParrent>();

	req_dove->base_thought = aqua::DoveParrent::kAddSource; 
	req_dove->target_ark = source_ark;
	if (!sink_ark->PostDove(req_dove)) return false;
	

	req_dove->base_thought = aqua::DoveParrent::kAddSink;
	req_dove->target_ark = sink_ark;
	if (!source_ark->PostDove(req_dove)) return false;

	return true;
}

QPointer<QWidget> ShipBuilder::GetWindow(aqua::ArkSptr ship)
{
	aqua::DoveSptr req_dove = std::make_shared<aqua::DoveParrent>(); 
	req_dove->base_thought = aqua::DoveParrent::kGetWindow; 
	ship->PostDove(req_dove);
	return req_dove->show_widget;
}
