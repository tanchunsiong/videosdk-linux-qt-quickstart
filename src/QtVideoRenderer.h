#pragma once

#include <QImage>

class QtVideoWidget;

class QtVideoRenderer
{
public:
    QtVideoRenderer(QtVideoWidget* widget);
    ~QtVideoRenderer();

    void renderVideoFrame(const char* y_data, const char* u_data, const char* v_data,
                         int width, int height, int y_stride, int u_stride, int v_stride);

private:
    QtVideoWidget* m_videoWidget;
    QImage convertYUVtoRGB(const char* y_data, const char* u_data, const char* v_data,
                          int width, int height, int y_stride, int u_stride, int v_stride);
};
