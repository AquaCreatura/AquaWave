#pragma once
#include "ui_selection_writer_window.h"
#include <qdialog.h>


class SelectionWriterWindow: public QDialog
{
	Q_OBJECT
public:
	SelectionWriterWindow();
	
signals:
protected:

	Ui::SelectionWriterWindow ui_;
};

