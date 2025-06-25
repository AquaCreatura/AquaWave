#pragma once
#include <string>
#include <ipps.h>
#include <fstream>
#include <vector>

#include "File Source/file_souce_defs.h"

namespace file_source {

/**
 * @brief ����� ��� ������ ������ �� ����� � ���������� ��������� ����� ������ IPP.
 * ��������� ������ ������ �� �������� ��� ������ �������� ����� (� ������������� �����������).
 */
class FileReader {
public:
    ~FileReader();

    /**
     * @brief ������������� ����� ��� ������.
     * @param params ��������� ������ (��� ������, ������� � �.�.).
     * @return true, ���� ���� ������� ������ � ��������� ���������.
     */
    bool SetFileParams(const file_source::file_params &params);

    /**
     * @brief �������� ������ ����� � �������.
     * @return ���������� ������� � �����.
     */
    size_t GetFileSize() const;

    /**
     * @brief ������ ������ �� �����, ������� � ���������� ������.
     * @param start_sample ��������� �����.
     * @param count_of_samples ���������� ������� ��� ������.
     * @param read_data ������, ���� ����� �������� ������.
     * @return true, ���� ������ ������� ��������� (��� �������� ���������, ���� ��������� ����� �����).
     */
    bool ReadDataFrom(const size_t start_sample, const size_t count_of_samples, std::vector<uint8_t>& read_data);

    /**
     * @brief ������ ������ ������ �������� ����� (� ������������� ����������� [0, 1]).
     * @param ratio_point ����� � ��������� [0, 1], ������ ������� ����� ��������� ������.
     * @param data_size ���������� ������� ��� ������.
     * @param read_data ������, ���� ����� �������� ������ (�������� ������, ���� ������� �� ������� �����).
     * @return true, ���� ������ ������� ��������� (��� ������ �� ������� �����).
     */
    bool GetDataAround(const double ratio_point, const size_t data_size, std::vector<uint8_t>& read_data);

protected:
    IppDataType   data_type_;          ///< ��� ������ (ipp8u, ipp16s, ipp32f � �.�.)
    size_t        file_size_samples_;  ///< ������ ����� � �������
    std::ifstream ifstream_;           ///< ����� ��� ������ �����

private:
    /**
     * @brief ���������, ����� �� ���� ��� ������ (������ � ����� �������������� ��� ������).
     * @return true, ���� ���� �����, ����� false.
     */
    bool IsFileReady() const;

    /**
     * @brief ��������� ���������� ��������� ����� � ���������� ������� ��� ������,
     * �����������, ��� ���� ��������� ������ ����� ��� ������������� ���������� ������.
     * @param ratio_point ������� ������������� �����.
     * @param requested_data_size ����������� ������ ������ (� �������).
     * @param out_actual_start_sample ���������� ��������� ����� ��� ������.
     * @param out_actual_count_samples ���������� ���������� ������� ��� ������.
     */
    void CalculateReadInfo(double ratio_point, size_t requested_data_size,
                           size_t& out_actual_start_sample,
                           size_t& out_actual_count_samples) const;
};

/**
 * @class StreamReader
 * @brief ����� ��� ���������� ������ ������ �� ����� � ������������ ������������� �� ���������� � ������������� �����������.
 *
 * ����������� �� FileReader � ������������� ��������� ��� ���������� ������ ������ ������.
 */
class StreamReader : public FileReader {
public:
    /**
     * @brief �������������� ��������� ������, ��������� ���������� ��������.
     *
     * @param start_sample ��������� �����, � �������� ���������� ������.
     * @param total_samples ����� ���������� ������� ��� ������.
     * @param block_size ������ ������ ����� ��� ������.
     * @return true ���� ������������� ������ �������, ����� false.
     */
    bool Init(const size_t start_sample, const size_t total_samples, const size_t block_size);

    /**
     * @brief �������������� ��������� ������, ��������� ������������� ����������.
     *
     * @param start_ratio ������������� ��������� ������� (0.0 � ������, 1.0 � �����).
     * @param end_ratio ������������� �������� �������.
     * @param block_size ������ ������ ����� ��� ������.
     * @return true ���� ������������� ������ �������, ����� false.
     */
    bool InitFloat(const double start_ratio, const double end_ratio, const size_t block_size);

    /**
     * @brief ��������� ��������� ���� ������ � �����.
     *
     * @param vec ������, � ������� ����� �������� ��������� �����.
     * @return true ���� ���� ��� ������� ��������, ����� false.
     */
    bool ReadStream(std::vector<uint8_t>& vec);

    /**
     * @brief ���������, �������� �� ���������� ������ �� ������.
     *
     * @return true ���� �������� ������ ��� ������, ����� false.
     */
    bool IsReadStreamAvailable() const;

private:
    bool    is_initialized_ {false};   ///< ����, ����������� �� �������� �������������.
    size_t  start_sample_;             ///< ��������� ����� ��� ������ � ������ ������.
    size_t  total_samples_;            ///< ����� ���������� ������� ��� ������ � ������ ������.
    size_t  block_size_;               ///< ������ ������ ����� ������ ��� ������.
    size_t  current_position_;         ///< ������� ������� � ������ ������ ������������ start_sample_.
};

} // namespace file_source