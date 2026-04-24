#include "ChartTiler.h"

void ChartTiler::InitBaseBounds(const HorVerLim<double> passed_bounds)
{
	base_bounds_ = passed_bounds;
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
void ChartTiler::SetViewBounds(const HorVerLim<double> view_bounds)
{
	auto used_tile = need_use_base_tile_ ? base_tile_ : cur_tile_;
	HorVerLim<double> cur_tile_bounds = used_tile.GetValBounds();
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
	HorVerLim<double> new_bounds = { calculate_new_bounds(view_bounds.hor, base_bounds_.hor),
										calculate_new_bounds(view_bounds.vert, base_bounds_.vert) };

	//Если отзумились к базовым границам
	if (new_bounds == base_bounds_) {
		need_use_base_tile_ = true;
		return;
	}



	//Определяем тайл
	buff_tile_.SetValBounds(new_bounds);
	buff_tile_.Reset();

	//Докидываем информацию из наших тайлов
	if(!need_use_base_tile_)
		buff_tile_.UpdateDataFromDataUnit(cur_tile_.GetDataUnit()); 
	buff_tile_.UpdateDataFromDataUnit(base_tile_.GetDataUnit());

	std::swap(buff_tile_, cur_tile_);
	need_use_base_tile_ = false;


}

void ChartTiler::SetData(const draw_data & data)
{
	/*for (auto tile_iter : tiles_) {
		tile_iter.
	}*/
}
