#include "Arks/Interfaces/special_defs/analyzer_defs.h"
#include <vector>
#include <map>
/*
	enum class analz_chart_type : int {
		undefined = 0,
		kAcf = 1, //АКФ

		kPhasorSpectrum = 2, //Спектр мгновенной частоты (Символьная для PSK)
		kEnvelopeSpectrum = 4, //Спектр огибающей (Символьная для AM)

		kPowerSpectrum = 8, //Спектр степени (Отстройка несущей)

		kBandwidth = 16, //Ширина полосы

		kConstellation = 32
	};

*/
using namespace analyzer;
class HarmonicEstimator {
public:
	void SetBandwidth(const double bw_hz);
	void SetBaseCarrier(const double fc_hz);

	void SetLocalisedPeaks(analz_chart_type type, std::vector<float> &peaks);
	void Estimate(); //main function, which updates return values
	double GetSymbolRateHz	() const { return symbol_rate_hz_; };
	double GetAcfMsec		() const { return acf_msec_; };
	double GetCarrierHz		() const { return bw_hz_; };
private:
	void EstimateSymbolRate(aqua_opt<double>& matched_bd_hz, aqua_opt<double>& best_psk_hz, aqua_opt<double>& best_env_hz);
	void EstimateCarrier(double &mid_fc_hz);
	void EstimateAcf(double& acf_msec);
	static bool IsHarmonicOf(size_t p, size_t f, double tolerance = 0.01, int max_harmonic = 8);
protected:
	std::map<analz_chart_type, std::vector<float>> storage_;
	double symbol_rate_hz_;
	double carrier_hz_;
	double acf_msec_;
	double bw_hz_;
	double data_carrier_hz_ = 0.;
};