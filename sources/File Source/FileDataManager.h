#include <future>       // ��� std::future � ����������� ��������
#include <functional>   // ��� std::bind � ���������
#include <thread>       // ��� ������ � ��������

#include <unordered_map> // ��� �������� ����������

#include "DSP Tools/Resampler/ResamplerMan.h" // �������� ����������
#include "file_souce_defs.h"                  // ��������� ��������� ���������
#include "../interfaces/ark_interface.h"      // ��������� ARK (��������, ���������� ��� ������)

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
            kReadAround     = 1 << 1,     // ����� ������ ������ ������� (���� �� ����������)
            kReadCyclic     = 1 << 2      // ����������� ������ (�� ��������� ������������� ������)
        };

    public:
        // �����������: ��������� weak_ptr �� ARK � ��������� �����
        FileDataListener(const fluctus::ArkWptr& weak_ptr, const file_params& params);
        
        // ��������� ������� ���������� (�������, ������ ����� � �. �.)
        void SetBaseParams(int64_t carrier, int64_t samplerate, int64_t block_size = (1 << 10));
        
        // ��������� �������� ������
        void StopProcess();
        
        // ����������� ������ ������ ������ ������� (pos_ratio � ������������� ������� � �����)
        bool StartReadAround(double pos_ratio);

        // ��������� �������� ��������� ���������
        const ListenerState GetState() const;

    private:
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
        void ReadAround(const fluctus::ArkWptr& reader, double pos_ratio);
        
        // �������� ��������� ��� ���������� ARK
        void DeleteReader(const fluctus::ArkWptr& reader);

    private:
        std::unordered_map<std::weak_ptr<fluctus::ArkInterface>, FileDataListener, 
            fluctus::WeakPtrHash<fluctus::ArkInterface>, fluctus::WeakPtrEqual<fluctus::ArkInterface>> listeners_; // ������� ����������
        const file_params& params_; // ��������� ����� (����������� ������)
    };
};