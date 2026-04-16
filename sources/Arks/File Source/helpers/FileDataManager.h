#include <future>       // Для std::future и асинхронных операций
#include <functional>   // Для std::bind и функторов
#include <thread>       // Для работы с потоками
#include <tbb\spin_mutex.h>
#include <unordered_map> // Для хранения слушателей

#include "DSP Tools/Resampler/ResamplerMan.h" // Менеджер ресемплера
#include "FileReadCore.h"
#include "ark_interface.h"
#include "special_defs\file_souce_defs.h"
#include "ark_defs.h"
using namespace fluctus;

namespace file_source
{
    class FileDataListener
    {
    public:
        // Состояния слушателя (битовая маска)
        enum ListenerState
        {
            kNoProcess      = 0,          // Без обработки
            kProcessStopped = 1 << 0,     // Процесс остановлен
            kReadAround     = 1 << 1,     // Режим чтения вокруг позиции 
            kReadCyclic     = 1 << 2,     // Циклическое чтение 
			kSingleReadInRange = 1 << 3,  // Чтение блоков в указанном диапазоне
			kLoopReadInRange   = 1 << 4,  // Циклическое чтение блоков в указанном диапазоне
			kNeedStop		   = 1 << 20,  //Запрос на остановку чтения
        };

    public:
        // Конструктор: принимает weak_ptr на ARK и параметры файла
        FileDataListener(const fluctus::ArkWptr& weak_ptr, const fluctus::SourceDescription& params);
        
        // Установка базовых параметров (частота, размер блока и т. д.)
        void SetBaseParams(InitParams &setup);
		void SetChunkSize(const int chunk_size);
        // Остановка процесса чтения
        void WaitProcess();
		void WaitProcessLocked();
        
        // Асинхронный запуск чтения вокруг позиции (pos_ratio — относительная позиция в файле)
        bool StartSingleAround(double pos_ratio);
		// Асинхронный запуск чтения чанками в указанных пределах (start_pos, end_pos - относительная позиция в файле)
		bool StartSingleInRange(const Limits<double>& time_bounds);

		// Асинхронный запуск циклического чтения чанками в указанных пределах (start_pos, end_pos - относительная позиция в файле)
		bool StartLoopInRange(const Limits<double>& time_bounds);

        // Получение текущего состояния слушателя
        const ListenerState GetState() const;

    private:
		/*
		* Чтение данных в 
		* - pos_ratio — относительная позиция (0.0–1.0)
		* - out_data_size — требуемый размер данных после ресемплера
		*/
		void ReadChunksInRangeProcess(const Limits<double>& time_bounds);

		/*
		* Чтение данных в
		* - pos_ratio — относительная позиция (0.0–1.0)
		* - out_data_size — требуемый размер данных после ресемплера
		*/
		void LoopReadInRangeProcess(const Limits<double>& time_bounds);
		
        /*
         * Чтение данных вокруг указанной позиции:
         * - pos_ratio — относительная позиция (0.0–1.0)
         * - out_data_size — требуемый размер данных после ресемплера
         */
        void ReadAroundProcess(double pos_ratio, int64_t out_data_size);
		void LoopReadPointsProcess();
        // Отправка подготовленных данных (вероятно, в ARK)
        void SendPreparedData();

    private:
        std::future<void> process_anchor_;   // "Якорь" — управляет асинхронной задачей
        fluctus::DataInfo data_info_;        // Информация о данных (переиспользуется для оптимизации)
        ListenerState state_{kNoProcess};    // Текущее состояние слушателя
        aqua_resampler::ResamplerManager resampler_; // Ресемплер
        const fluctus::ArkWptr target_ark_;       // Ссылка на weak_ptr целевого ARK
        int64_t block_size_;                 // Размер блока данных
        const SourceDescription& file_params_;          // Параметры файла (константная ссылка)
		tbb::spin_mutex init_mutex_;
		FileReader		file_reader_;
    };

    // Менеджер слушателей файловых данных
    class FileDataManager
    {
    public:
        // Конструктор: принимает параметры файла
        FileDataManager(const fluctus::SourceDescription& params);
        
        // Инициализация слушателя для ARK (если его ещё нет)
        void InitReader(const fluctus::ArkWptr& reader, InitParams &setup);
		void UpdateChunkSize(const fluctus::ArkWptr& reader, const int chunk_size);
        
        // Запуск чтения вокруг позиции для указанного ARK
        void StartReading(const fluctus::ArkWptr& reader, Limits<double> time_bounds, const FileSrcDove::FileSrcDoveThought read_type);

        // Удаление слушателя для указанного ARK
        void DeleteReader(const fluctus::ArkWptr& reader);
		void StopAllReaders();
    private:
        std::unordered_map<std::weak_ptr<fluctus::ArkInterface>, FileDataListener, 
            fluctus::WeakPtrHash<fluctus::ArkInterface>, fluctus::WeakPtrEqual<fluctus::ArkInterface>> listeners_; //Это инструкция для обращения с хэшом weak pointer
        const SourceDescription& params_; // Параметры файла
    };
};