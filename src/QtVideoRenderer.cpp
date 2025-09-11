#include "QtVideoRenderer.h"
#include "QtVideoWidget.h"
#include <QDebug>

QtVideoRenderer::QtVideoRenderer(QtVideoWidget* widget)
    : m_videoWidget(widget)
{
}

QtVideoRenderer::~QtVideoRenderer()
{
}

void QtVideoRenderer::renderVideoFrame(const char* y_data, const char* u_data, const char* v_data,
                                      int width, int height, int y_stride, int u_stride, int v_stride)
{
    if (!m_videoWidget || !y_data || !u_data || !v_data) {
        return;
    }

    QImage rgbFrame = convertYUVtoRGB(y_data, u_data, v_data, width, height, y_stride, u_stride, v_stride);

    // Update the video widget with the new frame
    m_videoWidget->updateVideoFrame(rgbFrame);
}

QImage QtVideoRenderer::convertYUVtoRGB(const char* y_data, const char* u_data, const char* v_data,
                                       int width, int height, int y_stride, int u_stride, int v_stride)
{
    QImage rgbImage(width, height, QImage::Format_RGB32);

    const uint8_t* y_plane = reinterpret_cast<const uint8_t*>(y_data);
    const uint8_t* u_plane = reinterpret_cast<const uint8_t*>(u_data);
    const uint8_t* v_plane = reinterpret_cast<const uint8_t*>(v_data);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Get YUV values with proper stride handling
            int y_val = y_plane[y * y_stride + x];
            int u_val = u_plane[(y / 2) * u_stride + (x / 2)];
            int v_val = v_plane[(y / 2) * v_stride + (x / 2)];

            // Convert YUV to RGB using ITU-R BT.601 coefficients for video range
            // Y range: 16-235, UV range: 16-240 (centered at 128)
            int c = y_val - 16;
            int d = u_val - 128;
            int e = v_val - 128;

            // Use proper BT.601 conversion matrix
            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;

            // Clamp values to valid RGB range
            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            // Set RGB pixel in QImage (Format_RGB32 uses 0xFFRRGGBB)
            rgbImage.setPixel(x, y, qRgb(r, g, b));
        }
    }

    return rgbImage;
}
