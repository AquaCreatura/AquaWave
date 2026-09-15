#pragma once

#include <QObject>
#include <QMainWindow>
#include <QWidget>
#include <QPoint>
#include <QRect>
#include <Qt>

class QHBoxLayout;
class QPushButton;
class QMouseEvent;
class QEvent;

class FramelessHelper : public QObject
{
	Q_OBJECT
public:
	explicit FramelessHelper(QMainWindow* window);

	void setTitleBar(QWidget* titleBar);
	void setResizeBorderWidth(int px);
	void setDragEnabled(bool enabled);
protected:
	bool eventFilter(QObject* obj, QEvent* ev) override;

private:
	// UI 
	void setupButtons();
	void applyButtonStyle();

	// Хелперы event'ов 
	bool handleTitleBarEvent(QEvent* ev);
	bool handleWindowEvent(QObject* obj, QEvent* ev);

	// Drag 
	bool handleDragPress(QMouseEvent* me);
	bool handleDragMove(QMouseEvent* me);
	void handleDragRelease();

	// Resize 
	bool handleResizePress(QObject* obj, QMouseEvent* me);
	bool handleResizeMove(QObject* obj, QMouseEvent* me);
	void handleResizeRelease();

	// Edges / курсор 
	QPoint          mapToWindow(QObject* obj, const QPoint& pos) const;
	Qt::Edges       edgesAt(const QPoint& pos) const;
	Qt::CursorShape cursorForEdges(Qt::Edges edges) const;
	void            updateCursor(Qt::Edges edges);
	void enableHoverTracking(QWidget* root);
private:
	QMainWindow* m_window = nullptr;
	QWidget*     m_titleBar = nullptr;

	QPushButton* m_minBtn = nullptr;
	QPushButton* m_maxBtn = nullptr;
	QPushButton* m_closeBtn = nullptr;

	bool m_dragEnabled = true;
	bool m_dragging = false;
	bool m_resizing = false;

	int       m_borderWidth = 6;
	QPoint    m_dragOrigin;
	QPoint    m_resizeOrigin;
	QRect     m_resizeGeometry;
	Qt::Edges m_resizeEdge;
};