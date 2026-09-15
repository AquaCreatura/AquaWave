#include "FramelessHelper.h"

#include <QEvent>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMainWindow>

// ─── Конструктор ────────────────────────────────────────
FramelessHelper::FramelessHelper(QMainWindow* window)
	: QObject(window), m_window(window)
{
	m_window->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

	m_window->setAttribute(Qt::WA_TranslucentBackground, false);
	m_window->setMouseTracking(true);      // ← важно для hover-курсора
	m_window->installEventFilter(this);
}

void FramelessHelper::setTitleBar(QWidget* titleBar)
{
	m_titleBar = titleBar;
	m_titleBar->setMouseTracking(true);
	m_titleBar->setAttribute(Qt::WA_Hover, true); // Добавь это
	m_titleBar->installEventFilter(this);

	// Включаем слежение для всего окна и его детей
	enableHoverTracking(m_window); // Добавь это

	setupButtons();
}
void FramelessHelper::setResizeBorderWidth(int px) { m_borderWidth = px; }
void FramelessHelper::setDragEnabled(bool enabled) { m_dragEnabled = enabled; }

// ─── Кнопки ─────────────────────────────────────────────
void FramelessHelper::setupButtons()
{
	QHBoxLayout* lay = qobject_cast<QHBoxLayout*>(m_titleBar->layout());
	if (!lay) {
		lay = new QHBoxLayout(m_titleBar);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(0);
	}
	m_titleBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	lay->addStretch(2);

	auto makeBtn = [&](const QString& text) {
		auto* b = new QPushButton(text, m_titleBar);
		b->setFixedSize(46, 30);
		b->setCursor(Qt::ArrowCursor);
		b->setFocusPolicy(Qt::NoFocus);
		lay->addWidget(b);
		return b;
	};

	m_minBtn = makeBtn("--");
	m_maxBtn = makeBtn("O");
	m_closeBtn = makeBtn("X");

	applyButtonStyle();

	connect(m_minBtn, &QPushButton::clicked, m_window, &QMainWindow::showMinimized);

	connect(m_maxBtn, &QPushButton::clicked, this, [this]() {
		if (m_window->isMaximized()) {
			m_window->showNormal();
			m_maxBtn->setText("O");
		}
		else {
			m_window->showMaximized();
			m_maxBtn->setText("o");
		}
	});

	connect(m_closeBtn, &QPushButton::clicked, m_window, &QMainWindow::close);
}

// ─── Event Filter ───────────────────────────────────────
bool FramelessHelper::eventFilter(QObject* obj, QEvent* ev)
{
	const bool isWindow = (obj == m_window);
	const bool isTitleBar = (obj == m_titleBar);

	if (handleWindowEvent(obj, ev))
		return true;
	if (!isWindow && !isTitleBar) {
		// любой QWidget внутри нашего окна — обрабатываем как фон для курсора/resize
		auto* w = qobject_cast<QWidget*>(obj);
		if (!w || w->window() != m_window)
			return false;
	}



	if (isTitleBar && m_dragEnabled && handleTitleBarEvent(ev))
		return true;

	return false;
}

// ─── Title bar: drag + dblclick ─────────────────────────
bool FramelessHelper::handleTitleBarEvent(QEvent* ev)
{
	switch (ev->type())
	{
	case QEvent::MouseButtonPress:
		return handleDragPress(static_cast<QMouseEvent*>(ev));

	case QEvent::MouseMove:
		return handleDragMove(static_cast<QMouseEvent*>(ev));

	case QEvent::MouseButtonRelease:
		handleDragRelease();
		return false;

	case QEvent::MouseButtonDblClick: {
		auto* me = static_cast<QMouseEvent*>(ev);
		if (me->button() == Qt::LeftButton) {
			m_maxBtn->click();
			return true;
		}
		break;
	}
	default: break;
	}
	return false;
}

bool FramelessHelper::handleDragPress(QMouseEvent* me)
{
	if (me->button() != Qt::LeftButton) return false;
	m_dragging = true;
	m_dragOrigin = me->globalPos() - m_window->frameGeometry().topLeft();
	return true;
}

bool FramelessHelper::handleDragMove(QMouseEvent* me)
{
	if (!m_dragging) return false;

	if (m_window->isMaximized()) {
		const qreal ratio = qreal(me->pos().x()) / qreal(m_titleBar->width());
		m_window->showNormal();
		m_maxBtn->setText("O");
		m_dragOrigin = QPoint(int(m_window->width() * ratio), me->pos().y());
	}
	m_window->move(me->globalPos() - m_dragOrigin);
	return true;
}

void FramelessHelper::handleDragRelease()
{
	m_dragging = false;
}

// ─── Window: resize + cursor ────────────────────────────
bool FramelessHelper::handleWindowEvent(QObject* obj, QEvent* ev)
{
	switch (ev->type())
	{
	case QEvent::MouseButtonPress:
		return handleResizePress(obj, static_cast<QMouseEvent*>(ev));

	case QEvent::MouseMove:
		return handleResizeMove(obj, static_cast<QMouseEvent*>(ev));

	case QEvent::MouseButtonRelease:
		handleResizeRelease();
		return false;

	case QEvent::Leave:
	case QEvent::HoverLeave:
		if (!m_resizing && !m_dragging)
			m_window->unsetCursor();
		return false;
	case QEvent::HoverMove: // <-- ДОБАВЬ ЭТО
	{
		// Приводим к QHoverEvent, а не к QMouseEvent
		auto* he = static_cast<QHoverEvent*>(ev);
		if (!m_resizing && !m_dragging && !m_window->isMaximized()) {
			// Используем mapToWindow, который у тебя уже есть
			updateCursor(edgesAt(mapToWindow(obj, he->pos())));
		}
		return false; // Не перехватываем, пусть событие идёт дальше
	}

		// ❗ QEvent::Resize здесь НЕ обрабатываем — это не QMouseEvent.
	default: break;
	}
	return false;
}

bool FramelessHelper::handleResizePress(QObject* obj, QMouseEvent* me)
{
	if (me->button() != Qt::LeftButton) return false;
	if (m_window->isMaximized())        return false;

	const Qt::Edges edges = edgesAt(mapToWindow(obj, me->pos()));
	if (!edges) return false;

	m_resizing = true;
	m_resizeEdge = edges;
	m_resizeOrigin = me->globalPos();
	m_resizeGeometry = m_window->frameGeometry();

	m_window->setCursor(cursorForEdges(edges));   // зафиксировали курсор
	return true;
}

bool FramelessHelper::handleResizeMove(QObject* obj, QMouseEvent* me)
{
	// Ещё не ресайзим — просто обновляем курсор по краям
	if (!m_resizing) {
		if (m_dragging)             return false;   // во время drag курсор не трогаем
		if (m_window->isMaximized()) return false;
		updateCursor(edgesAt(mapToWindow(obj, me->pos())));
		return false;
	}

	const QPoint delta = me->globalPos() - m_resizeOrigin;
	QRect g = m_resizeGeometry;

	if (m_resizeEdge & Qt::LeftEdge)   g.setLeft(g.left() + delta.x());
	if (m_resizeEdge & Qt::RightEdge)  g.setRight(g.right() + delta.x());
	if (m_resizeEdge & Qt::TopEdge)    g.setTop(g.top() + delta.y());
	if (m_resizeEdge & Qt::BottomEdge) g.setBottom(g.bottom() + delta.y());

	// Минимальный размер — «прижимаем» активный край, а не отменяем всё
	const int minW = m_window->minimumWidth();
	const int minH = m_window->minimumHeight();

	if (g.width() < minW) {
		if (m_resizeEdge & Qt::LeftEdge) g.setLeft(g.right() - minW);
		else                             g.setRight(g.left() + minW);
	}
	if (g.height() < minH) {
		if (m_resizeEdge & Qt::TopEdge)  g.setTop(g.bottom() - minH);
		else                             g.setBottom(g.top() + minH);
	}

	m_window->setGeometry(g);
	return true;
}

void FramelessHelper::handleResizeRelease()
{
	m_resizing = false;
	m_resizeEdge = Qt::Edges();
	m_window->unsetCursor();
}

// ─── Edges / курсор ─────────────────────────────────────
QPoint FramelessHelper::mapToWindow(QObject* obj, const QPoint& pos) const
{
	if (obj == m_window) return pos;
	auto* w = qobject_cast<QWidget*>(obj);
	if (!w) return pos;
	return w->mapTo(m_window, pos);
}

Qt::Edges FramelessHelper::edgesAt(const QPoint& pos) const
{
	Qt::Edges edges;
	const int w = m_window->width();
	const int h = m_window->height();
	const int b = m_borderWidth;

	if (pos.x() <= b)       edges |= Qt::LeftEdge;
	if (pos.x() >= w - b)   edges |= Qt::RightEdge;
	if (pos.y() <= b)       edges |= Qt::TopEdge;
	if (pos.y() >= h - b)   edges |= Qt::BottomEdge;

	return edges;
}

Qt::CursorShape FramelessHelper::cursorForEdges(Qt::Edges edges) const
{
	if ((edges & Qt::LeftEdge) && (edges & Qt::TopEdge))    return Qt::SizeFDiagCursor;
	if ((edges & Qt::RightEdge) && (edges & Qt::BottomEdge)) return Qt::SizeFDiagCursor;
	if ((edges & Qt::RightEdge) && (edges & Qt::TopEdge))    return Qt::SizeBDiagCursor;
	if ((edges & Qt::LeftEdge) && (edges & Qt::BottomEdge)) return Qt::SizeBDiagCursor;

	if (edges & Qt::LeftEdge || edges & Qt::RightEdge)      return Qt::SizeHorCursor;
	if (edges & Qt::TopEdge || edges & Qt::BottomEdge)     return Qt::SizeVerCursor;

	return Qt::ArrowCursor;
}

void FramelessHelper::updateCursor(Qt::Edges edges)
{
	m_window->setCursor(cursorForEdges(edges));
}

void FramelessHelper::enableHoverTracking(QWidget* root)
{
	if (!root) return;

	root->setMouseTracking(true);
	root->setAttribute(Qt::WA_Hover, true);

	// Рекурсивно для всех дочерних виджетов
	for (QWidget* child : root->findChildren<QWidget*>()) {
		child->setMouseTracking(true);
		child->setAttribute(Qt::WA_Hover, true);
	}
}

// ─── Стили кнопок ───────────────────────────────────────
void FramelessHelper::applyButtonStyle()
{
	const QString base = R"(
        QPushButton {
            background: transparent;
            border: none;
            color: white;
            font-size: 14px;
        }
        QPushButton:hover   { background: rgba(255,255,255,40); }
        QPushButton:pressed { background: rgba(255,255,255,80); }
    )";

	const QString closeStyle = R"(
        QPushButton {
            background: transparent;
            border: none;
            color: white;
            font-size: 14px;
        }
        QPushButton:hover   { background: #e81123; }
        QPushButton:pressed { background: #bf0f1d; }
    )";

	m_minBtn->setStyleSheet(base);
	m_maxBtn->setStyleSheet(base);
	m_closeBtn->setStyleSheet(closeStyle);
}