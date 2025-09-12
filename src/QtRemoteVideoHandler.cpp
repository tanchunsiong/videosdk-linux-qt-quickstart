#include "QtRemoteVideoHandler.h"
#include "QtVideoWidget.h"
#include "QtVideoRenderer.h"
#include <QTimer>
#include <QPainter>
#include <QDebug>

// Include Zoom SDK headers for video functionality
#include "zoom_video_sdk_api.h"
#include "zoom_video_sdk_def.h"
#include "zoom_video_sdk_interface.h"
#include "helpers/zoom_video_sdk_video_helper_interface.h"

USING_ZOOM_VIDEO_SDK_NAMESPACE

// Qt equivalent of GTK's RemoteVideoRawDataHandler for remote video
QtRemoteVideoHandler::QtRemoteVideoHandler(QtVideoWidget* widget)
    : QObject(nullptr)
    , m_videoWidget(widget)
    , m_currentUser(nullptr)
    , m_videoPipe(nullptr)
    , m_isSubscribed(false)
{
    qDebug() << "QtRemoteVideoHandler: Created new handler instance";
}

QtRemoteVideoHandler::~QtRemoteVideoHandler()
{
    Unsubscribe();
    qDebug() << "QtRemoteVideoHandler: Destroyed handler instance";
}

bool QtRemoteVideoHandler::SubscribeToUser(IZoomVideoSDKUser* user, ZoomVideoSDKResolution resolution)
{
    if (!user || !m_videoWidget) {
        qDebug() << "QtRemoteVideoHandler: Invalid user or widget";
        return false;
    }

    // Unsubscribe from any existing user first
    Unsubscribe();

    // Get the user's video pipe
    m_videoPipe = user->GetVideoPipe();
    if (!m_videoPipe) {
        qDebug() << "QtRemoteVideoHandler: No video pipe available for user" << user->getUserName();
        return false;
    }

    // Subscribe to raw data from the video pipe with specified resolution
    ZoomVideoSDKErrors err = m_videoPipe->subscribe(resolution, this);
    if (err == ZoomVideoSDKErrors_Success) {
        m_currentUser = user;
        m_isSubscribed = true;
        qDebug() << "QtRemoteVideoHandler: Successfully subscribed to raw data for user" << user->getUserName() 
                 << "at resolution" << (int)resolution;
        return true;
    } else {
        qDebug() << "QtRemoteVideoHandler: Failed to subscribe to raw data for user" << user->getUserName() 
                 << "at resolution" << (int)resolution << ", error:" << (int)err;
        m_videoPipe = nullptr;
        return false;
    }
}

bool QtRemoteVideoHandler::Unsubscribe()
{
    if (m_isSubscribed && m_videoPipe) {
        ZoomVideoSDKErrors err = m_videoPipe->unSubscribe(this);
        if (err == ZoomVideoSDKErrors_Success) {
            qDebug() << "QtRemoteVideoHandler: Successfully unsubscribed from raw data";
        } else {
            qDebug() << "QtRemoteVideoHandler: Failed to unsubscribe from raw data, error:" << (int)err;
        }
    }
    
    m_currentUser = nullptr;
    m_videoPipe = nullptr;
    m_isSubscribed = false;
    return true;
}

void QtRemoteVideoHandler::onRawDataFrameReceived(YUVRawDataI420* data)
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
    
    // Render the remote video frame using QtVideoRenderer
    QtVideoRenderer renderer(m_videoWidget);
    renderer.renderVideoFrame(y_data, u_data, v_data, width, height, y_stride, u_stride, v_stride);
    
    // Debug output (can be removed later)
    static int frame_count = 0;
    if (++frame_count % 30 == 0) { // Print every 30 frames
        qDebug() << "QtRemoteVideoHandler: Rendered remote video frame" << frame_count 
                 << "(" << width << "x" << height << ")";
        if (m_currentUser) {
            qDebug() << "from user" << m_currentUser->getUserName();
        }
    }
}

void QtRemoteVideoHandler::onRawDataStatusChanged(RawDataStatus status)
{
    const char* status_str = (status == RawData_On) ? "ON" : "OFF";
    qDebug() << "QtRemoteVideoHandler: Raw data status changed to" << status_str;
    if (m_currentUser) {
        qDebug() << "for user" << m_currentUser->getUserName();
    }

    // Update subscription status based on raw data status
    if (status == RawData_Off && m_isSubscribed) {
        qDebug() << "QtRemoteVideoHandler: Raw data turned off, cleaning up subscription";
        Unsubscribe();
    }
}

void QtRemoteVideoHandler::onShareCursorDataReceived(ZoomVideoSDKShareCursorData info)
{
    // Handle cursor data if needed for remote video rendering
    // For now, just log the event
    qDebug() << "QtRemoteVideoHandler: Share cursor data received";
    if (m_currentUser) {
        qDebug() << "for user" << m_currentUser->getUserName();
    }
}

// Legacy methods (kept for compatibility)
void QtRemoteVideoHandler::onRemoteVideoFrameReceived(const QImage& frame)
{
    if (!m_isSubscribed || !m_videoWidget) {
        return;
    }

    // Pass frame to video widget for display
    m_videoWidget->updateVideoFrame(frame);
}

void QtRemoteVideoHandler::onYUVFrameReceived(const char* y_data, const char* u_data, const char* v_data,
                                             int width, int height, int y_stride, int u_stride, int v_stride)
{
    if (!m_isSubscribed || !m_videoWidget) {
        return;
    }

    // Use QtVideoRenderer to convert YUV to RGB and display
    QtVideoRenderer renderer(m_videoWidget);
    renderer.renderVideoFrame(y_data, u_data, v_data, width, height, y_stride, u_stride, v_stride);
}
