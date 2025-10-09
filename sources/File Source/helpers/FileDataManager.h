#include <future>       // ��� std::future � ����������� ��������
#include <functional>   // ��� std::bind � ���������
#include <thread>       // ��� ������ � ��������

#include <unordered_map> // ��� �������� ����������

#include "DSP Tools/Resampler/ResamplerMan.h" // �������� ����������
#include "Interfaces/ark_interface.h"
#include "File Source/file_souce_defs.h"

namespace file_source
{
    class FileDataListener
    {
    public:
        // ��������� ��������� (������� �����)
        enum ListenerState
        {
            kNoProcess      = 0,          // ��� ���������
            kProcessStopped = 1 << 0,     // ������� ����������
            kReadAround     = 1 << 1,     // ����� ������ ������ ������� 
            kReadCyclic     = 1 << 2,     // ����������� ������ 
			kReadChunksInRange = 1 << 3,  // ������ ������ � ��������� ���������
        };

    public:
        // �����������: ��������� weak_ptr �� ARK � ��������� �����
        FileDataListener(const fluctus::ArkWptr& weak_ptr, const file_params& params);
        
        // ��������� ������� ���������� (�������, ������ ����� � �. �.)
        void SetBaseParams(int64_t carrier, int64_t samplerate, int64_t block_size = (1 << 10));
        
        // ��������� �������� ������
        void WaitProcess();
        
        // ����������� ������ ������ ������ ������� (pos_ratio � ������������� ������� � �����)
        bool StartReadAround(double pos_ratio);
		// ����������� ������ ������ ������� � ��������� �������� (start_pos, end_pos - ������������� ������� � �����)
		bool StartReadChunksInRange(double start_pos, double end_pos);

        // ��������� �������� ��������� ���������
        const ListenerState GetState() const;

    private:
		/*
		* ������ ������ � 
		* - pos_ratio � ������������� ������� (0.0�1.0)
		* - out_data_size � ��������� ������ ������ ����� ����������
		*/
		void ReadChunksInRangeProcess(double start_pos, double end_pos);
        /*
         * ������ ������ ������ ��������� �������:
         * - pos_ratio � ������������� ������� (0.0�1.0)
         * - out_data_size � ��������� ������ ������ ����� ����������
         */
        void ReadAroundProcess(double pos_ratio, int64_t out_data_size);
        
        // �������� �������������� ������ (��������, � ARK)
        void SendPreparedData();

    private:
        std::future<void> process_anchor_;   // "�����" � ��������� ����������� �������
        fluctus::DataInfo data_info_;        // ���������� � ������ (���������������� ��� �����������)
        ListenerState state_{kNoProcess};    // ������� ��������� ���������
        aqua_resampler::ResamplerManager resampler_; // ���������
        const fluctus::ArkWptr target_ark_;       // ������ �� weak_ptr �������� ARK
        int64_t block_size_;                 // ������ ����� ������
        const file_params& params_;          // ��������� ����� (����������� ������)
    };

    // �������� ���������� �������� ������
    class FileDataManager
    {
    public:
		enum SortOfReading : int
		{
			kDoNothing			= 0, //��������
			kReadAround			= 1 << 0,
			kReadChunksInRange	= 1 << 1
		};
        // �����������: ��������� ��������� �����
        FileDataManager(const file_params& params);
        
        // ������������� ��������� ��� ARK (���� ��� ��� ���)
        void InitReader(
            const fluctus::ArkWptr& reader,
            int64_t carrier = 0,
            int64_t samplerate = 0,
            int64_t block_size = (1 << 10)
        );
        
        // ������ ������ ������ ������� ��� ���������� ARK
        void StartReading(const fluctus::ArkWptr& reader, const double start_pos, const double end_pos, const SortOfReading read_type);

        // �������� ��������� ��� ���������� ARK
        void DeleteReader(const fluctus::ArkWptr& reader);

    private:
        std::unordered_map<std::weak_ptr<fluctus::ArkInterface>, FileDataListener, 
            fluctus::WeakPtrHash<fluctus::ArkInterface>, fluctus::WeakPtrEqual<fluctus::ArkInterface>> listeners_; // ������� ����������
        const file_params& params_; // ��������� ����� (����������� ������)
    };
};