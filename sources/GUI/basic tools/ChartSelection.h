#pragma once
//#include <tbb/spin_mutex.h>
#include <qgraphicsview.h>
class ChartSelection
{
public: 
    std::pair<int64_t, int64_t> GetSelection  ()  const;
    void                        SetSelection  (const std::pair<int64_t, int64_t> passed_borders);
    void                        ClearSelection();

private:
    std::pair<int64_t, int64_t> borders_;
    //tbb::spin_mutex             select_mutex_;
};

