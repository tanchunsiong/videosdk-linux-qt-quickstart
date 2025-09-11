#pragma once

#include <QWidget>
#include <QImage>
#include <QMutex>

QT_BEGIN_NAMESPACE
class QPaintEvent;
QT_END_NAMESPACE

class QtVideoRenderer;

class QtVideoWidget : public QWidget
{
    Q_OBJECT

public:
    QtVideoWidget(QWidget* parent = nullptr);
    ~QtVideoWidget();

    void setVideoRenderer(QtVideoRenderer* renderer);
    void updateVideoFrame(const QImage& frame);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QtVideoRenderer* m_renderer;
    QImage m_currentFrame;
    QMutex m_frameMutex;
};
