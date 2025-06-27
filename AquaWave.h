#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_AquaWave.h"
#include "DPX Spectrum/SpectrumDPX.h"
#include "Spectrogram/Spectrogram.h"
#include "File Source/FileSource.h"
class AquaWave : public QMainWindow
{
    Q_OBJECT

public:
    AquaWave(QWidget *parent = nullptr, const QString& file_path = QString() );
    ~AquaWave();

private:
    std::shared_ptr<dpx_core::SpectrumDPX>      spectrum_chart_;
    std::shared_ptr<file_source::FileSourceArk> file_src_;
    std::shared_ptr<spg_core::Spectrogram>      spectrogram_;
    Ui::AquaWaveWindow              ui;
};
