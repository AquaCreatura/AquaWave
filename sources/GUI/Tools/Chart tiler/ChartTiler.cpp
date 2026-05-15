#include "ChartTiler.h"
#include "Units/TileDPX.h"
#include "Units/TileSPG.h"
#include <qdebug.h>
ChartTiler::ChartTiler(const ChartScaleInfo & scale_info) : scale_info_(scale_info)
{
	is_spg_ = (scale_info_.val_info_.domain_type == aqua_gui::ChartDomainType::kTimeFrequency);	
	for (int i = 0; i < count_of_tiles_; i++) {
		if (is_spg_)
			tiles_.push_back(std::make_unique<TileSPG>());
		else
			tiles_.push_back(std::make_unique<TileDPX>());
		tiles_.back()->SetImageSize({ 1024ui64 * 2, 256ui64 * 2 });
	}
	tile_id_ = 0;
}

void ChartTiler::UpdateTileBase()
{
	const auto &base_bounds = scale_info_.val_info_.min_max_bounds;
	if (tiles_[0]->GetValBounds() != base_bounds) {
		tiles_[0] = tiles_[0]->RecreateWithBounds(base_bounds);
	}
		
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
	auto view_bounds = scale_info_.val_info_.cur_bounds;
	const auto &base_bounds = scale_info_.val_info_.min_max_bounds;
	if (!is_spg_) view_bounds.vert = base_bounds.vert;
	//Факт того, что диапазон попадаетв тайл
	auto is_tile_appopiate = [&](TileInterface::uptr& possible_tile) {
		const auto& cur_tile_bounds = possible_tile->GetValBounds();
		bool is_out_of_cur_tile = (view_bounds.hor.low < cur_tile_bounds.hor.low) || (view_bounds.hor.high > cur_tile_bounds.hor.high) || 
										(view_bounds.vert.low < cur_tile_bounds.vert.low) || (view_bounds.vert.high > cur_tile_bounds.vert.high);
		//Если в результате зумминга мы всё ещё находимся в пределах текущего тайла
		if (!is_out_of_cur_tile) {
			HV_Info<double> view_ratio = { cur_tile_bounds.hor.delta() / view_bounds.hor.delta(),
				cur_tile_bounds.vert.delta() / view_bounds.vert.delta() };
			if (view_ratio.hor < zoom_step_sqrt_*zoom_step_sqrt_ &&  view_ratio.vert < zoom_step_sqrt_*zoom_step_sqrt_) {
				return true;
			}
		}
		return false;
	};
	if (is_tile_appopiate(tiles_[tile_id_])) //Если текущий тайл подходит - выходим! (лучший вариант)
		return;

	//Ищем тайл среди имеющихся, в который попадает запрашиваемый дипазон
	int new_tile_id = -1;
	for (int tile_counter = 0; tile_counter < count_of_tiles_; tile_counter++) {
		//Не, ну а смысл проверять текущий тайл второй раз?)
		if ((tile_counter != tile_id_) && is_tile_appopiate(tiles_[tile_counter])) {
			new_tile_id = tile_counter;
			break;
		}
	}

	//Если нет подходящего тайла - создаём его...
	if (new_tile_id == -1) {
		auto calculate_new_bounds = [&](const Limits<double>& passed, const Limits<double>& base) -> Limits<double> {
			const double supposed_side_delta = passed.delta() * (zoom_step_sqrt_) / 2; //На сколько должно увеличиться с каждой стороны
			Limits<double> new_bounds = { std::max((passed.mid() - supposed_side_delta), base.low),
				std::min((passed.mid() + supposed_side_delta), base.high) };
			return new_bounds;
		};
		//Определяем границы нового тайла
		HorVerLim<double> new_bounds = { calculate_new_bounds(view_bounds.hor, base_bounds.hor),
											calculate_new_bounds(view_bounds.vert, base_bounds.vert) };

		if (new_bounds == base_bounds) { // Нулевой тайл у нас под базу
			new_tile_id = 0;
		}
		else {
			new_tile_id = 1 + (tile_id_) % (count_of_tiles_ - 1); //Выделяем элемент не под базу
			tiles_[new_tile_id]->SetValBounds(new_bounds);
		}
	}
	auto &new_use_tile = tiles_[new_tile_id];
	for (int tile_counter = count_of_tiles_ - 1; tile_counter >= 0; tile_counter--) {
		if (tile_counter == new_tile_id) continue;
		new_use_tile->UpdateFromTile(tiles_[tile_counter].get());
	}
	qDebug() << "new id: " << new_tile_id;
	tile_id_ = new_tile_id;
	
}

const QPixmap & ChartTiler::UpdateQPixmap()
{
	tbb::spin_mutex::scoped_lock scoped_locker(bounds_mutex_);
	auto &used_tile = tiles_[tile_id_];
	if (need_update_qimage_) {
		tbb::spin_mutex::scoped_lock data_locker(data_mutex_);
		need_update_qimage_ = false;
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
			used_tile->UpdateQimage(dyn_qim_, scale_info_.power_bounds_);
			zoomer_.MarkForUpdate();
		}
	}
	return zoomer_.GetPrecisedPart(used_tile->GetValBounds(),	//Границы тайла
									scale_info_.val_info_.cur_bounds,				//Границы отображаемые
									scale_info_.pix_info_.chart_size_px);			//Размер графика

}

bool ChartTiler::NeedUpdateTile()
{
	auto &used_tile = tiles_[tile_id_];
	const auto &view_bounds = scale_info_.val_info_.cur_bounds;
	const auto &cur_tile_bounds = used_tile->GetValBounds();
	bool is_out_of_cur_tile = (view_bounds.hor.low < cur_tile_bounds.hor.low) || (view_bounds.hor.high > cur_tile_bounds.hor.high) ||
								(view_bounds.vert.low < cur_tile_bounds.vert.low) || (view_bounds.vert.high > cur_tile_bounds.vert.high);
	const bool need_update = (is_out_of_cur_tile);
	return need_update;
}



void ChartTiler::SetData(const draw_data & data)
{
	tbb::spin_mutex::scoped_lock scoped_locker(data_mutex_);
	for (auto &it : tiles_) {
		it->SetData(data);
	}
}

void ChartTiler::Reset()
{
	tbb::spin_mutex::scoped_lock scoped_locker(data_mutex_);
	for (auto &it : tiles_)
		it->Reset();
}

const QPixmap & ChartTiler::GetRelevantPixmap()
{
	//Обновляем при необходимости сами тайлы
	if(NeedUpdateTile() || (image_update_timer_.elapsed() > 500))
	{
		UpdateBounds();
	}
	return UpdateQPixmap();
		
}

void ChartTiler::UpdateBounds()
{
	tbb::spin_mutex::scoped_lock scoped_locker(bounds_mutex_);
	tbb::spin_mutex::scoped_lock data_locker(data_mutex_);
	UpdateTileBase();//Обновляем тайлы
	UpdateTileView();
	image_update_timer_.restart(); //Перезапускаем таймер 
	need_update_qimage_ = true;
}
