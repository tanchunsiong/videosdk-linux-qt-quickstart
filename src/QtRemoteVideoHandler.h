#pragma once

#include <QObject>
#include <QImage>
#include "helpers/zoom_video_sdk_user_helper_interface.h"

USING_ZOOM_VIDEO_SDK_NAMESPACE

class QtVideoWidget;

// Qt equivalent of GTK's RemoteVideoRawDataHandler for remote video
class QtRemoteVideoHandler : public QObject, private IZoomVideoSDKRawDataPipeDelegate
{
    Q_OBJECT

public:
    QtRemoteVideoHandler(QtVideoWidget* widget);
    ~QtRemoteVideoHandler();

    bool SubscribeToUser(IZoomVideoSDKUser* user, ZoomVideoSDKResolution resolution = ZoomVideoSDKResolution_90P);
    bool Unsubscribe();

private:
    // IZoomVideoSDKRawDataPipeDelegate implementation
    virtual void onRawDataFrameReceived(YUVRawDataI420* data) override;
    virtual void onRawDataStatusChanged(RawDataStatus status) override;
    virtual void onShareCursorDataReceived(ZoomVideoSDKShareCursorData info) override;

    QtVideoWidget* m_videoWidget;
    IZoomVideoSDKUser* m_currentUser;
    IZoomVideoSDKRawDataPipe* m_videoPipe;
    bool m_isSubscribed;

    // Legacy methods (kept for compatibility)
    void onRemoteVideoFrameReceived(const QImage& frame);
    void onYUVFrameReceived(const char* y_data, const char* u_data, const char* v_data,
                           int width, int height, int y_stride, int u_stride, int v_stride);
};
