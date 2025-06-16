#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_AquaWave.h"
#include "Spectrum DPX/SpectrumDPX.h"
#include "File Source/FileSource.h"
class AquaWave : public QMainWindow
{
    Q_OBJECT

public:
    AquaWave(QWidget *parent = nullptr);
    ~AquaWave();

private:
    std::shared_ptr<dpx_core::SpectrumDPX>      spectrum_chart_;
    std::shared_ptr<file_source::FileSourceArk> file_src_;
    Ui::AquaWaveWindow              ui;
};
