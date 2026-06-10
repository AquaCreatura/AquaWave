#include "ShipBuilder.h"
#include "Arks/Spectral Viewer/SpectralViewer.h"
#include "Arks/File Source/FileSource.h"
#include "Arks/Scope Analyzer/ScopeAnalyzer.h"
#include "Arks/SelectionWriter/SelectionWriter.h"
fluctus::ArkSptr ShipBuilder::BuildNewShip(fluctus::ArkType ship_type, QWidget* main)
{
	fluctus::ArkSptr ark;
	switch (ship_type)
	{
	case fluctus::kFileSource:		ark = std::make_shared<file_source::FileSourceArk>(main);
		break;
	case fluctus::kSpectralViewer:	ark = std::make_shared<spectral_viewer::SpectralViewer>();
		break;
	case fluctus::kScopeAnalyser:	ark = std::make_shared<scope_analyzer::ScopeAnalyzer>();
		break;
	case fluctus::kSelectionWriter:	ark = std::make_shared<file_writer::SelectionWriter>();
		break;
	default:
		break;
	};
	if (ark) fleet.push_back(ark);
	return ark;
}

bool ShipBuilder::Bind_SrcSink(fluctus::ArkSptr source_ark, fluctus::ArkSptr sink_ark)
{
	fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>();

	req_dove->base_thought = fluctus::DoveParrent::kTieSource; 
	req_dove->target_ark = source_ark;
	if (!sink_ark->PostDove(req_dove)) return false;
	

	req_dove->base_thought = fluctus::DoveParrent::kTieSink;
	req_dove->target_ark = sink_ark;
	if (!source_ark->PostDove(req_dove)) return false;

	return true;
}

QPointer<QWidget> ShipBuilder::GetWindow(fluctus::ArkSptr ship)
{
	fluctus::DoveSptr req_dove = std::make_shared<fluctus::DoveParrent>(); 
	req_dove->base_thought = fluctus::DoveParrent::kGetWindow; 
	ship->PostDove(req_dove);
	return req_dove->show_widget;
}
