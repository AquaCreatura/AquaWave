#include "ChartTiler.h"

ChartTiler::ChartTiler(const ChartScaleInfo & scale_info) : scale_info_(scale_info)
{
}

void ChartTiler::UpdateTileBase()
{
	//Обновить данные базовый порог
}

/*
	1.1) Вышли за границу вверх:
			с определённым порогом (1.5) пересоздаём картинку выше.
			переносим наши данные в картинку выше - relevant - копируем как relevant,
			а всё, что осталось добиваем из base.
	1.2) Вышли за границу вниз:
			С определённым порогом (1.5) пересоздаём картинку ниже.
			переносим наши данные в картинку ниже - relevant копируем соотвествующим образом. Будет прореженная картинка, но потом дозапрашиваем данные
*/
void ChartTiler::UpdateTileView()
{
	const auto &view_bounds = scale_info_.val_info_.cur_bounds;
	const auto &base_bounds = scale_info_.val_info_.min_max_bounds;
	std::unique_ptr<TileInterface> &used_tile = need_use_base_tile_ ? base_tile_ : cur_tile_;
	const auto& cur_tile_bounds = used_tile->GetValBounds();
	const bool is_out_of_cur_tile = (view_bounds.hor.low < cur_tile_bounds.hor.low) || (view_bounds.hor.high > cur_tile_bounds.hor.high);

	//Если в результате зумминга мы всё ещё находимся в пределах текущего тайла
	if (!is_out_of_cur_tile) {
		HV_Info<double> view_ratio = { view_bounds.hor.delta() / cur_tile_bounds.hor.delta(),
										view_bounds.vert.delta() / cur_tile_bounds.vert.delta() };
		if (view_ratio.hor < zoom_step_sqrt_*zoom_step_sqrt_ && view_ratio.vert < zoom_step_sqrt_*zoom_step_sqrt_) {
			return;
		}
	} 

	
	auto calculate_new_bounds = [&](const Limits<double>& passed, const Limits<double>& base) -> Limits<double> {
		const double supposed_side_delta = passed.delta() * (zoom_step_sqrt_ - 1) / 2; //На сколько должно увеличиться с каждой стороны
		Limits<double> new_bounds = { std::max((passed.mid() - supposed_side_delta), base.low),
										std::min((passed.mid() + supposed_side_delta), base.high) };
		return new_bounds;
	};
	//Определяем границы нового тайла
	HorVerLim<double> new_bounds = { calculate_new_bounds(view_bounds.hor, base_bounds.hor),
										calculate_new_bounds(view_bounds.vert, base_bounds.vert) };

	//Если отзумились к базовым границам
	if (new_bounds == base_bounds) {
		need_use_base_tile_ = true;
		return;
	}



	//Определяем тайл
	buff_tile_->SetValBounds(new_bounds);
	buff_tile_->Reset();

	//Докидываем информацию из наших тайлов
	if(!need_use_base_tile_)
		buff_tile_->UpdateFromTile(cur_tile_); 
	buff_tile_->UpdateFromTile(base_tile_);

	std::swap(buff_tile_, cur_tile_);
	need_use_base_tile_ = false;


}

void ChartTiler::UpdateImageFromTile()
{
	auto &used_tile = need_use_base_tile_ ? base_tile_ : cur_tile_;
	//Приводим к размеру 1 к 1
	if (dyn_qim_.size != used_tile->GetImageSize())
	{
		auto need_size = used_tile->GetImageSize();
		dyn_qim_.data.resize(need_size.hor * need_size.vert);
		dyn_qim_.qimage = QImage((uint8_t*)dyn_qim_.data.data(), need_size.hor, need_size.vert, QImage::Format::Format_ARGB32);
		dyn_qim_.size = need_size;
		zoomer_.SetNewBase(&dyn_qim_.qimage);
	}
	if (used_tile->is_data_updated_) {
		used_tile->is_data_updated_ = false; //Лучше лишний раз отобразить, чем пропустить данные
		used_tile->UpdateQimage(dyn_qim_);
		zoomer_.MarkForUpdate();
	}
}

bool ChartTiler::NeedUpdateImage()
{
	std::unique_ptr<TileInterface> &used_tile = need_use_base_tile_ ? base_tile_ : cur_tile_;
	const auto &view_bounds = scale_info_.val_info_.cur_bounds;
	const auto &cur_tile_bounds = used_tile->GetValBounds();
	const bool is_out_of_cur_tile = (view_bounds.hor.low < cur_tile_bounds.hor.low) || (view_bounds.hor.high > cur_tile_bounds.hor.high);
	const bool need_update = (is_out_of_cur_tile);
	return need_update;
}

void ChartTiler::SetData(const draw_data & data)
{
	cur_tile_->SetData(data);
	base_tile_->SetData(data); //Только если вне границ у 'cur'
}

void ChartTiler::Reset()
{
	cur_tile_->Reset();
	base_tile_->Reset();
	buff_tile_->Reset();
}

const QPixmap & ChartTiler::GetRelevantPixmap()
{
	//Обновляем при необходимости сами тайлы
	if(NeedUpdateImage() || (image_update_timer_.elapsed() > 500))
	{
		UpdateTileBase();//Обновляем тайлы
		UpdateTileView();

		UpdateImageFromTile(); //Обновляем нашу текущую картинку

		image_update_timer_.restart(); //Перезапускаем таймер 
	}

	return zoomer_.GetPrecisedPart(	scale_info_.val_info_.min_max_bounds,	//Мин/Макс границы изображения 
									scale_info_.val_info_.cur_bounds,		//Границы отображаемые
									scale_info_.pix_info_.chart_size_px);	//Размер графика
		
}
