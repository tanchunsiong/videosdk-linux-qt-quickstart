#pragma once

#include <QObject>
#include <QImage>
#include "helpers/zoom_video_sdk_user_helper_interface.h"

USING_ZOOM_VIDEO_SDK_NAMESPACE

class QtVideoWidget;

// Qt equivalent of GTK's PreviewVideoHandler for self video
class QtPreviewVideoHandler : public QObject, private IZoomVideoSDKRawDataPipeDelegate
{
    Q_OBJECT

public:
    QtPreviewVideoHandler(QtVideoWidget* widget);
    ~QtPreviewVideoHandler();

    bool StartPreview();
    bool StopPreview();
    bool IsPreviewActive() const { return m_isRunning; }

private:
    // IZoomVideoSDKRawDataPipeDelegate implementation
    virtual void onRawDataFrameReceived(YUVRawDataI420* data) override;
    virtual void onRawDataStatusChanged(RawDataStatus status) override;
    virtual void onShareCursorDataReceived(ZoomVideoSDKShareCursorData info) override;

    QtVideoWidget* m_videoWidget;
    bool m_isRunning;

    // Legacy methods (kept for compatibility)
    void onPreviewFrameReceived(const QImage& frame);
    void onYUVFrameReceived(const char* y_data, const char* u_data, const char* v_data,
                           int width, int height, int y_stride, int u_stride, int v_stride);
};
