#pragma once
#include <string>
#include <ipps.h>
#include <fstream>
#include <vector>

#include "File Source/file_souce_defs.h"

namespace file_source {

/**
 * @brief Класс для чтения данных из файла с поддержкой различных типов данных IPP.
 * Позволяет читать данные по смещению или вокруг заданной точки (в относительных координатах).
 */
class FileReader {
public:
    ~FileReader();

    /**
     * @brief Инициализация файла для чтения.
     * @param params Параметры чтения (тип данных, частота и т.д.).
     * @return true, если файл успешно открыт и параметры корректны.
     */
    bool SetFileParams(const file_source::file_params &params);

    /**
     * @brief Получить размер файла в сэмплах.
     * @return Количество сэмплов в файле.
     */
    size_t GetFileSize() const;

    /**
     * @brief Чтение данных из файла, начиная с указанного сэмпла.
     * @param start_sample Начальный сэмпл.
     * @param count_of_samples Количество сэмплов для чтения.
     * @param read_data Вектор, куда будут записаны данные.
     * @return true, если данные успешно прочитаны (или частично прочитаны, если достигнут конец файла).
     */
    bool ReadDataFrom(const size_t start_sample, const size_t count_of_samples, std::vector<uint8_t>& read_data);

    /**
     * @brief Чтение данных вокруг заданной точки (в относительных координатах [0, 1]).
     * @param ratio_point Точка в диапазоне [0, 1], вокруг которой нужно прочитать данные.
     * @param data_size Количество сэмплов для чтения.
     * @param read_data Вектор, куда будут записаны данные (дополнен нулями, если выходит за границы файла).
     * @return true, если данные успешно прочитаны (без выхода за границы файла).
     */
    bool GetDataAround(const double ratio_point, const size_t data_size, std::vector<uint8_t>& read_data);

protected:
    IppDataType   data_type_;          ///< Тип данных (ipp8u, ipp16s, ipp32f и т.д.)
    size_t        file_size_samples_;  ///< Размер файла в сэмплах
    std::ifstream ifstream_;           ///< Поток для чтения файла

private:
    /**
     * @brief Проверяет, готов ли файл для чтения (открыт и имеет поддерживаемый тип данных).
     * @return true, если файл готов, иначе false.
     */
    bool IsFileReady() const;

    /**
     * @brief Вычисляет актуальные начальный сэмпл и количество сэмплов для чтения,
     * обеспечивая, что блок находится внутри файла без необходимости дополнения нулями.
     * @param ratio_point Входная относительная точка.
     * @param requested_data_size Запрошенный размер данных (в сэмплах).
     * @param out_actual_start_sample Актуальный начальный сэмпл для чтения.
     * @param out_actual_count_samples Актуальное количество сэмплов для чтения.
     */
    void CalculateReadInfo(double ratio_point, size_t requested_data_size,
                           size_t& out_actual_start_sample,
                           size_t& out_actual_count_samples) const;
};

/**
 * @class StreamReader
 * @brief Класс для потокового чтения данных из файла с возможностью инициализации по абсолютным и относительным координатам.
 *
 * Наследуется от FileReader и предоставляет интерфейс для поэтапного чтения блоков данных.
 */
class StreamReader : public FileReader {
public:
    /**
     * @brief Инициализирует потоковое чтение, используя абсолютные значения.
     *
     * @param start_sample Начальный сэмпл, с которого начинается чтение.
     * @param total_samples Общее количество сэмплов для чтения.
     * @param block_size Размер одного блока для чтения.
     * @return true если инициализация прошла успешно, иначе false.
     */
    bool Init(const size_t start_sample, const size_t total_samples, const size_t block_size);

    /**
     * @brief Инициализирует потоковое чтение, используя относительные координаты.
     *
     * @param start_ratio Относительная начальная позиция (0.0 — начало, 1.0 — конец).
     * @param end_ratio Относительная конечная позиция.
     * @param block_size Размер одного блока для чтения.
     * @return true если инициализация прошла успешно, иначе false.
     */
    bool InitFloat(const double start_ratio, const double end_ratio, const size_t block_size);

    /**
     * @brief Считывает следующий блок данных в поток.
     *
     * @param vec Вектор, в который будут записаны считанные байты.
     * @return true если блок был успешно прочитан, иначе false.
     */
    bool ReadStream(std::vector<uint8_t>& vec);

    /**
     * @brief Проверяет, возможно ли дальнейшее чтение из потока.
     *
     * @return true если доступны данные для чтения, иначе false.
     */
    bool IsReadStreamAvailable() const;

private:
    bool    is_initialized_ {false};   ///< Флаг, указывающий на успешную инициализацию.
    size_t  start_sample_;             ///< Начальный сэмпл для чтения в рамках потока.
    size_t  total_samples_;            ///< Общее количество сэмплов для чтения в рамках потока.
    size_t  block_size_;               ///< Размер одного блока данных для чтения.
    size_t  current_position_;         ///< Текущая позиция в потоке чтения относительно start_sample_.
};

} // namespace file_source