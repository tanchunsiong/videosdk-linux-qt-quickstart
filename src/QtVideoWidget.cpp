#include "QtVideoWidget.h"
#include <QPainter>
#include <QDebug>

QtVideoWidget::QtVideoWidget(QWidget* parent)
    : QWidget(parent)
    , m_renderer(nullptr)
{
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

QtVideoWidget::~QtVideoWidget()
{
}

void QtVideoWidget::setVideoRenderer(QtVideoRenderer* renderer)
{
    m_renderer = renderer;
}

void QtVideoWidget::updateVideoFrame(const QImage& frame)
{
    QMutexLocker locker(&m_frameMutex);
    m_currentFrame = frame;
    update(); // Trigger repaint
}

void QtVideoWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black); // Fill with black background

    QMutexLocker locker(&m_frameMutex);
    if (!m_currentFrame.isNull()) {
        // Calculate scaling to maintain aspect ratio
        QRect targetRect = rect();
        QSize imageSize = m_currentFrame.size();

        double scaleX = (double)targetRect.width() / imageSize.width();
        double scaleY = (double)targetRect.height() / imageSize.height();
        double scale = qMin(scaleX, scaleY);

        int scaledWidth = (int)(imageSize.width() * scale);
        int scaledHeight = (int)(imageSize.height() * scale);

        // Center the video
        int offsetX = (targetRect.width() - scaledWidth) / 2;
        int offsetY = (targetRect.height() - scaledHeight) / 2;

        QRect drawRect(offsetX, offsetY, scaledWidth, scaledHeight);
        painter.drawImage(drawRect, m_currentFrame);
    } else {
        // Draw placeholder text when no video
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Waiting for video...");
    }
}
