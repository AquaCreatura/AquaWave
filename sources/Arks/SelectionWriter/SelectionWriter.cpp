#include "SelectionWriter.h"
#include "Arks/Interfaces/special_defs/file_writer_defs.h"
file_writer::SelectionWriter::SelectionWriter()
{
}

file_writer::SelectionWriter::~SelectionWriter()
{
}

bool file_writer::SelectionWriter::SendData(fluctus::DataInfo const & data_info)
{
	return false;
}

bool file_writer::SelectionWriter::PostDove(fluctus::DoveSptr const & sent_dove)
{
	// Получаем целевое значение и "мысль" из сообщения.
	auto target_val = sent_dove->target_ark;
	auto base_thought = sent_dove->base_thought;

	if (base_thought == fluctus::DoveParrent::DoveThought::kTieSource)
	{		
		src_info_.ark = target_val;
	}
	if (base_thought == fluctus::DoveParrent::DoveThought::kReset)
	{
		UpdateSource();
	}
	if (base_thought & fluctus::DoveParrent::DoveThought::kSpecialThought) {
		const auto special_thought = sent_dove->special_thought;
		if (auto fw_dove = std::dynamic_pointer_cast<FileWriterDove>(sent_dove)) {

			if (special_thought & file_writer::FileWriterDove::SpecThought::kRecordSelection) {
				return StartSelectionRecord(fw_dove->freq_bounds_hz,fw_dove->file_bounds_ratio);
			}
		};
	}
	// Передаём сообщение базовому классу для дальнейшей обработки.
	return ArkBase::PostDove(sent_dove);
}

fluctus::ArkType file_writer::SelectionWriter::GetArkType() const
{
	return fluctus::ArkType::kSelectionWriter;
}

void file_writer::SelectionWriter::UpdateSource()
{
	auto file_src = src_info_.ark.lock();

	if (!file_src) return;


	auto parrent_dove = std::make_shared<fluctus::DoveParrent>(fluctus::DoveParrent::kGetDescription);
	if (!file_src->PostDove(parrent_dove) || !parrent_dove->description) {
		return;
	}
	src_info_.descr = *parrent_dove->description;
	const int max_order = std::min(log2(parrent_dove->description->count_of_samples), 21.);
	return;
}

bool file_writer::SelectionWriter::StartSelectionRecord(fluctus::Limits<double> freq_bounds_hz, fluctus::Limits<double> time_bounds)
{
	window_.exec();
	return true;
}
