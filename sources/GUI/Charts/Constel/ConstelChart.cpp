#include "ConstelChart.h"

using namespace aqua_gui;
using namespace constel;
ChartConstel::ChartConstel(QWidget * parrent): 
	QWidget(parrent), bg_image_(scale_info_)
{
	bg_image_.InitImage(":/AquaWave/sources/GUI/External/background/sym_sky.jpg");
	setMaximumWidth(257 * 5);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	scale_info_.pix_info_.margin_px = { 0,0 };

	scale_info_.val_info_.min_max_bounds.hor = { 0,1 };
	scale_info_.val_info_.min_max_bounds.vert = { 0,1 };
	
	scale_info_.val_info_.view_bounds.hor = { 0,1 };
	scale_info_.val_info_.view_bounds.vert = { 0,1 };

	connect(&redraw_timer_, &QTimer::timeout, this, QOverload<>::of(&ChartConstel::update));
	{
		double fps = 20;
		redraw_timer_.start(1000 / fps);
		core_.InitDecay(fps, 2);
	}
}

ChartConstel::~ChartConstel()
{
}


void ChartConstel::PushData(std::vector<Ipp32fc> & draw_data)
{
	core_.AddData(draw_data);
}

void ChartConstel::ClearData()
{
	core_.Emplace();
}

void ChartConstel::paintEvent(QPaintEvent* paint_event)
{
    QPainter new_frame_painter(this);
    new_frame_painter.setRenderHint(QPainter::Antialiasing, true);

    // Вычисляем смещение и размер квадрата
    auto chart_size = scale_info_.pix_info_.chart_size_px;
    int offset_x = (this->width() - chart_size.hor) / 2;
    int offset_y = (this->height() - chart_size.vert) / 2;

    // Сохраняем состояние и применяем трансформацию
    new_frame_painter.save();
    new_frame_painter.translate(offset_x, offset_y);

    // Закруглённые края для квадратной области
    {
        QPainterPath path;
        path.addRoundedRect(0, 0, chart_size.hor, chart_size.vert, 10, 10);
        new_frame_painter.setClipPath(path);
    }
	if (!CheckChartValidation(new_frame_painter))
		return;



    // Рисуем фон (относительно смещённой системы координат)
    bg_image_.DrawImage(new_frame_painter);

    {
        auto data_pixmap = core_.GetRelevantPixmap(std::min(chart_size.hor, chart_size.vert));
        new_frame_painter.drawPixmap(0, 0, data_pixmap);
    }

    new_frame_painter.restore();
}

void ChartConstel::resizeEvent(QResizeEvent* event)
{
    aqua_gui::HV_Info<int> cur_size = { this->width(), this->height() };
    auto& pix_info = scale_info_.pix_info_;

    if (pix_info.widget_size_px == cur_size) return;

    // Сохраняем реальный размер виджета
    pix_info.widget_size_px = cur_size;

    // chart_size_px — квадрат минимального размера
    auto min_size = std::min(cur_size.hor, cur_size.vert);
    pix_info.chart_size_px = HV_Info<int>({ min_size, min_size }) - pix_info.margin_px;
}

bool constel::ChartConstel::CheckChartValidation(QPainter& painter)
{
    auto& view_bounds = scale_info_.val_info_.view_bounds;
    auto& constel_data = core_.GetConstelData();

    bool val_bounds_valid = (view_bounds.hor.delta() > 1.e-6) && (view_bounds.vert.delta() > 1.e-6) &&
        std::isfinite(view_bounds.hor.delta()) && std::isfinite(view_bounds.vert.delta());
    bool is_core_valid = constel_data.side_size > 0 && constel_data.side_amplitude > 0;
    bool is_data_valid = constel_data.count_of_points > 0;

    if (!val_bounds_valid || !is_core_valid || !is_data_valid) {
        QPixmap pix(":/AquaWave/sources/GUI/External/BaseImages/month_sky.png");

        auto chart_size = scale_info_.pix_info_.chart_size_px;

        // Масштабируем относительно размера области чарта
        QSize scaledSize = pix.size().scaled(chart_size.hor, chart_size.vert, Qt::KeepAspectRatio);

        // Центрируем внутри локальной системы координат (уже смещённой через translate)
        QPoint pos((chart_size.hor - scaledSize.width()) / 2,
            (chart_size.vert - scaledSize.height()) / 2);

        painter.drawPixmap(pos.x(), pos.y(),
            scaledSize.width(), scaledSize.height(),
            pix);
        return false;
    }
    return true;
}

bool ChartConstel::ShouldRedraw()
{
	return true;
}
