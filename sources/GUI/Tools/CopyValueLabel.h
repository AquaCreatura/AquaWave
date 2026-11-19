#pragma once
#include <qlabel.h>
#include <qtimer.h>

/*
	QLabel для отображения значения, которое копируется в буфер обмена кликом.

	При наведении курсор меняется на указатель и текст подчёркивается (подсказка,
	что значение можно скопировать), а после копирования на короткое время
	показывается индикатор "скопировано".
*/
class CopyValueLabel : public QLabel
{
public:
	explicit CopyValueLabel(QWidget* parent = nullptr);

	// Значение, которое отображается и копируется. Пустое значение не копируется.
	void SetValue(const QString& value);
	QString GetValue() const { return value_; }

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	void UpdateAppearance();

	QString value_;
	QTimer  copied_timer_;
	bool    hovered_{ false };
	bool    just_copied_{ false };
};
