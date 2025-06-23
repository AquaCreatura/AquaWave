#pragma once

#include <stdint.h>
#include <vector>
#include <atomic>
#include <tbb/spin_mutex.h>
#include "GUI/gui_defs.h"
using namespace fluctus;
using namespace aqua_gui;

namespace spg_core
{
    typedef uint8_t DataType;

    struct spg_holder
    {
        //�������� ������ ���������� ����� ����� �����:
        //����� ������� = �����, � ������ ���� - ������� (������� = val_bounds) (�.�. �������� ����� ������!)

        // ������ � ������� ������
        DataType* operator[](size_t row) { return &data[row * size.horizontal]; }
        const DataType* operator[](size_t row) const { return &data[row * size.horizontal]; }

        WH_Bounds<double>       val_bounds; // �������� �������� 
        WH_Info<size_t>         size;
        std::vector<bool>       relevant_vec; //������������ ����� ������ ��������
        std::vector<DataType>   data;         //���� ������
    };

    struct spg_data
    {
        spg_holder base_data; //���� ������ 
        int a;

     


    };


}