#pragma once

#include <QtWidgets/QMainWindow>
#include <qgraphicsview.h>
#include <ipps.h>
typedef float data_type;
//Размеры нашей линейки
const int lineage_pix_height = 30;
const int lineage_pix_width  = 50;

std::vector<float> GetFloatRandomVec (const size_t size, const float min_value = -1.f,const float max_value = 1.f );
std::vector<float> GetFloatHamonics(const size_t size, const float amplitude, std::vector<float> signal_freqs);
class ChartMan : public QGraphicsView
{
public:
    ChartMan(QGraphicsView *parrent = nullptr);
    ~ChartMan();
    void                    SetArray(std::vector<Ipp32fc>   data_vec, std::pair<int64_t, int64_t> x_borders = {0,0} );
    void                    SetArray(std::vector<data_type> data_vec, std::pair<int64_t, int64_t> x_borders = {0,0} );
    std::vector<data_type>  GetArray();
    std::vector<Ipp32fc>    GetComplexArray();
    void                    DrawData();
private:
    std::vector<data_type>          data_vec_;
    std::vector<Ipp32fc>            complex_vec_;
    std::pair<int64_t, int64_t>     x_borders_;
};


