#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QMetaObject>
#include <QThread>
#include <QDebug>

// Include our Qt classes
#include "QtMainWindow.h"
#include "QtVideoWidget.h"
#include "QtVideoRenderer.h"
#include "QtRemoteVideoHandler.h"
#include "QtPreviewVideoHandler.h"

// Test SDK loading without Qt dependencies first
#include <iostream>
#include <dlfcn.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <sstream>
#include <thread>
#include <map>
#include <ctime>
#include <alsa/asoundlib.h>

#include "json.hpp"

// Simple audio playback class using ALSA
class AudioPlayback {
private:
    snd_pcm_t* pcm_handle;
    bool initialized;

public:
    AudioPlayback() : pcm_handle(nullptr), initialized(false) {}

    ~AudioPlayback() {
        cleanup();
    }

    bool init() {
        int err;

        // Open PCM device for playback
        err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
            printf("Failed to open PCM device: %s\n", snd_strerror(err));
            return false;
        }

        // Set hardware parameters
        snd_pcm_hw_params_t* hw_params;
        snd_pcm_hw_params_alloca(&hw_params);

        err = snd_pcm_hw_params_any(pcm_handle, hw_params);
        if (err < 0) {
            printf("Failed to initialize hw_params: %s\n", snd_strerror(err));
            return false;
        }

        // Set access type
        err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) {
            printf("Failed to set access type: %s\n", snd_strerror(err));
            return false;
        }

        // Set sample format (16-bit signed)
        err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
        if (err < 0) {
            printf("Failed to set sample format: %s\n", snd_strerror(err));
            return false;
        }

        // Set sample rate (44.1kHz)
        unsigned int rate = 44100;
        err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0);
        if (err < 0) {
            printf("Failed to set sample rate: %s\n", snd_strerror(err));
            return false;
        }

        // Set number of channels (stereo)
        err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 2);
        if (err < 0) {
            printf("Failed to set channel count: %s\n", snd_strerror(err));
            return false;
        }

        // Apply hardware parameters
        err = snd_pcm_hw_params(pcm_handle, hw_params);
        if (err < 0) {
            printf("Failed to set hw params: %s\n", snd_strerror(err));
            return false;
        }

        // Prepare the PCM device
        err = snd_pcm_prepare(pcm_handle);
        if (err < 0) {
            printf("Failed to prepare PCM device: %s\n", snd_strerror(err));
            return false;
        }

        initialized = true;
        printf("Audio playback initialized successfully\n");
        return true;
    }

    void playAudio(const char* buffer, int buffer_len) {
        if (!initialized || !pcm_handle || !buffer) return;

        // Convert buffer length to frames (assuming 16-bit stereo)
        int frames = buffer_len / 4; // 2 bytes per sample * 2 channels

        snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, buffer, frames);
        if (written < 0) {
            // Handle underrun
            if (written == -EPIPE) {
                printf("Audio underrun occurred, recovering...\n");
                snd_pcm_prepare(pcm_handle);
            } else {
                printf("Audio write error: %s\n", snd_strerror(written));
            }
        }
    }

    void cleanup() {
        if (pcm_handle) {
            snd_pcm_close(pcm_handle);
            pcm_handle = nullptr;
        }
        initialized = false;
    }
};

// Global audio playback instance
AudioPlayback* g_audio_playback = nullptr;

// Include Zoom SDK headers
#include "helpers/zoom_video_sdk_user_helper_interface.h"
#include "zoom_video_sdk_api.h"
#include "zoom_video_sdk_def.h"
#include "zoom_video_sdk_delegate_interface.h"
#include "zoom_video_sdk_interface.h"
#include "zoom_video_sdk_session_info_interface.h"
#include "zoom_video_sdk_platform.h"

//needed for chat
#include "helpers/zoom_video_sdk_chat_helper_interface.h"
#include "zoom_video_sdk_chat_message_interface.h"

using Json = nlohmann::json;
USING_ZOOM_VIDEO_SDK_NAMESPACE
IZoomVideoSDK* video_sdk_obj = nullptr;

// Global variables
bool g_in_session = false;
bool g_audio_muted = false;
bool g_video_muted = false;

//controls to demonstrate the flow
bool enableChat = true;

// Zoom Video SDK Delegate
class ZoomVideoSDKDelegate : public IZoomVideoSDKDelegate
{
public:
    ZoomVideoSDKDelegate(QtMainWindow* mainWindow) : m_mainWindow(mainWindow) {}

    /// \brief Triggered when user enter the session.
    virtual void onSessionJoin()
    {
        printf("=== DELEGATE: Session joined successfully ===\n");

        // CRITICAL FIX: Set session state BEFORE updating UI
        g_in_session = true;
        printf("DEBUG: Setting g_in_session = true (BEFORE UI update)\n");

        // Initialize audio playback system
        if (!g_audio_playback) {
            printf("Initializing audio playback system...\n");
            g_audio_playback = new AudioPlayback();
            if (!g_audio_playback->init()) {
                printf("ERROR: Failed to initialize audio playback\n");
                delete g_audio_playback;
                g_audio_playback = nullptr;
            } else {
                printf("Audio playback system initialized successfully\n");
            }
        }

        // Update UI on main thread - try direct call first
        printf("Updating UI status...\n");
        if (QThread::currentThread() == m_mainWindow->thread()) {
            // We're already on the main thread, call directly
            m_mainWindow->updateStatus("Session joined successfully");
            m_mainWindow->updateButtonStates();
        } else {
            // Cross-thread, use blocking queued connection
            QMetaObject::invokeMethod(m_mainWindow, "updateStatus",
                Qt::BlockingQueuedConnection, Q_ARG(QString, "Session joined successfully"));
            QMetaObject::invokeMethod(m_mainWindow, "updateButtonStates", Qt::BlockingQueuedConnection);
        }

        printf("Session state set to IN_SESSION\n");

        if (enableChat) {
            printf("Attempting to send chat message...\n");
            try {
                IZoomVideoSDKChatHelper* pChatHelper = video_sdk_obj->getChatHelper();
                if (pChatHelper) {
                    printf("Chat helper obtained\n");
                    if (pChatHelper->isChatDisabled() == false && pChatHelper->isPrivateChatDisabled() == false) {
                        ZoomVideoSDKErrors err = pChatHelper->sendChatToAll("hello world from Qt client");
                        printf("Chat message sent, status: %d\n", (int)err);
                    } else {
                        printf("Chat is disabled\n");
                    }
                } else {
                    printf("ERROR: Could not get chat helper\n");
                }
            } catch (const std::exception& e) {
                printf("EXCEPTION in chat: %s\n", e.what());
            } catch (...) {
                printf("UNKNOWN EXCEPTION in chat\n");
            }
        } else {
            printf("Chat disabled by configuration\n");
        }

        printf("=== Session join callback complete ===\n");
    }

    /// \brief Triggered when session leaveSession
    virtual void onSessionLeave()
    {
        printf("Left session.\n");

        // Clean up audio playback system
        if (g_audio_playback) {
            printf("Cleaning up audio playback system\n");
            delete g_audio_playback;
            g_audio_playback = nullptr;
        }

        // Update UI on main thread - try direct call first
        if (QThread::currentThread() == m_mainWindow->thread()) {
            // We're already on the main thread, call directly
            m_mainWindow->updateStatus("Left session");
            m_mainWindow->updateButtonStates();
        } else {
            // Cross-thread, use blocking queued connection
            QMetaObject::invokeMethod(m_mainWindow, "updateStatus",
                Qt::BlockingQueuedConnection, Q_ARG(QString, "Left session"));
            QMetaObject::invokeMethod(m_mainWindow, "updateButtonStates", Qt::BlockingQueuedConnection);
        }

        g_in_session = false;
    };

    virtual void onSessionLeave(ZoomVideoSDKSessionLeaveReason eReason)
    {
        printf("Left session with reason: %d\n", (int)eReason);

        // Clean up audio playback system
        if (g_audio_playback) {
            printf("Cleaning up audio playback system\n");
            delete g_audio_playback;
            g_audio_playback = nullptr;
        }

        // Update UI on main thread - try direct call first
        if (QThread::currentThread() == m_mainWindow->thread()) {
            // We're already on the main thread, call directly
            m_mainWindow->updateStatus("Left session");
            m_mainWindow->updateButtonStates();
        } else {
            // Cross-thread, use blocking queued connection
            QMetaObject::invokeMethod(m_mainWindow, "updateStatus",
                Qt::BlockingQueuedConnection, Q_ARG(QString, "Left session"));
            QMetaObject::invokeMethod(m_mainWindow, "updateButtonStates", Qt::BlockingQueuedConnection);
        }

        g_in_session = false;
    };

    virtual void onError(ZoomVideoSDKErrors errorCode, int detailErrorCode)
    {
        printf("join session errorCode : %d  detailErrorCode: %d\n", errorCode, detailErrorCode);

        // Update UI on main thread - try direct call first
        if (QThread::currentThread() == m_mainWindow->thread()) {
            // We're already on the main thread, call directly
            m_mainWindow->updateStatus("Session error occurred");
        } else {
            // Cross-thread, use blocking queued connection
            QMetaObject::invokeMethod(m_mainWindow, "updateStatus",
                Qt::BlockingQueuedConnection, Q_ARG(QString, "Session error occurred"));
        }
    };

    // Other delegate methods...
    virtual void onUserJoin(IZoomVideoSDKUserHelper* pUserHelper, IVideoSDKVector<IZoomVideoSDKUser*>* userList) {}
    virtual void onUserLeave(IZoomVideoSDKUserHelper* pUserHelper, IVideoSDKVector<IZoomVideoSDKUser*>* userList) {}
	virtual void onUserVideoStatusChanged(IZoomVideoSDKVideoHelper* pVideoHelper, IVideoSDKVector<IZoomVideoSDKUser*>* userList) {
		if (userList && video_sdk_obj && m_mainWindow) {
			// Get current user to exclude from remote video display
			IZoomVideoSDKSession* session = video_sdk_obj->getSessionInfo();
			IZoomVideoSDKUser* myself = session ? session->getMyself() : nullptr;

			int count = userList->GetCount();
			for (int index = 0; index < count; index++) {
				IZoomVideoSDKUser* user = userList->GetItem(index);
				if (user && user != myself) { // Only handle remote users, not myself
					printf("Video status changed for remote user: %s\n", user->getUserName());

					// Check if user has video enabled
					if (user->GetVideoPipe()) {
						printf("User %s has video pipe available - checking for existing handler\n", user->getUserName());

						// TODO: Check if we already have a handler for this user
						// For now, create new handler (will be fixed to prevent duplicates)
						if (m_mainWindow->getRemoteVideoWidget()) {
							QtRemoteVideoHandler* remoteHandler = new QtRemoteVideoHandler(m_mainWindow->getRemoteVideoWidget());
							if (remoteHandler->SubscribeToUser(user, ZoomVideoSDKResolution_90P)) {
								printf("Successfully subscribed to remote video for user: %s\n", user->getUserName());
								// TODO: Store handler in list to prevent duplicates
							} else {
								printf("Failed to subscribe to remote video for user: %s\n", user->getUserName());
								delete remoteHandler;
							}
						}
					} else {
						printf("User %s has no video pipe - remote video disabled\n", user->getUserName());
						// TODO: Clean up existing handler for this user
					}
				}
				else if (user == myself)
				{
					printf("Self user detected in onUserVideoStatusChanged: %s - using preview handler\n", user->getUserName());
				}
			}
		}
	}
    virtual void onUserAudioStatusChanged(IZoomVideoSDKAudioHelper* pAudioHelper, IVideoSDKVector<IZoomVideoSDKUser*>* userList) {}
    virtual void onUserShareStatusChanged(IZoomVideoSDKShareHelper* pShareHelper, IZoomVideoSDKUser* pUser, IZoomVideoSDKShareAction* pShareAction) {}
    virtual void onShareContentChanged(IZoomVideoSDKShareHelper* pShareHelper, IZoomVideoSDKUser* pUser, IZoomVideoSDKShareAction* pShareAction) {}
    virtual void onFailedToStartShare(IZoomVideoSDKShareHelper* pShareHelper, IZoomVideoSDKUser* pUser) {}
    virtual void onShareSettingChanged(ZoomVideoSDKShareSetting setting) {}
    virtual void onUserRecordingConsent(IZoomVideoSDKUser* pUser) {}
    virtual void onLiveStreamStatusChanged(IZoomVideoSDKLiveStreamHelper* pLiveStreamHelper, ZoomVideoSDKLiveStreamStatus status) {}
    virtual void onChatNewMessageNotify(IZoomVideoSDKChatHelper* pChatHelper, IZoomVideoSDKChatMessage* messageItem) {}
    virtual void onUserHostChanged(IZoomVideoSDKUserHelper* pUserHelper, IZoomVideoSDKUser* pUser) {}
    virtual void onUserActiveAudioChanged(IZoomVideoSDKAudioHelper* pAudioHelper, IVideoSDKVector<IZoomVideoSDKUser*>* list) {}
    virtual void onSessionNeedPassword(IZoomVideoSDKPasswordHandler* handler) {}
    virtual void onSessionPasswordWrong(IZoomVideoSDKPasswordHandler* handler) {}
    virtual void onCommandReceived(IZoomVideoSDKUser* sender, const zchar_t* strCmd) {}
    virtual void onCommandChannelConnectResult(bool isSuccess) {};
    virtual void onInviteByPhoneStatus(PhoneStatus status, PhoneFailedReason reason) {};
    virtual void onCalloutJoinSuccess(IZoomVideoSDKUser* pUser, const zchar_t* phoneNumber) {};
    virtual void onCloudRecordingStatus(RecordingStatus status, IZoomVideoSDKRecordingConsentHandler* pHandler) {};
    virtual void onHostAskUnmute() {};
    virtual void onMultiCameraStreamStatusChanged(ZoomVideoSDKMultiCameraStreamStatus status, IZoomVideoSDKUser* pUser, IZoomVideoSDKRawDataPipe* pVideoPipe) {}
    virtual void onMicSpeakerVolumeChanged(unsigned int micVolume, unsigned int speakerVolume) {}
    virtual void onAudioDeviceStatusChanged(ZoomVideoSDKAudioDeviceType type, ZoomVideoSDKAudioDeviceStatus status) {}
    virtual void onTestMicStatusChanged(ZoomVideoSDK_TESTMIC_STATUS status) {}
    virtual void onSelectedAudioDeviceChanged() {}
    virtual void onCameraListChanged() {}
    virtual void onLiveTranscriptionStatus(ZoomVideoSDKLiveTranscriptionStatus status) {};
    virtual void onLiveTranscriptionMsgReceived(const zchar_t* ltMsg, IZoomVideoSDKUser* pUser, ZoomVideoSDKLiveTranscriptionOperationType type) {};
    virtual void onLiveTranscriptionMsgInfoReceived(ILiveTranscriptionMessageInfo* messageInfo) {};
    virtual void onLiveTranscriptionMsgError(ILiveTranscriptionLanguage* spokenLanguage, ILiveTranscriptionLanguage* transcriptLanguage) {};
    virtual void onSpokenLanguageChanged(ILiveTranscriptionLanguage* spokenLanguage) {}
    virtual void onShareNetworkStatusChanged(ZoomVideoSDKNetworkStatus shareNetworkStatus, bool isSendingShare) {}
    virtual void onAnnotationPrivilegeChange(IZoomVideoSDKUser* pUser, IZoomVideoSDKShareAction* pShareAction) {}
    virtual void onSpotlightVideoChanged(IZoomVideoSDKVideoHelper* pVideoHelper, IVideoSDKVector<IZoomVideoSDKUser*>* userList) {}
    virtual void onBindIncomingLiveStreamResponse(bool bSuccess, const zchar_t* strStreamKeyID) {}
    virtual void onUnbindIncomingLiveStreamResponse(bool bSuccess, const zchar_t* strStreamKeyID) {}
    virtual void onIncomingLiveStreamStatusResponse(bool bSuccess, IVideoSDKVector<IncomingLiveStreamStatus>* pStreamsStatusList) {}
    virtual void onStartIncomingLiveStreamResponse(bool bSuccess, const zchar_t* strStreamKeyID) {}
    virtual void onStopIncomingLiveStreamResponse(bool bSuccess, const zchar_t* strStreamKeyID) {}
    virtual void onShareContentSizeChanged(IZoomVideoSDKShareHelper* pShareHelper, IZoomVideoSDKUser* pUser, IZoomVideoSDKShareAction* pShareAction) {}
    virtual void onSubSessionStatusChanged(ZoomVideoSDKSubSessionStatus status, IVideoSDKVector<ISubSessionKit*>* pSubSessionKitList) {}
    virtual void onSubSessionManagerHandle(IZoomVideoSDKSubSessionManager* pManager) {}
    virtual void onSubSessionParticipantHandle(IZoomVideoSDKSubSessionParticipant* pParticipant) {}
    virtual void onSubSessionUsersUpdate(ISubSessionKit* pSubSessionKit) {}
    virtual void onBroadcastMessageFromMainSession(const zchar_t* sMessage, const zchar_t* sUserName) {}
    virtual void onSubSessionUserHelpRequest(ISubSessionUserHelpRequestHandler* pHandler) {}
    virtual void onSubSessionUserHelpRequestResult(ZoomVideoSDKUserHelpRequestResult eResult) {}
    virtual void onChatMsgDeleteNotification(IZoomVideoSDKChatHelper* pChatHelper, const zchar_t* msgID, ZoomVideoSDKChatMessageDeleteType deleteBy) {};
    virtual void onVirtualSpeakerMixedAudioReceived(AudioRawData* data_) {}
    virtual void onVirtualSpeakerOneWayAudioReceived(AudioRawData* data_, IZoomVideoSDKUser* pUser) {}
    virtual void onVirtualSpeakerSharedAudioReceived(AudioRawData* data_) {}
    virtual void onOriginalLanguageMsgReceived(ILiveTranscriptionMessageInfo* messageInfo) {};
    virtual void onChatPrivilegeChanged(IZoomVideoSDKChatHelper* pChatHelper, ZoomVideoSDKChatPrivilegeType privilege) {};
    virtual void onSendFileStatus(IZoomVideoSDKSendFile* file, const FileTransferStatus& status) {};
    virtual void onReceiveFileStatus(IZoomVideoSDKReceiveFile* file, const FileTransferStatus& status) {};
    virtual void onProxyDetectComplete() {};
    virtual void onProxySettingNotification(IZoomVideoSDKProxySettingHandler* handler) {};
    virtual void onSSLCertVerifiedFailNotification(IZoomVideoSDKSSLCertificateInfo* info) {};
    virtual void onUserVideoNetworkStatusChanged(ZoomVideoSDKNetworkStatus status, IZoomVideoSDKUser* pUser) {};
    virtual void onCallCRCDeviceStatusChanged(ZoomVideoSDKCRCCallStatus status) {};
    virtual void onVideoCanvasSubscribeFail(ZoomVideoSDKSubscribeFailReason fail_reason, IZoomVideoSDKUser* pUser, void* handle) {};
    virtual void onShareCanvasSubscribeFail(ZoomVideoSDKSubscribeFailReason fail_reason, IZoomVideoSDKUser* pUser, void* handle) {};
    virtual void onAnnotationHelperCleanUp(IZoomVideoSDKAnnotationHelper* helper) {};
    virtual void onAnnotationPrivilegeChange(IZoomVideoSDKUser* pUser, bool enable) {};
    virtual void onAnnotationHelperActived(void* handle) {};
    virtual void onVideoAlphaChannelStatusChanged(bool isAlphaModeOn) {};
    virtual void onUserManagerChanged(IZoomVideoSDKUser* pUser) {};
    virtual void onUserNameChanged(IZoomVideoSDKUser* pUser) {};
    virtual void onCameraControlRequestResult(IZoomVideoSDKUser* pUser, bool isApproved) {};
    virtual void onCameraControlRequestReceived(IZoomVideoSDKUser* pUser, ZoomVideoSDKCameraControlRequestType requestType, IZoomVideoSDKCameraControlRequestHandler* pCameraControlRequestHandler) {};

    // Video callbacks - try enabling for local video preview
    virtual void onOneWayVideoRawDataReceived(YUVRawDataI420* data_, IZoomVideoSDKUser* pUser) {
        if (data_ && pUser && m_mainWindow) {
            // Check if this is our own video (for preview)
            IZoomVideoSDKSession* session = video_sdk_obj ? video_sdk_obj->getSessionInfo() : nullptr;
            IZoomVideoSDKUser* myself = session ? session->getMyself() : nullptr;

            if (pUser == myself) {
                printf("DEBUG: Received self video frame via callback: %dx%d\n",
                       data_->GetStreamWidth(), data_->GetStreamHeight());

                // Get YUV data pointers
                const char* y_data = data_->GetYBuffer();
                const char* u_data = data_->GetUBuffer();
                const char* v_data = data_->GetVBuffer();

                if (y_data && u_data && v_data) {
                    // Get video dimensions
                    int width = data_->GetStreamWidth();
                    int height = data_->GetStreamHeight();

                    // Calculate strides (assuming standard YUV420 format)
                    int y_stride = width;
                    int u_stride = width / 2;
                    int v_stride = width / 2;

                    // Get video widget
                    QtVideoWidget* videoWidget = m_mainWindow->getSelfVideoWidget();
                    if (videoWidget) {
                        // Create QtVideoRenderer and render the frame
                        QtVideoRenderer* renderer = new QtVideoRenderer(videoWidget);
                        renderer->renderVideoFrame(y_data, u_data, v_data, width, height,
                                                 y_stride, u_stride, v_stride);

                        static int self_frame_count = 0;
                        if (++self_frame_count % 30 == 0) {
                            printf("Processed %d self video frames via callback\n", self_frame_count);
                        }
                    }
                }
            }
        }
    };

    virtual void onMixedVideoRawDataReceived(YUVRawDataI420* data_) {
        if (data_ && m_mainWindow) {
            printf("DEBUG: Received mixed video frame: %dx%d\n", data_->GetStreamWidth(), data_->GetStreamHeight());

            // Get YUV data pointers
            const char* y_data = data_->GetYBuffer();
            const char* u_data = data_->GetUBuffer();
            const char* v_data = data_->GetVBuffer();

            if (y_data && u_data && v_data) {
                // Get video dimensions
                int width = data_->GetStreamWidth();
                int height = data_->GetStreamHeight();

                // Calculate strides (assuming standard YUV420 format)
                int y_stride = width;
                int u_stride = width / 2;
                int v_stride = width / 2;

                // Get video widget
                QtVideoWidget* videoWidget = m_mainWindow->getRemoteVideoWidget();
                if (videoWidget) {
                    // Create QtVideoRenderer and render the frame
                    QtVideoRenderer* renderer = new QtVideoRenderer(videoWidget);
                    renderer->renderVideoFrame(y_data, u_data, v_data, width, height,
                                             y_stride, u_stride, v_stride);

                    static int mixed_frame_count = 0;
                    if (++mixed_frame_count % 30 == 0) {
                        printf("Processed %d mixed video frames\n", mixed_frame_count);
                    }
                }
            }
        }
    };

    // Audio raw data methods
    virtual void onMixedAudioRawDataReceived(AudioRawData* data_) {
        if (data_ && g_audio_playback) {
            printf("Mixed audio received: buffer size %d bytes\n", data_->GetBufferLen());

            // Process mixed audio data here
            char* buffer = data_->GetBuffer();
            if (buffer) {
                // Route received audio to ALSA playback system
                g_audio_playback->playAudio(buffer, data_->GetBufferLen());

                static int audio_frame_count = 0;
                if (++audio_frame_count % 100 == 0) { // Log every 100 frames
                    printf("Processed %d mixed audio frames\n", audio_frame_count);
                }
            }
        }
    };

    virtual void onOneWayAudioRawDataReceived(AudioRawData* data_, IZoomVideoSDKUser* pUser) {
        if (data_ && pUser && g_audio_playback) {
            printf("One-way audio received from %s: buffer size %d bytes\n",
                   pUser->getUserName(), data_->GetBufferLen());

            // Process individual user audio data here
            char* buffer = data_->GetBuffer();
            if (buffer) {
                // Route received audio to ALSA playback system
                g_audio_playback->playAudio(buffer, data_->GetBufferLen());

                static int user_audio_frame_count = 0;
                if (++user_audio_frame_count % 100 == 0) { // Log every 100 frames
                    printf("Processed %d user audio frames from %s\n", user_audio_frame_count, pUser->getUserName());
                }
            }
        }
    };

    virtual void onSharedAudioRawDataReceived(AudioRawData* data_) {
        if (data_ && g_audio_playback) {
            printf("Shared audio received: buffer size %d bytes\n", data_->GetBufferLen());

            // Process shared audio data here (screen sharing audio)
            char* buffer = data_->GetBuffer();
            if (buffer) {
                // Route received audio to ALSA playback system
                g_audio_playback->playAudio(buffer, data_->GetBufferLen());

                static int shared_audio_frame_count = 0;
                if (++shared_audio_frame_count % 100 == 0) { // Log every 100 frames
                    printf("Processed %d shared audio frames\n", shared_audio_frame_count);
                }
            }
        }
    };

private:
    QtMainWindow* m_mainWindow;
};

// Global delegate instance
ZoomVideoSDKDelegate* g_delegate = nullptr;
QtMainWindow* g_mainWindow = nullptr;

QString getSelfDirPath()
{
    char dest[PATH_MAX];
    memset(dest, 0, sizeof(dest)); // readlink does not null terminate!
    if (readlink("/proc/self/exe", dest, PATH_MAX) == -1)
    {
        return QString();
    }

    char* tmp = strrchr(dest, '/');
    if (tmp)
        *tmp = 0;
    printf("getpath\n");
    return QString(dest);
}

void joinVideoSDKSession(const QString& session_name, const QString& session_psw, const QString& session_token)
{
    printf("=== Starting Video SDK Session Join Process (Qt-Free) ===\n");

    // Basic validation
    if (session_name.isEmpty()) {
        printf("ERROR: Session name is empty!\n");
        return;
    }

    if (session_token.isEmpty()) {
        printf("ERROR: Session token is empty!\n");
        return;
    }

    printf("Session Name: %s\n", session_name.toStdString().c_str());

    // Check if SDK is already initialized
    if (!video_sdk_obj) {
        printf("ERROR: SDK not initialized!\n");
        return;
    }

    // If already in a session, leave it first
    if (g_in_session) {
        printf("Leaving current session...\n");
        video_sdk_obj->leaveSession(false);
        g_in_session = false;
    }

    // Temporarily disable the delegate to avoid Qt interference during join
    printf("Temporarily disabling delegate for clean join...\n");
    if (g_delegate) {
        video_sdk_obj->removeListener(dynamic_cast<IZoomVideoSDKDelegate*>(g_delegate));
    }

    // Prepare session context (matching GTK exactly)
    // FIX: Store std::string objects to avoid temporary destruction
    std::string session_name_str = session_name.toStdString();
    std::string session_psw_str = session_psw.toStdString();
    std::string session_token_str = session_token.toStdString();

    ZoomVideoSDKSessionContext session_context;
    session_context.sessionName = session_name_str.c_str();

    if (!session_psw.isEmpty()) {
        session_context.sessionPassword = session_psw_str.c_str();
    }

    session_context.userName = "Linux Qt Bot";
    session_context.token = session_token_str.c_str();
    session_context.videoOption.localVideoOn = false;
    session_context.audioOption.connect = true;
    session_context.audioOption.mute = false;

    // DEBUG: Print all session parameters before joining
    printf("=== SESSION JOIN PARAMETERS ===\n");
    printf("Username: %s\n", session_context.userName);
    printf("Session Name: %s\n", session_context.sessionName);
    printf("Session Password: %s\n", session_context.sessionPassword ? session_context.sessionPassword : "(empty)");
    printf("Token: %.50s...\n", session_context.token); // Truncate token for readability
    printf("Video On: %s\n", session_context.videoOption.localVideoOn ? "true" : "false");
    printf("Audio Connect: %s\n", session_context.audioOption.connect ? "true" : "false");
    printf("Audio Mute: %s\n", session_context.audioOption.mute ? "true" : "false");
    printf("===============================\n");

    // Re-enable the delegate BEFORE joining so we can receive callbacks
    printf("Re-enabling delegate for callback handling...\n");
    if (g_delegate) {
        video_sdk_obj->addListener(dynamic_cast<IZoomVideoSDKDelegate*>(g_delegate));
    }

    printf("Joining session (waiting for callback)...\n");
    IZoomVideoSDKSession* session = video_sdk_obj->joinSession(session_context);

    // Don't check session return value - success/failure comes through callbacks
    printf("Join session request sent - waiting for callback...\n");

    // Update UI to show we're attempting to join
    if (g_mainWindow) {
        QMetaObject::invokeMethod(g_mainWindow, "updateStatus",
            Qt::QueuedConnection, Q_ARG(QString, "Joining session..."));
    }

    printf("=== Session Join Process Complete ===\n");
}

int main(int argc, char* argv[])
{
    printf("=== Starting Qt Video SDK Application ===\n");

    // Check if we have a display available
    const char* display = getenv("DISPLAY");
    const char* qt_platform = getenv("QT_QPA_PLATFORM");

    printf("Display: %s\n", display ? display : "None");
    printf("Qt Platform: %s\n", qt_platform ? qt_platform : "Default");

    QApplication app(argc, argv);
    app.setApplicationName("Zoom Video SDK Qt Demo");
    app.setApplicationVersion("1.0");

    printf("QApplication created successfully\n");

    // Create main window
    printf("Creating QtMainWindow...\n");
    QtMainWindow mainWindow;
    g_mainWindow = &mainWindow;
    printf("QtMainWindow created successfully\n");

    // Only show window if we have a display or are using a GUI platform
    if (display || (qt_platform && strcmp(qt_platform, "offscreen") != 0)) {
        printf("Showing main window...\n");
        mainWindow.show();
        printf("Main window shown successfully\n");
    } else {
        printf("Running in headless mode - not showing GUI\n");
    }

    // Load session parameters from config.json
    QString session_name;
    QString session_psw;
    QString session_token;

    QString self_dir = getSelfDirPath();
    if (!self_dir.isEmpty()) {
        QString config_path = self_dir + "/config.json";
        QFile config_file(config_path);
        if (config_file.open(QIODevice::ReadOnly)) {
            QByteArray config_data = config_file.readAll();
            config_file.close();

            try {
                Json config_json = Json::parse(config_data.toStdString());
                if (!config_json.is_null()) {
                    if (config_json.contains("session_name"))
                        session_name = QString::fromStdString(config_json["session_name"]);
                    if (config_json.contains("session_psw"))
                        session_psw = QString::fromStdString(config_json["session_psw"]);
                    if (config_json.contains("token"))
                        session_token = QString::fromStdString(config_json["token"]);
                }
            } catch (Json::parse_error& ex) {
                printf("Error parsing config.json: %s\n", ex.what());
            }
        } else {
            printf("Config file not found: %s\n", config_path.toStdString().c_str());
        }
    }

    // Pre-fill the UI with loaded values
    if (!session_name.isEmpty()) mainWindow.findChild<QLineEdit*>("sessionNameEdit")->setText(session_name);
    if (!session_psw.isEmpty()) mainWindow.findChild<QLineEdit*>("sessionPasswordEdit")->setText(session_psw);
    if (!session_token.isEmpty()) mainWindow.findChild<QLineEdit*>("signatureEdit")->setText(session_token);

    mainWindow.updateStatus("Qt Video SDK Demo ready - Qt version");

    // Initialize SDK for device enumeration (but don't join session yet)
    printf("Initializing SDK for device enumeration...\n");
    ZoomVideoSDKRawDataMemoryMode heap = ZoomVideoSDKRawDataMemoryMode::ZoomVideoSDKRawDataMemoryModeHeap;
    video_sdk_obj = CreateZoomVideoSDKObj();

    if (video_sdk_obj) {
        ZoomVideoSDKInitParams init_params;
        init_params.domain = "https://zoom.us";
        init_params.enableLog = false;
        init_params.logFilePrefix = "";
        init_params.videoRawDataMemoryMode = ZoomVideoSDKRawDataMemoryModeHeap;
        init_params.shareRawDataMemoryMode = ZoomVideoSDKRawDataMemoryModeHeap;
        init_params.audioRawDataMemoryMode = ZoomVideoSDKRawDataMemoryModeHeap;
        init_params.enableIndirectRawdata = false;

        ZoomVideoSDKErrors err = video_sdk_obj->initialize(init_params);
        if (err == ZoomVideoSDKErrors_Success) {
            printf("SDK initialized for device enumeration\n");

            // Set up delegate once during SDK initialization
            printf("Setting up delegate...\n");
            g_delegate = new ZoomVideoSDKDelegate(&mainWindow);
            video_sdk_obj->addListener(dynamic_cast<IZoomVideoSDKDelegate*>(g_delegate));
            printf("Delegate added successfully\n");

            mainWindow.updateStatus("SDK initialized - populating device lists...");

            // Populate device dropdowns after a short delay
            QTimer::singleShot(500, [&mainWindow]() {
                mainWindow.populateDeviceDropdowns();
                mainWindow.updateStatus("Device lists populated - ready to join session");
            });
        } else {
            printf("Failed to initialize SDK for device enumeration: %d\n", (int)err);
            mainWindow.updateStatus("Failed to initialize SDK for device enumeration");
        }
    } else {
        printf("Failed to create SDK object for device enumeration\n");
        mainWindow.updateStatus("Failed to create SDK object");
    }

    // Manual join only - user must click the join button
    // Auto-join has been disabled

    return app.exec();
}
