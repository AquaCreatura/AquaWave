#include <future>       // Для std::future и асинхронных операций
#include <functional>   // Для std::bind и функторов
#include <thread>       // Для работы с потоками
#include <tbb\spin_mutex.h>
#include <unordered_map> // Для хранения слушателей

#include "DSP Tools/Resampler/ResamplerMan.h" // Менеджер ресемплера
#include "ark_interface.h"
#include "special_defs\file_souce_defs.h"

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
			kReadChunksInRange = 1 << 3,  // Чтение блоков в указанном диапазоне
			kNeedStop		   = 1 << 4,  //Запрос на остановку чтения
        };

    public:
        // Конструктор: принимает weak_ptr на ARK и параметры файла
        FileDataListener(const fluctus::ArkWptr& weak_ptr, const file_params& params);
        
        // Установка базовых параметров (частота, размер блока и т. д.)
        void SetBaseParams(int64_t carrier, int64_t samplerate, int64_t block_size = (1 << 10));
        
        // Остановка процесса чтения
        void WaitProcess();
		void WaitProcessLocked();
        
        // Асинхронный запуск чтения вокруг позиции (pos_ratio — относительная позиция в файле)
        bool StartReadAround(double pos_ratio);
		// Асинхронный запуск чтения чанками в указанных пределах (start_pos, end_pos - относительная позиция в файле)
		bool StartReadChunksInRange(double start_pos, double end_pos);

        // Получение текущего состояния слушателя
        const ListenerState GetState() const;

    private:
		/*
		* Чтение данных в 
		* - pos_ratio — относительная позиция (0.0–1.0)
		* - out_data_size — требуемый размер данных после ресемплера
		*/
		void ReadChunksInRangeProcess(double start_pos, double end_pos);
        /*
         * Чтение данных вокруг указанной позиции:
         * - pos_ratio — относительная позиция (0.0–1.0)
         * - out_data_size — требуемый размер данных после ресемплера
         */
        void ReadAroundProcess(double pos_ratio, int64_t out_data_size);
        
        // Отправка подготовленных данных (вероятно, в ARK)
        void SendPreparedData();

    private:
        std::future<void> process_anchor_;   // "Якорь" — управляет асинхронной задачей
        fluctus::DataInfo data_info_;        // Информация о данных (переиспользуется для оптимизации)
        ListenerState state_{kNoProcess};    // Текущее состояние слушателя
        aqua_resampler::ResamplerManager resampler_; // Ресемплер
        const fluctus::ArkWptr target_ark_;       // Ссылка на weak_ptr целевого ARK
        int64_t block_size_;                 // Размер блока данных
        const file_params& params_;          // Параметры файла (константная ссылка)
		tbb::spin_mutex init_mutex_;
    };

    // Менеджер слушателей файловых данных
    class FileDataManager
    {
    public:
		enum SortOfReading : int
		{
			kDoNothing			= 0, //Заглушка
			kReadAround			= 1 << 0,
			kReadChunksInRange	= 1 << 1
		};
        // Конструктор: принимает параметры файла
        FileDataManager(const file_params& params);
        
        // Инициализация слушателя для ARK (если его ещё нет)
        void InitReader(
            const fluctus::ArkWptr& reader,
            int64_t carrier = 0,
            int64_t samplerate = 0,
            int64_t block_size = (1 << 10)
        );
        
        // Запуск чтения вокруг позиции для указанного ARK
        void StartReading(const fluctus::ArkWptr& reader, const double start_pos, const double end_pos, const SortOfReading read_type);

        // Удаление слушателя для указанного ARK
        void DeleteReader(const fluctus::ArkWptr& reader);
		void StopAllReaders();
    private:
        std::unordered_map<std::weak_ptr<fluctus::ArkInterface>, FileDataListener, 
            fluctus::WeakPtrHash<fluctus::ArkInterface>, fluctus::WeakPtrEqual<fluctus::ArkInterface>> listeners_; // Словарь слушателей
        const file_params& params_; // Параметры файла
    };
};