#include "ChartMan.h"
#include <random>
#include <time.h>
#include <qgraphicsitem.h>
ChartMan::ChartMan(QGraphicsView * parrent):
QGraphicsView(parrent)
{
    auto c1 = new QGraphicsScene(parrent->rect(),this); 
    c1->setSceneRect(parrent->rect()); 
    this->QGraphicsView::setScene(c1);
    c1->setBackgroundBrush(Qt::gray);
}

ChartMan::~ChartMan()
{
}
void ChartMan::SetArray(std::vector<Ipp32fc> data_vec, std::pair<int64_t, int64_t> x_borders)
{
    complex_vec_ = data_vec;
    int size = data_vec.size();
    int mid = size/2;
    data_vec_.resize(size);
 
    for(int i=0;i<size;i++)
    {
        int real_index = i;
        auto abs_value = sqrt (data_vec[real_index].re*data_vec[real_index].re + data_vec[real_index].im*data_vec[real_index].im);
        data_vec_[i] =  abs_value;


    }
    SetArray(data_vec_, x_borders);
}
void ChartMan::SetArray(std::vector<data_type> data_vec, std::pair<int64_t, int64_t> x_borders)
{
    data_vec_  = data_vec;

    if(!x_borders.first && !x_borders.second)
        x_borders_ = {0,data_vec_.size()};
    else
        x_borders_ = x_borders;
}



std::vector<data_type> ChartMan::GetArray()
{
    return data_vec_;
}

std::vector<Ipp32fc> ChartMan::GetComplexArray()
{
    return complex_vec_;
}

void ChartMan::DrawData()
{

        
    //Размеры в пикселях под наш график
    const int chart_pix_height = this->scene()->height() - lineage_pix_height;
    const int chart_pix_width  = this->scene()->width () - lineage_pix_width;

    //Чтобы корректно расстянуть график по оси X
    const double chart_x_scale = double(chart_pix_width)/(int(data_vec_.size()));
    //Коэффициент с учётом того, что у нас индексация с нуля
    const double chart_x_scale_adapted = double(chart_pix_width)/(int(data_vec_.size() - 1));

    //Находим минимальное и максимальное значение
    data_type min_arg = data_vec_.empty()?0:data_vec_.front(), max_arg = min_arg;
    data_type mini_arg = data_vec_.empty()?0:data_vec_.front(), maxi_arg = mini_arg;
    float elem_summ = 0;
    int64_t elem_num= 0;
    auto i_scale = std::max(1,int(1/chart_x_scale));
    for(auto &elem_it:data_vec_)
    {
        mini_arg = std::min (mini_arg , elem_summ);
        maxi_arg = std::max (maxi_arg , elem_summ);
        elem_summ+=elem_it;
        if(++elem_num == i_scale)
        {
            elem_summ /= i_scale;
            min_arg = std::min (min_arg , elem_summ);
            max_arg = std::max (max_arg , elem_summ);
            elem_summ = 0;
            elem_num=0;
        }
        
    }
    //Коэффициент скалирования, учитвая диапазон значений массива
    const double chart_y_scale =  double(chart_pix_height) / (max_arg - min_arg);

    //Очищаем сцену
    this->scene()->clear();
    
    //Отрисовывваем линеечку для отображания значений
    {
        QPen pen;
        pen.setWidth(1);
        //Вектор из значений пикселей и соответсвующих им значений
        auto GetStepParams = [](std::pair<float, float> min_max_val, std::pair<int, int> min_max_pix, int pix_diff_min  = 5, std::pair<int, int> pix_shift = {0,0})
        {
            bool is_inverted_pix = (min_max_pix.second - min_max_pix.first)<0;
            float delta_val = min_max_val.second - min_max_val.first  ; //Разница в значениях
            int pix_count =  std::abs(min_max_pix.second - min_max_pix.first);  //Разница в пикселях

            float marks_count        = float(pix_count)/pix_diff_min; //Максимальное количество отметок
            float values_in_mark_min = delta_val/marks_count;         //Количество значений на одну отметку

            float adapted_ext = (log10(values_in_mark_min)) + 1;
            //Т.к. отрицательный диапазон округляет не в ту сторону
            if(adapted_ext < 0)
                adapted_ext -=1;
            float adapted_step = pow(10, int(adapted_ext));
            if(!adapted_step) return std::vector<std::pair<int, float>>();
            //Получили подстроенный шаг
            if(adapted_step < values_in_mark_min) return std::vector<std::pair<int, float>>();
            if (adapted_step/2 >=values_in_mark_min) adapted_step/=2;
    
            //Получили стартовое значение, от которого будем откладывать отсчёты
            float start_shift = fmod(min_max_val.first,adapted_step);
            if(start_shift) start_shift = (!is_inverted_pix) * adapted_step - start_shift;
            float start_value = min_max_val.first + start_shift;
    
            std::vector<std::pair<int, float>> pix_val_vec;
            float x_scale_koeff = pix_count / (delta_val);
            auto value_iter = start_value;

            while(true)
            {
                if(is_inverted_pix)
                {
                    int pix_value = min_max_pix.first - (value_iter - min_max_val.first) * x_scale_koeff;
                    if(pix_value < (min_max_pix.second + pix_shift.second))
                        break;
                    if(pix_value <= (min_max_pix.first - pix_shift.first))
                        pix_val_vec.emplace_back(pix_value,value_iter);
                    value_iter += adapted_step;
                }
                else
                {
                    int pix_value = (value_iter - min_max_val.first) * x_scale_koeff + min_max_pix.first;
                    if(pix_value > (min_max_pix.second - pix_shift.second))
                        break;
                    if(pix_value >= (min_max_pix.first + pix_shift.first))
                        pix_val_vec.emplace_back(pix_value,value_iter);
                    value_iter += adapted_step;
                }
                
            }
            return pix_val_vec;
        };
        //Добавляем отметки
        {
            auto AddText = [&](const QString text, const int width_px, const int height_px)
            {
                QGraphicsTextItem *text_item = this->scene()->addText(text);
                text_item->setDefaultTextColor("green");
                text_item->setPos(width_px, height_px);

            };
            //Шкала X
            {
                //Получаем меточки
                std::vector<std::pair<int, float>> x_marks = GetStepParams(x_borders_, {0,chart_pix_width}, 25, {5,30});
                int cur_coord_y = chart_pix_height;
                bool is_y_shifted = true;
                for(auto x_mark_it: x_marks)
                {
                    const auto &x_coord = x_mark_it.first;
                    const auto &x_value = x_mark_it.second;
                    AddText(QString::number(x_value), x_coord - 5 , cur_coord_y + is_y_shifted * 12);
                    is_y_shifted = !is_y_shifted;
                    //Добавляем отметку
                    pen.setColor("white");
                    this->scene()->addLine
                    (
                        {
                            QPoint({x_coord,cur_coord_y    }),
                            QPoint({x_coord,cur_coord_y + 5    }),
                        }, 
                        pen
                    );
                    //Проводим линию
                    pen.setColor("green");
                    this->scene()->addLine
                    (
                        {
                            QPoint({x_coord,cur_coord_y    }),
                            QPoint({x_coord,0              }),
                        }, 
                        pen
                    );
                }
            }
            //Шкала Y
            {
                //Получаем меточки
                std::vector<std::pair<int, float>> x_marks = GetStepParams({min_arg, max_arg}, {chart_pix_height, 0}, 15, {5,0});
                int cur_coord_x = chart_pix_width;
                for(auto y_mark_it: x_marks)
                {
                    const auto &cur_coord_y = y_mark_it.first;
                    const auto &y_value = y_mark_it.second;
                    //Добавляем отметку
                    pen.setColor("white");
                    this->scene()->addLine
                    (
                        {
                            QPoint({cur_coord_x    ,cur_coord_y}),
                            QPoint({cur_coord_x + 5,cur_coord_y}),
                        }, 
                        pen
                    );
                    //Проводим линию
                    pen.setColor("green");
                    this->scene()->addLine
                    (
                        {
                            QPoint({cur_coord_x    ,cur_coord_y}),
                            QPoint({0              ,cur_coord_y}),
                        }, 
                        pen
                    );
                    AddText(QString::number(y_value), cur_coord_x-2 , cur_coord_y - 10);
                }
            }

            
        }
        //Проводим границу
        {
            pen.setColor("white");
            //Слева направо
            QPoint start_point = {0, chart_pix_height};
            QPoint end_point   = {chart_pix_width, chart_pix_height};
            this->scene()->addLine({start_point,end_point}, pen);
            //Сверху вниз
            start_point = {chart_pix_width, 0};
            this->scene()->addLine({start_point,end_point}, pen);
        }
    
    }
    //Отрисовываем все наши данные
    {   
        QPen pen;
        pen.setColor("blue");
        pen.setWidth(1);
        if(data_vec_.empty())
            return;
        auto GetDotPoint = [&](int elem_it)
        {
            QPoint ret_dot = 
            {
                int(elem_it*chart_x_scale_adapted),
                int(chart_pix_height - (data_vec_[elem_it] - min_arg)*chart_y_scale)
            };
            return ret_dot;
        };
        //Пробегаем по пикселям, отрисовывая значения
        QPoint prev_dot = GetDotPoint(0);
        int prev_elem_it = 0;
        for(int pix_it = 0; pix_it < chart_pix_width; pix_it +=1)
        {
            //Порядковый номер элемента
            int cur_elem_it = (pix_it / chart_x_scale) ; 

            double fetched_val = 0;
            
            for(int elem_delta  = cur_elem_it - prev_elem_it;elem_delta>=0;elem_delta--)
                fetched_val += (data_vec_[cur_elem_it - elem_delta]);

            QPoint cur_dot = 
            {
                int(cur_elem_it*chart_x_scale_adapted),
                int(chart_pix_height - (fetched_val/(cur_elem_it - prev_elem_it + 1) - min_arg)*chart_y_scale)
            };

            this->scene()->addLine({prev_dot,cur_dot}, pen);
            //Запоминаем последний элемент, чтобы продолжить рисовать дальше
            std::swap(prev_dot, cur_dot);
            std::swap(prev_elem_it,cur_elem_it);
        }
    };
}

std::vector<float> GetFloatRandomVec(const size_t size, const float min_value, const float max_value)
{
    srand(time(0));
    std::vector<float>  ret_vec;
    ret_vec.resize(size);
    {
        for(auto &vec_it: ret_vec)
        {
            double rand_value = double(rand())/RAND_MAX; //от 0 до 1
            rand_value = rand_value * (max_value - min_value)  + min_value; //приводим к нашему диапазону, учитывая сдвиг
            vec_it = rand_value;
        }
        return ret_vec;
    }
}
std::vector<float> GetFloatHamonics(const size_t size, const float amplitude, std::vector<float> signal_freqs)
{
    srand(time(0));
    std::vector<float>  ret_vec;
    ret_vec.resize(size);

    for( auto &it_signal:signal_freqs)
    {
        it_signal = (2*3.14 / size) * it_signal;
    }
    for(int i=0; i<size; i++)
    {
        for (auto it_sig:signal_freqs)
        {
            ret_vec[i] += sinf(it_sig*i);
        }
        ret_vec[i] = amplitude * ret_vec[i] / signal_freqs.size();
    }
    return ret_vec;

}