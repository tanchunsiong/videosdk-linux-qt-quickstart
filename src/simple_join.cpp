#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <dlfcn.h>

#include "json.hpp"

// Include Zoom SDK headers
#include "helpers/zoom_video_sdk_user_helper_interface.h"
#include "zoom_video_sdk_api.h"
#include "zoom_video_sdk_def.h"
#include "zoom_video_sdk_delegate_interface.h"
#include "zoom_video_sdk_interface.h"
#include "zoom_video_sdk_session_info_interface.h"
#include "zoom_video_sdk_platform.h"

using Json = nlohmann::json;
USING_ZOOM_VIDEO_SDK_NAMESPACE

// Simple delegate for session events
class SimpleDelegate : public IZoomVideoSDKDelegate
{
public:
    virtual void onSessionJoin() {
        std::cout << "=== SUCCESS: Session joined successfully! ===" << std::endl;
    }

    virtual void onSessionLeave() {
        std::cout << "Session left." << std::endl;
    }

    virtual void onSessionLeave(ZoomVideoSDKSessionLeaveReason eReason) {
        std::cout << "Session left with reason: " << (int)eReason << std::endl;
    }

    virtual void onError(ZoomVideoSDKErrors errorCode, int detailErrorCode) {
        std::cout << "Session error - Code: " << errorCode << ", Detail: " << detailErrorCode << std::endl;
    }

    // Stub implementations for other required methods
    virtual void onUserJoin(IZoomVideoSDKUserHelper* pUserHelper, IVideoSDKVector<IZoomVideoSDKUser*>* userList) {}
    virtual void onUserLeave(IZoomVideoSDKUserHelper* pUserHelper, IVideoSDKVector<IZoomVideoSDKUser*>* userList) {}
    virtual void onUserVideoStatusChanged(IZoomVideoSDKVideoHelper* pVideoHelper, IVideoSDKVector<IZoomVideoSDKUser*>* userList) {}
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
    virtual void onCommandChannelConnectResult(bool isSuccess) {}
    virtual void onInviteByPhoneStatus(PhoneStatus status, PhoneFailedReason reason) {}
    virtual void onCalloutJoinSuccess(IZoomVideoSDKUser* pUser, const zchar_t* phoneNumber) {}
    virtual void onCloudRecordingStatus(RecordingStatus status, IZoomVideoSDKRecordingConsentHandler* pHandler) {}
    virtual void onHostAskUnmute() {}
    virtual void onMultiCameraStreamStatusChanged(ZoomVideoSDKMultiCameraStreamStatus status, IZoomVideoSDKUser* pUser, IZoomVideoSDKRawDataPipe* pVideoPipe) {}
    virtual void onMicSpeakerVolumeChanged(unsigned int micVolume, unsigned int speakerVolume) {}
    virtual void onAudioDeviceStatusChanged(ZoomVideoSDKAudioDeviceType type, ZoomVideoSDKAudioDeviceStatus status) {}
    virtual void onTestMicStatusChanged(ZoomVideoSDK_TESTMIC_STATUS status) {}
    virtual void onSelectedAudioDeviceChanged() {}
    virtual void onCameraListChanged() {}
    virtual void onLiveTranscriptionStatus(ZoomVideoSDKLiveTranscriptionStatus status) {}
    virtual void onLiveTranscriptionMsgReceived(const zchar_t* ltMsg, IZoomVideoSDKUser* pUser, ZoomVideoSDKLiveTranscriptionOperationType type) {}
    virtual void onLiveTranscriptionMsgInfoReceived(ILiveTranscriptionMessageInfo* messageInfo) {}
    virtual void onLiveTranscriptionMsgError(ILiveTranscriptionLanguage* spokenLanguage, ILiveTranscriptionLanguage* transcriptLanguage) {}
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
    virtual void onChatMsgDeleteNotification(IZoomVideoSDKChatHelper* pChatHelper, const zchar_t* msgID, ZoomVideoSDKChatMessageDeleteType deleteBy) {}
    virtual void onVirtualSpeakerMixedAudioReceived(AudioRawData* data_) {}
    virtual void onVirtualSpeakerOneWayAudioReceived(AudioRawData* data_, IZoomVideoSDKUser* pUser) {}
    virtual void onVirtualSpeakerSharedAudioReceived(AudioRawData* data_) {}
    virtual void onOriginalLanguageMsgReceived(ILiveTranscriptionMessageInfo* messageInfo) {}
    virtual void onChatPrivilegeChanged(IZoomVideoSDKChatHelper* pChatHelper, ZoomVideoSDKChatPrivilegeType privilege) {}
    virtual void onSendFileStatus(IZoomVideoSDKSendFile* file, const FileTransferStatus& status) {}
    virtual void onReceiveFileStatus(IZoomVideoSDKReceiveFile* file, const FileTransferStatus& status) {}
    virtual void onProxyDetectComplete() {}
    virtual void onProxySettingNotification(IZoomVideoSDKProxySettingHandler* handler) {}
    virtual void onSSLCertVerifiedFailNotification(IZoomVideoSDKSSLCertificateInfo* info) {}
    virtual void onUserVideoNetworkStatusChanged(ZoomVideoSDKNetworkStatus status, IZoomVideoSDKUser* pUser) {}
    virtual void onCallCRCDeviceStatusChanged(ZoomVideoSDKCRCCallStatus status) {}
    virtual void onVideoCanvasSubscribeFail(ZoomVideoSDKSubscribeFailReason fail_reason, IZoomVideoSDKUser* pUser, void* handle) {}
    virtual void onShareCanvasSubscribeFail(ZoomVideoSDKSubscribeFailReason fail_reason, IZoomVideoSDKUser* pUser, void* handle) {}
    virtual void onAnnotationHelperCleanUp(IZoomVideoSDKAnnotationHelper* helper) {}
    virtual void onAnnotationPrivilegeChange(IZoomVideoSDKUser* pUser, bool enable) {}
    virtual void onAnnotationHelperActived(void* handle) {}
    virtual void onVideoAlphaChannelStatusChanged(bool isAlphaModeOn) {}
    virtual void onUserManagerChanged(IZoomVideoSDKUser* pUser) {}
    virtual void onUserNameChanged(IZoomVideoSDKUser* pUser) {}
    virtual void onCameraControlRequestResult(IZoomVideoSDKUser* pUser, bool isApproved) {}
    virtual void onCameraControlRequestReceived(IZoomVideoSDKUser* pUser, ZoomVideoSDKCameraControlRequestType requestType, IZoomVideoSDKCameraControlRequestHandler* pCameraControlRequestHandler) {}
    virtual void onMixedAudioRawDataReceived(AudioRawData* data_) {}
    virtual void onOneWayAudioRawDataReceived(AudioRawData* data_, IZoomVideoSDKUser* pUser) {}
    virtual void onSharedAudioRawDataReceived(AudioRawData* data_) {}
};

int main(int argc, char* argv[])
{
    std::cout << "=== Simple Zoom SDK Session Join Test ===" << std::endl;

    // Load config from config.json
    std::string session_name;
    std::string session_psw;
    std::string session_token;

    try {
        std::ifstream config_file("config.json");
        if (config_file.is_open()) {
            Json config_json;
            config_file >> config_json;

            if (config_json.contains("session_name"))
                session_name = config_json["session_name"];
            if (config_json.contains("session_psw"))
                session_psw = config_json["session_psw"];
            if (config_json.contains("token"))
                session_token = config_json["token"];

            std::cout << "Loaded config:" << std::endl;
            std::cout << "  Session: " << session_name << std::endl;
            std::cout << "  Token length: " << session_token.length() << std::endl;
        } else {
            std::cerr << "ERROR: Could not open config.json" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR parsing config.json: " << e.what() << std::endl;
        return 1;
    }

    if (session_name.empty() || session_token.empty()) {
        std::cerr << "ERROR: Missing session_name or token in config.json" << std::endl;
        return 1;
    }

    // Create SDK instance
    std::cout << "Creating Zoom Video SDK instance..." << std::endl;
    IZoomVideoSDK* video_sdk_obj = CreateZoomVideoSDKObj();
    if (!video_sdk_obj) {
        std::cerr << "ERROR: Failed to create Video SDK object" << std::endl;
        return 1;
    }

    // Initialize SDK
    std::cout << "Initializing Video SDK..." << std::endl;
    ZoomVideoSDKInitParams init_params;
    init_params.domain = "https://zoom.us";
    init_params.enableLog = false;
    init_params.logFilePrefix = "";
    init_params.videoRawDataMemoryMode = ZoomVideoSDKRawDataMemoryModeHeap;
    init_params.shareRawDataMemoryMode = ZoomVideoSDKRawDataMemoryModeHeap;
    init_params.audioRawDataMemoryMode = ZoomVideoSDKRawDataMemoryModeHeap;
    init_params.enableIndirectRawdata = false;

    ZoomVideoSDKErrors err = video_sdk_obj->initialize(init_params);
    if (err != ZoomVideoSDKErrors_Success) {
        std::cerr << "ERROR: Failed to initialize Video SDK, error: " << (int)err << std::endl;
        return 1;
    }

    // Set up delegate
    std::cout << "Setting up delegate..." << std::endl;
    SimpleDelegate* delegate = new SimpleDelegate();
    video_sdk_obj->addListener(delegate);

    // Prepare session context
    std::cout << "Preparing session context..." << std::endl;
    ZoomVideoSDKSessionContext session_context;
    session_context.sessionName = session_name.c_str();
    session_context.userName = "Simple Linux Bot";
    session_context.token = session_token.c_str();

    if (!session_psw.empty()) {
        session_context.sessionPassword = session_psw.c_str();
    }

    // Enable audio and video
    session_context.videoOption.localVideoOn = true;
    session_context.audioOption.connect = true;
    session_context.audioOption.mute = false;

    // Join session
    std::cout << "Joining session: " << session_name << std::endl;
    IZoomVideoSDKSession* session = video_sdk_obj->joinSession(session_context);

    if (!session) {
        std::cerr << "ERROR: Failed to join session" << std::endl;
        video_sdk_obj->cleanup();
        DestroyZoomVideoSDKObj();
        return 1;
    }

    // Wait for session to establish or fail
    std::cout << "Waiting for session to establish..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Clean up
    std::cout << "Cleaning up..." << std::endl;
    video_sdk_obj->leaveSession(false);
    video_sdk_obj->cleanup();
    DestroyZoomVideoSDKObj();

    std::cout << "=== Test completed ===" << std::endl;
    return 0;
}
