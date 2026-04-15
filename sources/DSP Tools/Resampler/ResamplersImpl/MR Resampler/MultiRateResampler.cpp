#include <algorithm>
#include "MultiRateResampler.h"
using namespace aqua_resampler;

MultiRateResampler::MultiRateResampler() 
    : pSpec_(nullptr), up_factor_(0), down_factor_(0), base_rate_(0) {}

MultiRateResampler::~MultiRateResampler() 
{
    Clear();
}

void aqua_resampler::MultiRateResampler::SetPrecision(const int quality)
{
	settings_.denom_quality = std::max(1,std::min(20, quality));
}



void aqua_resampler::MultiRateResampler::SetSettings(const ResamplerSettings s)
{
	if (settings_.need_norm_power == s.need_norm_power &&
		settings_.denom_quality == s.denom_quality)
	{
		return;
	}
    settings_ = s;
	need_reset_ = true;
}

bool MultiRateResampler::Init(const int64_t base_fs_hz, int64_t& tgt_sr_hz, const int64_t bw_hz) 
{
	if (base_fs_hz <= 0 || tgt_sr_hz <= 0 || bw_hz > tgt_sr_hz)
        return false;
	// Расчёт коэффициентов ресэмплинга
    const double need_ratio = static_cast<double>(tgt_sr_hz) / base_fs_hz;

	int max_nom_denom = int(std::ceil(std::max(need_ratio, 1 / need_ratio)));
	max_nom_denom = std::max(settings_.denom_quality, max_nom_denom);
    const auto up_down = aqua_resampler::FindBestFraction(need_ratio, max_nom_denom, true);

	// Корректировка конечной частоты
	tgt_sr_hz = (base_fs_hz * up_down.first) / up_down.second;

	//Нет необходимости переинициализировать
	if (!need_reset_ && up_down.first == up_factor_ && up_down.second == down_factor_) 
		return true;
    up_factor_ = up_down.first;
    down_factor_ = up_down.second;

	const double bw_ratio = double(bw_hz) / tgt_sr_hz;
	double fir_coeff_after_interp = bw_ratio / down_factor_; 	//Делим на down_factor_ т.к. FIR после интерполяции. 
    // Проектирование фильтра с правильной частотой среза
    const double cutoff_after_interp = std::min(0.5 * fir_coeff_after_interp,  0.5 / std::max(up_factor_, down_factor_));

	int fir_len = GetFirLenPow2(1. / down_factor_, std::max(0.95, bw_ratio));
	fir_len = std::min(std::max(16, fir_len), 1024);
	
    std::vector<Ipp32fc> taps;
    if(!GenerateWindowKoeffs(cutoff_after_interp, fir_len, ippWinBlackman, taps))
        return false;
    if(settings_.need_norm_power)
    {
        const Ipp32fc norm_koeff = {powf(up_factor_ * down_factor_, 0.5), 0};
        ippsMulC_32fc_I(norm_koeff , taps.data(), taps.size());
    }
    // Получение размеров буферов
    int specSize = 0, bufSize = 0;
    IppStatus status = ippsFIRMRGetSize(
		fir_len,
        up_factor_, 
        down_factor_, 
        ipp32fc, 
        &specSize, 
        &bufSize
    );
    if (status != ippStsNoErr) 
        return false;

    // Выделение памяти с выравниванием (critical для IPP!)
    spec_buffer_.resize(specSize/sizeof(Ipp8u) + 64);
    work_buffer_.resize(bufSize + 64);
    pSpec_ = (IppsFIRSpec_32fc*)spec_buffer_.data();
    // Инициализация спецификации с правильными фазами
    status = ippsFIRMRInit_32fc(
        taps.data(),                  // коэффициенты фильтра
		fir_len,     // длина фильтра
        up_factor_,                  0,  // upPhase
        down_factor_,                0,               // downPhase 
        pSpec_// выравнивание 
        
    );
    // Инициализация линии задержки
    delay_line_.resize(fir_len + up_factor_ - 1, {0,0});
	need_reset_ = false;
    return (status == ippStsNoErr);
}

bool MultiRateResampler::ProcessData(const Ipp32fc* passed_data, const size_t passed_size, std::vector<Ipp32fc>& res_vec) 
{
    if (!passed_data || passed_size < down_factor_) 
        return false;

	if (up_factor_ == down_factor_ && down_factor_ == 1) {
		res_vec.resize(passed_size);
		ippsCopy_32fc(passed_data, res_vec.data(), passed_size);
		return true;
	}
	if (!pSpec_)
		return false;
    const int numIters = static_cast<int>(passed_size) / down_factor_;
    const int outSize = numIters * up_factor_;
    res_vec.resize(outSize);

    IppStatus status = ippsFIRMR_32fc(
        passed_data,                        // входные данные
        res_vec.data(),              // выходной буфер
        numIters,                    // количество входных блоков
        pSpec_, // спецификация
        delay_line_.data(),          // исходное состояние
        delay_line_.data(),          // обновлённое состояние
        work_buffer_.data()          // временный буфер
    );
    
    return (status == ippStsNoErr);
}

bool aqua_resampler::MultiRateResampler::ProcessData(const std::vector<Ipp32fc>& passed_data, std::vector<Ipp32fc>& res_data)
{
	return ProcessData(passed_data.data(), passed_data.size(), res_data);
}

void MultiRateResampler::Clear() 
{
    pSpec_ = nullptr;

    spec_buffer_.clear();
    spec_buffer_.shrink_to_fit();

    work_buffer_.clear();
    work_buffer_.shrink_to_fit();

    delay_line_.clear();
    delay_line_.shrink_to_fit();


    up_factor_ = down_factor_ = 0;
}