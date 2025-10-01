#include "FileReadCore.h"
#include <algorithm>
#include <climits>
#include <iostream> // For potential logging/debugging

namespace file_source {

FileReader::~FileReader() {
    if (ifstream_.is_open()) {
        ifstream_.close(); // Закрыть файл при уничтожении объекта
    }
}

bool FileReader::SetFileParams(const file_source::file_params &params) {
    // Закрываем любой ранее открытый файл
    if (ifstream_.is_open()) {
        ifstream_.close();
    }

    ifstream_.open(params.file_name_.toLocal8Bit().constData(), std::ios::binary | std::ios::ate);
    if (!ifstream_.is_open()) {
        std::cerr << "Error: Could not open file " << params.file_name_.toLocal8Bit().constData() << std::endl;
        return false; // Ошибка открытия файла
    }

    data_type_ = params.data_type_;

    size_t sample_size = GetSampleSize(data_type_);
    if (sample_size == 0) {
        std::cerr << "Error: Unsupported data type." << std::endl;
        ifstream_.close();
        return false; // Неподдерживаемый тип данных
    }

    std::streampos file_size_bytes = ifstream_.tellg();
    ifstream_.seekg(0, std::ios::beg); // Вернуться в начало файла

    // Проверяем, чтобы размер файла был кратен размеру сэмпла
    if (file_size_bytes % sample_size != 0) {
        std::cerr << "Warning: File size (" << file_size_bytes << " bytes) is not a multiple of sample size (" << sample_size << " bytes). Truncating." << std::endl;
    }
    file_size_samples_ = static_cast<size_t>(file_size_bytes) / sample_size;

    return true;
}

size_t FileReader::GetFileSize() const {
    return file_size_samples_;
}

bool FileReader::ReadDataFrom(const size_t start_sample, const size_t count_of_samples, std::vector<uint8_t>& read_data) {
    read_data.clear();

    if (!ifstream_.is_open()) {
        std::cerr << "Error: File is not open for reading." << std::endl;
        return false;
    }

    size_t sample_size = GetSampleSize(data_type_);
    if (sample_size == 0) {
        std::cerr << "Error: Unsupported data type for reading." << std::endl;
        return false;
    }

    if (count_of_samples == 0) {
        return true; // Запрошено 0 сэмплов, считаем успешным
    }

    // Вычисляем фактическое количество сэмплов, которое можно прочитать
    size_t actual_read_samples = 0;
    if (start_sample >= file_size_samples_) {
        std::cerr << "Warning: Start sample " << start_sample << " is beyond file size " << file_size_samples_ << "." << std::endl;
        return false; // Начальный сэмпл за пределами файла
    } else {
        size_t available_samples = file_size_samples_ - start_sample;
        actual_read_samples = std::min(count_of_samples, available_samples);
    }

    if (actual_read_samples == 0) {
        return true; // Ничего не нужно читать, но это не ошибка
    }

    size_t bytes_to_read = actual_read_samples * sample_size;
    read_data.resize(bytes_to_read);

    ifstream_.seekg(start_sample * sample_size, std::ios::beg);
    if (ifstream_.fail()) {
        std::cerr << "Error: Failed to seek to position " << start_sample * sample_size << " bytes." << std::endl;
        read_data.clear();
        return false; // Ошибка позиционирования
    }

    ifstream_.read(reinterpret_cast<char*>(read_data.data()), bytes_to_read);

    // Проверяем, сколько байт было фактически прочитано
    std::streamsize bytes_gcount = ifstream_.gcount();
    if (bytes_gcount != static_cast<std::streamsize>(bytes_to_read)) {
        std::cerr << "Warning: Expected to read " << bytes_to_read << " bytes, but read " << bytes_gcount << " bytes." << std::endl;
        size_t full_samples_read = static_cast<size_t>(bytes_gcount) / sample_size;
        read_data.resize(full_samples_read * sample_size); // Обрезать буфер, если прочитано меньше
        return false; // Частичное чтение или ошибка чтения
    }

    return true; // Прочитано успешно
}

bool FileReader::IsFileReady() const {
    size_t sample_size = GetSampleSize(data_type_);
    return (sample_size > 0 && ifstream_.is_open());
}

// Renamed and re-purposed for 'no padding' logic
void FileReader::CalculateReadInfo(double ratio_point, size_t requested_data_size,
                                   size_t& out_actual_start_sample,
                                   size_t& out_actual_count_samples) const {
    // Clamp ratio_point to [0, 1]
    double clamped_ratio = std::max(0.0, std::min(ratio_point, 1.0));

    // Calculate the desired center sample based on ratio_point
    int64_t desired_center_sample = static_cast<int64_t>(clamped_ratio * file_size_samples_);

    // Determine the ideal start sample for the requested block, aiming to center it
    int64_t ideal_start_sample = desired_center_sample - (static_cast<int64_t>(requested_data_size) / 2);

    // Adjust start_sample to be within file bounds.
    // This will effectively shift the block to the right if it's too far left,
    // or to the left if it's too far right.
    int64_t constrained_start_sample = std::max(ideal_start_sample, int64_t(0));

    // Calculate the maximum possible start_sample if the block is to fit entirely within the file
    if (requested_data_size > file_size_samples_) {
        // If requested size is larger than the file, we can only read the whole file
        out_actual_start_sample = 0;
        out_actual_count_samples = file_size_samples_;
        return;
    }

    int64_t max_possible_start_sample = static_cast<int64_t>(file_size_samples_ - requested_data_size);
    out_actual_start_sample = static_cast<size_t>(std::min(constrained_start_sample, max_possible_start_sample));

    // The actual count of samples will be exactly requested_data_size,
    // unless the file itself is smaller than requested_data_size.
    out_actual_count_samples = requested_data_size;
    if (out_actual_start_sample + out_actual_count_samples > file_size_samples_) {
        out_actual_count_samples = file_size_samples_ - out_actual_start_sample;
    }
}

bool FileReader::GetDataAround(const double ratio_point, const size_t data_size, std::vector<uint8_t>& read_data) {
    read_data.clear();

    if (data_size == 0) {
        return true; // Nothing to read, success.
    }

    if (!IsFileReady()) {
        std::cerr << "Error: File is not ready for GetDataAround." << std::endl;
        return false;
    }

    size_t sample_size = GetSampleSize(data_type_);

    size_t actual_start_sample;
    size_t actual_count_samples;

    // Calculate the actual start sample and count of samples to read,
    // ensuring no padding is needed.
    CalculateReadInfo(ratio_point, data_size,
                      actual_start_sample,
                      actual_count_samples);

    // If actual_count_samples is 0, it means we can't read anything
    // (e.g., requested_data_size was 0, or file is empty).
    if (actual_count_samples == 0) {
        return true; // Nothing to read, success.
    }

    // Read the data directly into read_data.
    // ReadDataFrom will resize read_data accordingly.
    bool read_ok = ReadDataFrom(actual_start_sample, actual_count_samples, read_data);

    // If read_ok is true, it means we successfully read exactly actual_count_samples.
    // The contract of this modified GetDataAround is to return the available block
    // without padding. So, success means we could read some data.
    return read_ok;
}

//=======================================  StreamReader ============================================

bool StreamReader::Init(const size_t start_sample, const size_t total_samples, const size_t block_size) {
    if (!ifstream_.is_open()) {
        std::cerr << "Error: File not open. Call FileReader::SetFileParams() first." << std::endl;
        return false;
    }

    const size_t file_size_samples = GetFileSize();

    if (block_size == 0) {
        std::cerr << "Error: Block size must be greater than 0." << std::endl;
        return false;
    }

    if (start_sample >= file_size_samples) {
        std::cerr << "Error: Start position " << start_sample << " is beyond file size " << file_size_samples << "." << std::endl;
        return false;
    }

    // Check if the requested total_samples extend beyond the file's end.
    // If so, truncate total_samples to fit within the file.
    size_t effective_total_samples = total_samples;
    if (start_sample + total_samples > file_size_samples) {
        effective_total_samples = file_size_samples - start_sample;
        std::cerr << "Warning: Requested total samples (" << total_samples << ") from start " << start_sample
                  << " exceed file size (" << file_size_samples << "). Truncating to " << effective_total_samples << " samples." << std::endl;
    }

    start_sample_     = start_sample;
    total_samples_    = effective_total_samples; // Use the potentially truncated value
    block_size_       = block_size;
    current_position_ = 0;
    is_initialized_   = true;

    return true;
}

bool StreamReader::InitFloat(const double start_ratio, const double end_ratio, const size_t block_size) {
    if (!ifstream_.is_open()) {
        std::cerr << "Error: File not open. Call FileReader::SetFileParams() first." << std::endl;
        return false;
    }

    if (start_ratio < 0.0 || start_ratio > 1.0 || end_ratio < 0.0 || end_ratio > 1.0 || start_ratio > end_ratio) {
        std::cerr << "Error: Invalid ratio values: start_ratio=" << start_ratio << ", end_ratio=" << end_ratio << std::endl;
        return false;
    }

    const size_t file_size_samples = GetFileSize();
    size_t start_pos = static_cast<size_t>(start_ratio * file_size_samples);
    size_t end_pos   = static_cast<size_t>(end_ratio * file_size_samples);

    // Adjust positions due to float precision or clamping
    if (start_pos > file_size_samples) start_pos = file_size_samples;
    if (end_pos > file_size_samples) end_pos = file_size_samples;
    if (end_pos < start_pos) end_pos = start_pos; // Ensure end_pos is not before start_pos

    // Call the main Init with calculated absolute positions
    return Init(start_pos, end_pos - start_pos, block_size);
}

bool StreamReader::ReadStream(std::vector<uint8_t>& vec) {
    vec.clear();

    if (!is_initialized_ || !IsReadStreamAvailable()) {
        return false;
    }

    const size_t remaining_samples = total_samples_ - current_position_;
    const size_t samples_to_read = std::min(block_size_, remaining_samples);

    if (samples_to_read == 0) {
        return false;
    }

    bool read_success = ReadDataFrom(start_sample_ + current_position_, samples_to_read, vec);

    const size_t sample_size = GetSampleSize(data_type_);
    if (sample_size == 0) {
        std::cerr << "Error: Unsupported data type for stream reading." << std::endl;
        is_initialized_ = false;
        return false;
    }

    const size_t actual_bytes_read = vec.size();
    const size_t actual_samples_read = actual_bytes_read / sample_size;
    current_position_ += actual_samples_read;

    return actual_samples_read > 0;
}

bool StreamReader::IsReadStreamAvailable() const {
    return is_initialized_ && (current_position_ < total_samples_);
}

} // namespace file_source