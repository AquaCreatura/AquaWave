#pragma once

#include <vector>

#include <ipps.h>

namespace aq_demod
{

	class TimePeakerDemod
	{
	public:
		explicit TimePeakerDemod(int upsample_koeff = 4);
		~TimePeakerDemod();

		bool Process(const std::vector<Ipp32fc>& passed, std::vector<Ipp32fc>& out_samples);
		void Reset();

	private:
		Ipp32fc Interpolate(const std::vector<Ipp32fc>& signal, double pos) const;
		Ipp32fc GetSample(const std::vector<Ipp32fc>& signal, int index) const;
		double GetError(const Ipp32fc& prev, const Ipp32fc& mid, const Ipp32fc& cur);
		void UpdateLoop(double error);
		void UpdateHistory(const std::vector<Ipp32fc>& signal);

	private:
		int upsample_koeff_;

		// Оценка периода символа в отсчётах.
		double w_;

		// Положение текущего symbol instant относительно блока.
		double pos_;

		// PI timing loop.
		double gain_mu_;
		double gain_omega_;

		// Не даём оценке SPS слишком далеко уйти.
		double omega_min_;
		double omega_max_;

		// Медленная оценка средней мощности сигнала.
		double power_;

		// Предыдущий symbol-timed sample.
		Ipp32fc prev_symb_;
		bool have_prev_symb_;

		// Последние 4 входных отсчёта нужны при переходе между блоками.
		std::vector<Ipp32fc> history_;
	};

} // namespace aq_demod