#pragma once
#include "ui_scope_analyzer_window.h"
#include <qdialog.h>



class ScopeAnalyzerWindow : public QDialog
{
Q_OBJECT
public:
	ScopeAnalyzerWindow();
    void SetChartWindow(QWidget* wigdet_ptr);

protected:
    Ui::ScopeAnalyzerWindow ui_;
};

