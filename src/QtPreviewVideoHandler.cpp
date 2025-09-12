#include "QtPreviewVideoHandler.h"
#include "QtVideoWidget.h"
#include "QtVideoRenderer.h"
#include <QTimer>
#include <QPainter>
#include <QDebug>
#include <QRandomGenerator>

// Include Zoom SDK headers for video functionality
#include "zoom_video_sdk_api.h"
#include "zoom_video_sdk_def.h"
#include "zoom_video_sdk_interface.h"
#include "helpers/zoom_video_sdk_video_helper_interface.h"

USING_ZOOM_VIDEO_SDK_NAMESPACE

// External reference to the global SDK object
extern IZoomVideoSDK* video_sdk_obj;

// Qt equivalent of GTK's PreviewVideoHandler for self video
QtPreviewVideoHandler::QtPreviewVideoHandler(QtVideoWidget* widget)
    : QObject(nullptr)
    , m_videoWidget(widget)
    , m_isRunning(false)
{
    qDebug() << "QtPreviewVideoHandler: Created new handler instance";
}

QtPreviewVideoHandler::~QtPreviewVideoHandler()
{
    StopPreview();
    qDebug() << "QtPreviewVideoHandler: Destroyed handler instance";
}

bool QtPreviewVideoHandler::StartPreview()
{
    if (!video_sdk_obj || !m_videoWidget) {
        qDebug() << "QtPreviewVideoHandler: SDK or widget not available";
        return false;
    }

    IZoomVideoSDKVideoHelper* videoHelper = video_sdk_obj->getVideoHelper();
    if (!videoHelper) {
        qDebug() << "QtPreviewVideoHandler: Video helper not available";
        return false;
    }

    ZoomVideoSDKErrors err = videoHelper->startVideoPreview(this);
    if (err == ZoomVideoSDKErrors_Success) {
        m_isRunning = true;
        qDebug() << "QtPreviewVideoHandler: Preview started successfully";
        return true;
    } else {
        qDebug() << "QtPreviewVideoHandler: Failed to start preview, error:" << (int)err;
        return false;
    }
}

bool QtPreviewVideoHandler::StopPreview()
{
    if (!video_sdk_obj || !m_isRunning) {
        return true;
    }

    IZoomVideoSDKVideoHelper* videoHelper = video_sdk_obj->getVideoHelper();
    if (videoHelper) {
        ZoomVideoSDKErrors err = videoHelper->stopVideoPreview(this);
        if (err == ZoomVideoSDKErrors_Success) {
            qDebug() << "QtPreviewVideoHandler: Preview stopped successfully";
        } else {
            qDebug() << "QtPreviewVideoHandler: Failed to stop preview, error:" << (int)err;
        }
    }
    
    m_isRunning = false;
    return true;
}

void QtPreviewVideoHandler::onRawDataFrameReceived(YUVRawDataI420* data)
{
    if (!data || !m_videoWidget) {
        return;
    }

    const int width = data->GetStreamWidth();
    const int height = data->GetStreamHeight();
    
    // Get YUV data pointers
    const char* y_data = reinterpret_cast<const char*>(data->GetYBuffer());
    const char* u_data = reinterpret_cast<const char*>(data->GetUBuffer());
    const char* v_data = reinterpret_cast<const char*>(data->GetVBuffer());
    
    // Calculate strides (assuming standard YUV420 layout)
    const int y_stride = width;
    const int u_stride = width / 2;
    const int v_stride = width / 2;
    
    // Render the preview frame using QtVideoRenderer
    QtVideoRenderer renderer(m_videoWidget);
    renderer.renderVideoFrame(y_data, u_data, v_data, width, height, y_stride, u_stride, v_stride);
    
    // Debug output (can be removed later)
    static int frame_count = 0;
    if (++frame_count % 30 == 0) { // Print every 30 frames
        qDebug() << "QtPreviewVideoHandler: Rendered preview frame" << frame_count 
                 << "(" << width << "x" << height << ")";
    }
}

void QtPreviewVideoHandler::onRawDataStatusChanged(RawDataStatus status)
{
    m_isRunning = (status == RawData_On);
    const char* status_str = (status == RawData_On) ? "ON" : "OFF";
    qDebug() << "QtPreviewVideoHandler: Preview status changed to" << status_str;
}

void QtPreviewVideoHandler::onShareCursorDataReceived(ZoomVideoSDKShareCursorData info)
{
    // Handle cursor data if needed for preview rendering
    // For now, just log the event
    qDebug() << "QtPreviewVideoHandler: Share cursor data received";
}

// Legacy methods (kept for compatibility)
void QtPreviewVideoHandler::onPreviewFrameReceived(const QImage& frame)
{
    if (!m_isRunning || !m_videoWidget) {
        return;
    }

    // Pass frame to video widget for display
    m_videoWidget->updateVideoFrame(frame);
}

void QtPreviewVideoHandler::onYUVFrameReceived(const char* y_data, const char* u_data, const char* v_data,
                                              int width, int height, int y_stride, int u_stride, int v_stride)
{
    if (!m_isRunning || !m_videoWidget) {
        return;
    }

    // Use QtVideoRenderer to convert YUV to RGB and display
    QtVideoRenderer renderer(m_videoWidget);
    renderer.renderVideoFrame(y_data, u_data, v_data, width, height, y_stride, u_stride, v_stride);
}
