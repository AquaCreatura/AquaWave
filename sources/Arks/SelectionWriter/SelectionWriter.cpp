#include "SelectionWriter.h"
#include <qmessagebox.h>
#include "Arks/Interfaces/special_defs/file_writer_defs.h"
#include "Arks/Interfaces/special_defs/file_souce_defs.h"
#include "Utilities/parse_tools.h"
constexpr int c_expr_read_chunk_size_ = 4096;
file_writer::SelectionWriter::SelectionWriter()
{
	connect(&window_, &SelectionWriterWindow::StartNeed, this, &SelectionWriter::StartRecording);
	connect(&window_, &SelectionWriterWindow::StopNeed, this, &SelectionWriter::StopRecording);
	connect(&window_, &SelectionWriterWindow::IsStarted, [this]()->bool {return is_started_.load(); });


	{ //Таймер
		connect(&stop_timer_, &QTimer::timeout,this, &SelectionWriter::OnStopTimerEvent);
		stop_timer_.setInterval(interval_stop_msec_);
		stop_timer_.setSingleShot(true);
	}
}
file_writer::SelectionWriter::~SelectionWriter()
{
}

bool file_writer::SelectionWriter::SendData(fluctus::DataInfo const & data_info) //Вызывается асинхронно не в потоке GUI
{
	//Закидываем данные
	{
		std::vector<Ipp32fc>& passed_32fc = (std::vector<Ipp32fc>&)data_info.data_vec;
		casted_16sc_.resize(passed_32fc.size());
		ippsConvert_32f16s_Sfs((Ipp32f*)passed_32fc.data(), (Ipp16s*)casted_16sc_.data(), casted_16sc_.size() * 2, IppRoundMode::ippRndNear, 0);
		if (!writer_.WriteData((std::vector<uint8_t>&)casted_16sc_))
		{
			window_.Stop();
			return false;
		}
	}
	//Обновляем статус
	if (status_update_timer_.elapsed() > 0) {
		double passed_ratio = data_info.time_point; time_bounds_.pos(data_info.time_point);
		QMetaObject::invokeMethod(&window_, [=]() {
			window_.UpdateProgressRatio(passed_ratio);
			window_.UpdateBytesWritten(writer_.GetCurSizeBytes());
		}, Qt::QueuedConnection);
		status_update_timer_.start();
	}

	return true;
}

bool file_writer::SelectionWriter::PostDove(fluctus::DoveSptr const & sent_dove)
{
	// Получаем целевое значение и "мысль" из сообщения.
	auto target_val = sent_dove->target_ark;
	auto base_thought = sent_dove->base_thought;

	if (base_thought == fluctus::DoveParrent::DoveThought::kTieSource)
	{		
		if (target_val->GetArkType() == ArkType::kFileSource)
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
				return InitSelectionRecord(fw_dove->freq_bounds_hz,fw_dove->file_bounds_ratio);
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

bool file_writer::SelectionWriter::InitSelectionRecord(fluctus::Limits<double> freq_bounds_hz, fluctus::Limits<double> time_bounds)
{
	time_bounds_ = time_bounds;
	selection_bound_hz_ = freq_bounds_hz;
	window_.SetFreqparams(freq_bounds_hz.mid(), freq_bounds_hz.delta());
	window_.UpdateProgressRatio(0.);
	window_.UpdateBytesWritten(0);

	window_.exec();
	return true;
}

//folder_path может прийти в любом формате, в том числе и пустой, может с /,а может и с \\, может иметь в конце слэш, а может нет.
bool file_writer::SelectionWriter::StartRecording(const std::string folder_path)
{
	window_.UpdateProgressRatio(0.);
	window_.UpdateBytesWritten(0);


	{
		auto file_src = src_info_.ark.lock();
		auto req_dove = std::make_shared<file_source::FileSrcDove>();
		req_dove->sender = shared_from_this();
		req_dove->special_thought = file_source::FileSrcDove::kInitiate | file_source::FileSrcDove::kAskChunksInRange;
		req_dove->target_ark = shared_from_this();

		req_dove->time_bounds = time_bounds_;

		req_dove->setup.emplace();
		auto &setup = req_dove->setup;
		setup->carrier_hz = selection_bound_hz_.mid();
		setup->chunk_size = c_expr_read_chunk_size_;
		setup->banwidth_hz = selection_bound_hz_.delta();
		setup->samplerate_hz = setup->banwidth_hz / src_info_.descr.bw_ratio_;
		if (!file_src->PostDove(req_dove))
		{
			QMessageBox::warning(nullptr, "Error", "Can not start recording");
			return false;
		}
		work_samplerate_hz_ = req_dove->setup->samplerate_hz;
	}

	if (!CaptureFile(folder_path)) {
		return false;
	}



	is_started_ = true;
	status_update_timer_.start();
	stop_timer_.start();
	return true;
}

bool file_writer::SelectionWriter::CaptureFile(const std::string folder_path)
{
	// 1. Normalize folder path (handle / vs \, trailing separator, empty case)
	std::string base_folder = folder_path;

	if (base_folder.empty()) {
		base_folder = ".";  // current directory
	}

	// Normalize separators to '/' (most cross-platform friendly)
	for (char& c : base_folder) {
		if (c == '\\') c = '/';
	}

	// Ensure trailing slash
	if (!base_folder.empty() && base_folder.back() != '/') {
		base_folder += '/';
	}

	// 2. Generate base filename
	auto time_string = aqua_parse_tools::gen_time_string();
	std::string file_name = aqua_parse_tools::generate_filename(
		selection_bound_hz_.mid(),
		work_samplerate_hz_,
		time_string,
		"pcm"
	);

	// 3. Find unique filename with postfix if needed
	std::string file_full_path;
	std::string postfix = "";

	for (int i = 0; i < 3; ++i) {
		std::string candidate = base_folder + file_name;

		if (!postfix.empty()) {
			// Insert postfix before extension
			size_t dot_pos = candidate.find_last_of('.');
			if (dot_pos != std::string::npos) {
				candidate.insert(dot_pos, postfix);
			}
			else {
				candidate += postfix;
			}
		}

		if (!FsHelper::IsFileExist(candidate)) {
			file_full_path = std::move(candidate);
			break;
		}
		postfix += '_';
	}
	if (file_full_path.empty()) {
		return false;
	}
	if (!writer_.CaptureFile(file_full_path, true)) {
		QMessageBox::warning(nullptr, "Error", "Wrong folder path");
		return false;
	}
	return true;
}

bool file_writer::SelectionWriter::StopRecording()
{
	is_started_ = false;
	QMetaObject::invokeMethod(&window_, [=]() {
		window_.UpdateProgressRatio(1.);
		window_.UpdateBytesWritten(writer_.GetCurSizeBytes());
	}, Qt::QueuedConnection);
	window_.close();
	FileSavedDialog(writer_.GetFilePath(), writer_.GetCurSizeBytes()).exec();
	writer_.ReleaseFile();
	return true;
}

void file_writer::SelectionWriter::OnStopTimerEvent()
{
	if (status_update_timer_.elapsed() > 300) { 
		window_.Stop();
	}
	else 
		stop_timer_.start();
}
