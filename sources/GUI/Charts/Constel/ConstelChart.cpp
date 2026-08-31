#include "ConstelChart.h"
using namespace aqua_gui;
using namespace constel;
ChartConstel::ChartConstel(QWidget * parrent): 
	QWidget(parrent), bg_image_(scale_info_)
{
	bg_image_.InitImage(":/AquaWave/third_party/background/sym_sky.jpg");
	setMaximumWidth(257 * 5);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	scale_info_.pix_info_.margin_px = { 0,0 };

	scale_info_.val_info_.min_max_bounds.hor = { 0,1 };
	scale_info_.val_info_.min_max_bounds.vert = { 0,1 };
	
	scale_info_.val_info_.view_bounds.hor = { 0,1 };
	scale_info_.val_info_.view_bounds.vert = { 0,1 };

	connect(&redraw_timer_, &QTimer::timeout, this, QOverload<>::of(&ChartConstel::update));
	redraw_timer_.start(100);
}

ChartConstel::~ChartConstel()
{
}


void ChartConstel::PushData(std::vector<Ipp32fc> & draw_data)
{
	/*
	01. Fs_in   = 1.0e6;                           // исходная частота дискретизации IQ
02. Rs      = 700e3;                            // известная символьная скорость
03. SPS     = 4;                                // делаем 4 отсчёта на символ
04. Fs_work = SPS * Rs;                         // рабочая ЧД = 2.8 MHz
05. iq      = DC_remove(iq);                    // убираем постоянную составляющую
06. iq      = AGC(iq);                           // нормализуем амплитуду сигнала
07. iq      = resample_polyphase(iq, Fs_in, Fs_work); // 1 MHz -> 2.8 MHz
08. rrc     = RRC_taps(Rs, Fs_work, 0.25);     // создаём RRC matched filter
09. iq      = FIR(iq, rrc);                    // выполняем matched filtering
10. f_off   = estimate_coarse_CFO(iq);         // грубо оцениваем частотную ошибку
11. iq      = mix(iq, -f_off, Fs_work);        // компенсируем найденный CFO
12. timing  = SymbolSynchronizer(SPS);         // создаём timing recovery
13. timing.TED = Gardner;                      // используем Gardner для оценки timing error
14. timing.interpolator = Polyphase;           // дробная интерполяция внутри символа
15. timing.omega = Fs_work / Rs;               // начальное число отсчётов на символ = 4
16. symbols = timing.process(iq);              // получаем синхронизированные символы
17. carrier = CostasLoop(QPSK);                // создаём PLL восстановления несущей
18. symbols = carrier.process(symbols);        // исправляем остаточную фазу и частоту
19. symbols = remove_phase_ambiguity(symbols); // устраняем ±90° неоднозначность QPSK
20. bits    = QPSK_slicer(symbols);            // принимаем решение и получаем 2 бита/символ
	
	*/
	core_.AddData(draw_data);
}

void ChartConstel::ClearData()
{
	core_.Emplace();
}

void ChartConstel::paintEvent(QPaintEvent * paint_event)
{
	QPainter new_frame_painter(this);
	bg_image_.DrawImage(new_frame_painter);
	{
		auto data_pixmap = core_.GetRelevantPixmap(scale_info_.pix_info_.chart_size_px.hor);
		new_frame_painter.drawPixmap(0,0, data_pixmap);
	}
	
	
	

}

void ChartConstel::resizeEvent(QResizeEvent * event)
{
	aqua_gui::HV_Info<int> cur_size = { this->width(), this->height() };
	auto        &pix_info = scale_info_.pix_info_;
	if (pix_info.widget_size_px == cur_size) return;
	pix_info.widget_size_px = cur_size;
	pix_info.chart_size_px = pix_info.widget_size_px - pix_info.margin_px;
	

	
	if (cur_size.vert < cur_size.hor) {
		setMinimumHeight(cur_size.hor);
	}
	else if (cur_size.hor < cur_size.vert) {
		setMinimumHeight(cur_size.hor);		
		const auto min_size = std::min(cur_size.hor, cur_size.vert);
		//resize(min_size, min_size);
	}
	
		


}

bool ChartConstel::ShouldRedraw()
{
	return true;
}
