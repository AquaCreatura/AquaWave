#include "image_shower.h"
#include <QPainter>
#include <QTimer>
#include <QEvent>
#include <QParallelAnimationGroup>
#include <qguiapplication.h>
#include <qscreen.h>
#include <QGuiApplication>
#include <QScreen>

#include <QGuiApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>

ImageShower::ImageShower(QWidget* geometry_ref,
    const QString& resource_path,
    int min_show_ms,
    int fade_ms)
    : QWidget(nullptr)
    , m_geometry_ref_(geometry_ref)
    , m_pixmap(resource_path)
    , m_min_show_ms_(min_show_ms)
    , m_fade_ms_(fade_ms)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    // 1. Определяем размер сплэша
    QRect startRect;
    if (m_geometry_ref_) {
        startRect = m_geometry_ref_->rect();
        // Если окно еще "схлопнулось" до минимума
        if (startRect.width() < 200 || startRect.height() < 200) {
            startRect.setSize(QSize(1'000, 800));
        }
    }
    else {
        startRect = QRect(0, 0, 1'000, 800);
    }

    // 2. 🔥 Находим экран под курсором мыши
    QScreen* targetScreen = nullptr;

    // Пытаемся найти экран, где сейчас находится курсор
    for (QScreen* screen : QGuiApplication::screens()) {
        if (screen->geometry().contains(QCursor::pos())) {
            targetScreen = screen;
            break;
        }
    }

    // Если вдруг курсор не попал ни на один экран (редкий баг ОС), берем первичный
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen();
    }

    // 3. Центрируем сплэш на найденном экране
    if (targetScreen) {
        QRect screenGeom = targetScreen->availableGeometry();
        QPoint centerPos = screenGeom.center() - startRect.center();
        setGeometry(centerPos.x(), centerPos.y(), startRect.width(), startRect.height());
    }

    if (m_geometry_ref_) {
        m_geometry_ref_->installEventFilter(this);
    }

    m_opacity_effect_ = new QGraphicsOpacityEffect(this);
    m_opacity_effect_->setOpacity(1.0);
    setGraphicsEffect(m_opacity_effect_);

    m_elapsed_.start();
}
ImageShower::~ImageShower()
{
    if (m_geometry_ref_) m_geometry_ref_->removeEventFilter(this);
    if (m_blur_anim_) m_blur_anim_->stop();
    if (m_fade_anim_) m_fade_anim_->stop();
}

void ImageShower::updateGeometry()
{
    if (m_geometry_ref_) {
        setGeometry(m_geometry_ref_->geometry());
    }
}

bool ImageShower::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_geometry_ref_) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
            updateGeometry();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ImageShower::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_pixmap.isNull()) {
        QPixmap scaled = m_pixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
    }
    QWidget::paintEvent(event);
}

void ImageShower::hideWhenReady()
{
    if (m_is_hiding_) return;

    int elapsed = static_cast<int>(m_elapsed_.elapsed());
    int remaining = qMax(0, m_min_show_ms_ - elapsed);

    QTimer::singleShot(remaining, this, &ImageShower::startHide);
}

//void ImageShower::startHide()
//{
//    if (m_is_hiding_) return;
//    m_is_hiding_ = true;
//
//    // Анимируем свойство "pos" (позицию окна на экране)
//    m_fade_anim_ = new QPropertyAnimation(this, "pos");
//
//    m_fade_anim_->setDuration(m_fade_ms_);
//
//    // Стартуем с текущей позиции
//    m_fade_anim_->setStartValue(pos());
//
//    // Финишируем, сдвинувшись вниз на полную высоту окна
//    QPoint endPos = pos();
//    endPos.setY(endPos.y() + height());
//    m_fade_anim_->setEndValue(endPos);
//
//    // InCubic дает приятное ускорение в начале падения
//    m_fade_anim_->setEasingCurve(QEasingCurve::InCubic);
//
//    connect(m_fade_anim_, &QPropertyAnimation::finished, this, [this]() {
//        close();
//        });
//
//    m_fade_anim_->start();
//}

void ImageShower::startHide()
{
    if (m_is_hiding_) return;
    m_is_hiding_ = true;

    // Анимируем только прозрачность
    m_fade_anim_ = new QPropertyAnimation(m_opacity_effect_, "opacity");

    m_fade_anim_->setDuration(m_fade_ms_);
    m_fade_anim_->setStartValue(1);
    m_fade_anim_->setEndValue(0);

    // OutCubic дает очень мягкое и естественное затухание в конце
    m_fade_anim_->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_fade_anim_, &QPropertyAnimation::finished, this, [this]() {
        close();
        });

    m_fade_anim_->start();
}