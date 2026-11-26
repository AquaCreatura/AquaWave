#pragma once

#include <ipps.h>

#include <vector>

// Fractionally-spaced adaptive equaliser for complex IQ symbols.
//
// It supports blind algorithms (CMA, MMA) that need no decisions, and
// decision-directed ones (LMS, NLMS). In AUTO (warm-up) mode it starts blind and
// switches to decision-directed once the constellation is locked, with hysteresis
// so it can fall back if lock is lost.
class EqaliserAqua
{
public:
	enum class BlindAlgorithm
	{
		CMA,
		MMA
	};

	enum class DdAlgorithm
	{
		LMS,
		NLMS
	};

	static constexpr double kBlindStepMin = 1e-4;
	static constexpr double kBlindStepMax = 5e-3;
	static constexpr double kDdStepMin = 1e-3;
	static constexpr double kDdStepMax = 1e-2;

	explicit EqaliserAqua(int tap_count = 7);

	// Filter length (tap count). Resizes the state and resets the equaliser.
	void SetTapCount(int tap_count);
	int  GetTapCount() const { return static_cast<int>(taps_.size()); }

	void Reset();
	bool IsValid() const;

	// Filters one input sample in place, storing the history for the tap update.
	void Process(Ipp32fc& sample);

	// Updates the taps from a constellation decision (the slicer output).
	void Update(const Ipp32fc& pivot);

	// Initialises the blind-algorithm parameters from the constellation points.
	void InitFromPivots(const std::vector<Ipp32fc>& pivots);

	void EnableDirectDecision(bool enabled);
	void SetBlindAlgorithm(BlindAlgorithm algorithm);
	void SetDdAlgorithm(DdAlgorithm algorithm);
	void SetBlindStep(double step); // 1e-4 ... 5e-3
	void SetDdStep(double step);    // 1e-3 ... 1e-2

	// AUTO (warm-up): run the blind algorithm first, then switch to the
	// decision-directed one once locked. The switch is driven by a smoothed decision
	// error with hysteresis, so it can also switch back (LMS -> blind).
	void SetAutoWarmUp(bool enabled);
	bool IsWarmUpActive() const { return auto_warmup_; }
	// Current warm-up state: true - decision-directed (LMS), false - blind.
	bool IsLocked() const { return warmup_dd_; }

private:
	void UpdateBlind();
	void UpdateDd(const Ipp32fc& pivot);
	void UpdateLockMetric(const Ipp32fc& pivot);

	void UpdateCma();
	void UpdateMma();
	void UpdateLms(const Ipp32fc& pivot);
	void UpdateNlms(const Ipp32fc& pivot);
	void UpdateTaps(const Ipp32fc& error, double mu);

private:
	std::vector<Ipp32fc> taps_;
	std::vector<Ipp32fc> history_;
	std::vector<Ipp32fc> update_;

	Ipp32fc last_output_{ 0.0f, 0.0f };

	BlindAlgorithm blind_algorithm_ = BlindAlgorithm::MMA;
	DdAlgorithm    dd_algorithm_ = DdAlgorithm::NLMS;

	double blind_step_ = 0.01;
	double dd_step_ = 0.01;
	double dd_nlms_step_ = 0.8;   // used only by NLMS

	double cma_modulus_ = 1.0;
	double mma_real_modulus_ = 1.0;
	double mma_imag_modulus_ = 1.0;

	// Reliability gate for the decision-directed update (squared distance). Used only
	// in AUTO: a decision with an error above the gate is treated as unreliable and
	// replaced by a blind MMA update (see UpdateDd). Scaled by d_min^2 (0.25).
	double dd_gate_ = 0.0;

	bool direct_decision_enabled_ = true;

	// Warm-up (AUTO) state: blind start, switch to decision-directed on lock.
	bool   auto_warmup_ = false;
	bool   warmup_dd_ = false;
	// Smoothed (EMA) decision error; deliberately high at start so warm-up begins
	// blind.
	double lock_metric_ = 1.0;
	double lock_alpha_ = 1.0 / 1024.0;
	// Hysteresis thresholds derived from the minimum constellation distance:
	//   lock_on_  - error below it switches to LMS;
	//   lock_off_ - error above it switches back to blind.
	double lock_on_ = 0.0;
	double lock_off_ = 0.0;
};
