#pragma once

#include <QWidget>
#include <QPixmap>
#include <QElapsedTimer>
#include <QPropertyAnimation>
#include <QGraphicsBlurEffect>
#include <QGraphicsOpacityEffect>

class PreviewLogoShower : public QWidget
{
    Q_OBJECT

public:
    // geometry_ref нужен только чтобы узнать начальный размер и позицию окна
    explicit PreviewLogoShower(QWidget* geometry_ref,
        const QString& resource_path,
        int min_show_ms = 1000,
        int fade_ms = 800);

    ~PreviewLogoShower() override;

    void hideWhenReady();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void startHide();

private:
    QWidget* m_geometry_ref_ = nullptr;
    QPixmap m_pixmap;
    QElapsedTimer m_elapsed_;

    QGraphicsBlurEffect* m_blur_effect_ = nullptr;
    QGraphicsOpacityEffect* m_opacity_effect_ = nullptr;

    QPropertyAnimation* m_blur_anim_ = nullptr;
    QPropertyAnimation* m_fade_anim_ = nullptr;

    int m_min_show_ms_;
    int m_fade_ms_;
    bool m_is_hiding_ = false;

    void updateGeometry();
};