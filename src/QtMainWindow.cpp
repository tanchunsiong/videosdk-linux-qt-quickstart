#include "QtMainWindow.h"
#include "QtVideoWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTimer>
#include <QDebug>

// Include Zoom SDK headers
#include "zoom_video_sdk_def.h"
#include "zoom_video_sdk_interface.h"
#include "helpers/zoom_video_sdk_video_helper_interface.h"
#include "helpers/zoom_video_sdk_audio_helper_interface.h"
#include "zoom_video_sdk_session_info_interface.h"
#include "helpers/zoom_video_sdk_user_helper_interface.h"

// Use Zoom SDK namespace
USING_ZOOM_VIDEO_SDK_NAMESPACE

// Include external function
extern void joinVideoSDKSession(const QString& session_name, const QString& session_psw, const QString& session_token);

// External variables
extern IZoomVideoSDK* video_sdk_obj;
extern bool g_in_session;
extern bool g_audio_muted;
extern bool g_video_muted;

QtMainWindow::QtMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_selfVideoEnabled(false)
    , m_remoteVideoEnabled(true)
{
    setWindowTitle("Zoom Video SDK Qt Demo");
    setMinimumSize(800, 600);

    // Create central widget
    QWidget* centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Create session info group
    QGroupBox* sessionGroup = new QGroupBox("Session Information");
    QFormLayout* sessionLayout = new QFormLayout(sessionGroup);

    m_usernameEdit = new QLineEdit("Linux Qt User");
    m_sessionNameEdit = new QLineEdit();
    m_sessionNameEdit->setObjectName("sessionNameEdit"); // For finding in main
    m_sessionPasswordEdit = new QLineEdit();
    m_sessionPasswordEdit->setObjectName("sessionPasswordEdit");
    m_signatureEdit = new QLineEdit();
    m_signatureEdit->setObjectName("signatureEdit");

    sessionLayout->addRow("Username:", m_usernameEdit);
    sessionLayout->addRow("Session Name:", m_sessionNameEdit);
    sessionLayout->addRow("Password:", m_sessionPasswordEdit);
    sessionLayout->addRow("Signature:", m_signatureEdit);

    mainLayout->addWidget(sessionGroup);

    // Create device selection group
    QGroupBox* deviceGroup = new QGroupBox("Device Settings");
    QFormLayout* deviceLayout = new QFormLayout(deviceGroup);

    m_cameraCombo = new QComboBox();
    m_microphoneCombo = new QComboBox();
    m_speakerCombo = new QComboBox();
    m_resolutionCombo = new QComboBox();

    // Populate resolution combo
    m_resolutionCombo->addItem("90P (Recommended)", ZOOMVIDEOSDK::ZoomVideoSDKResolution_90P);
    m_resolutionCombo->addItem("180P", ZOOMVIDEOSDK::ZoomVideoSDKResolution_180P);
    m_resolutionCombo->addItem("360P", ZOOMVIDEOSDK::ZoomVideoSDKResolution_360P);
    m_resolutionCombo->addItem("720P", ZOOMVIDEOSDK::ZoomVideoSDKResolution_720P);
    m_resolutionCombo->addItem("1080P", ZOOMVIDEOSDK::ZoomVideoSDKResolution_1080P);
    m_resolutionCombo->setCurrentIndex(0); // Default to 90P

    deviceLayout->addRow("Camera:", m_cameraCombo);
    deviceLayout->addRow("Microphone:", m_microphoneCombo);
    deviceLayout->addRow("Speaker:", m_speakerCombo);
    deviceLayout->addRow("Resolution:", m_resolutionCombo);

    mainLayout->addWidget(deviceGroup);

    // Create control buttons
    QHBoxLayout* controlLayout = new QHBoxLayout();

    m_joinButton = new QPushButton("Join Session");
    m_leaveButton = new QPushButton("Leave Session");
    m_muteAudioButton = new QPushButton("Mute Audio");
    m_selfVideoButton = new QPushButton("Start Video");

    controlLayout->addWidget(m_joinButton);
    controlLayout->addWidget(m_leaveButton);
    controlLayout->addWidget(m_muteAudioButton);
    controlLayout->addWidget(m_selfVideoButton);

    mainLayout->addLayout(controlLayout);

    // Create status display
    QGroupBox* statusGroup = new QGroupBox("Status");
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);

    m_statusText = new QTextEdit();
    m_statusText->setMaximumHeight(100);
    m_statusText->setReadOnly(true);

    statusLayout->addWidget(m_statusText);
    mainLayout->addWidget(statusGroup);

    // Create video display area
    QGroupBox* videoGroup = new QGroupBox("Video Display");
    QHBoxLayout* videoLayout = new QHBoxLayout(videoGroup);

    // Self video widget
    QVBoxLayout* selfVideoLayout = new QVBoxLayout();
    selfVideoLayout->addWidget(new QLabel("Self Video"));
    m_selfVideoWidget = new QtVideoWidget();
    selfVideoLayout->addWidget(m_selfVideoWidget);

    // Remote video widget
    QVBoxLayout* remoteVideoLayout = new QVBoxLayout();
    remoteVideoLayout->addWidget(new QLabel("Remote Video"));
    m_remoteVideoWidget = new QtVideoWidget();
    remoteVideoLayout->addWidget(m_remoteVideoWidget);

    videoLayout->addLayout(selfVideoLayout);
    videoLayout->addLayout(remoteVideoLayout);

    mainLayout->addWidget(videoGroup);

    // Connect signals
    connect(m_joinButton, &QPushButton::clicked, this, &QtMainWindow::onJoinSessionClicked);
    connect(m_leaveButton, &QPushButton::clicked, this, &QtMainWindow::onLeaveSessionClicked);
    connect(m_muteAudioButton, &QPushButton::clicked, this, &QtMainWindow::onMuteAudioClicked);
    connect(m_selfVideoButton, &QPushButton::clicked, this, &QtMainWindow::onSelfVideoClicked);

    connect(m_cameraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QtMainWindow::onCameraChanged);
    connect(m_microphoneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QtMainWindow::onMicrophoneChanged);
    connect(m_speakerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QtMainWindow::onSpeakerChanged);
    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QtMainWindow::onResolutionChanged);

    // Initialize UI state
    updateButtonStates();
}

QtMainWindow::~QtMainWindow()
{
}

void QtMainWindow::updateStatus(const QString& message)
{
    m_statusText->append(message);
    // Scroll to bottom
    QTextCursor cursor = m_statusText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_statusText->setTextCursor(cursor);
}

void QtMainWindow::updateButtonStates()
{
    printf("DEBUG: updateButtonStates() called, g_in_session = %s\n", g_in_session ? "true" : "false");

    m_joinButton->setEnabled(!g_in_session);
    m_leaveButton->setEnabled(g_in_session);
    m_muteAudioButton->setEnabled(g_in_session);
    m_selfVideoButton->setEnabled(g_in_session);

    m_muteAudioButton->setText(g_audio_muted ? "Unmute Audio" : "Mute Audio");
    m_selfVideoButton->setText(m_selfVideoEnabled ? "Stop Video" : "Start Video");

    printf("DEBUG: Buttons updated - Join:%s, Leave:%s, Mute:%s, Video:%s\n",
           m_joinButton->isEnabled() ? "enabled" : "disabled",
           m_leaveButton->isEnabled() ? "enabled" : "disabled",
           m_muteAudioButton->isEnabled() ? "enabled" : "disabled",
           m_selfVideoButton->isEnabled() ? "enabled" : "disabled");
}

void QtMainWindow::populateDeviceDropdowns()
{
    if (!video_sdk_obj) return;

    // Populate camera dropdown
    m_cameraCombo->clear();
    IZoomVideoSDKVideoHelper* videoHelper = video_sdk_obj->getVideoHelper();
    if (videoHelper) {
        IVideoSDKVector<IZoomVideoSDKCameraDevice*>* cameraList = videoHelper->getCameraList();
        if (cameraList) {
            int count = cameraList->GetCount();
            for (int i = 0; i < count; i++) {
                IZoomVideoSDKCameraDevice* camera = cameraList->GetItem(i);
                if (camera) {
                    QString cameraName = camera->getDeviceName();
                    QString cameraId = camera->getDeviceId();
                    m_cameraCombo->addItem(cameraName, cameraId);
                }
            }
            if (count > 0) m_cameraCombo->setCurrentIndex(0);
        }
    }

    // Populate microphone dropdown
    m_microphoneCombo->clear();
    IZoomVideoSDKAudioHelper* audioHelper = video_sdk_obj->getAudioHelper();
    if (audioHelper) {
        IVideoSDKVector<IZoomVideoSDKMicDevice*>* micList = audioHelper->getMicList();
        if (micList) {
            int count = micList->GetCount();
            for (int i = 0; i < count; i++) {
                IZoomVideoSDKMicDevice* mic = micList->GetItem(i);
                if (mic) {
                    QString micName = mic->getDeviceName();
                    QString micId = mic->getDeviceId();
                    m_microphoneCombo->addItem(micName, micId);
                }
            }
            if (count > 0) m_microphoneCombo->setCurrentIndex(0);
        }
    }

    // Populate speaker dropdown
    m_speakerCombo->clear();
    if (audioHelper) {
        IVideoSDKVector<IZoomVideoSDKSpeakerDevice*>* speakerList = audioHelper->getSpeakerList();
        if (speakerList) {
            int count = speakerList->GetCount();
            for (int i = 0; i < count; i++) {
                IZoomVideoSDKSpeakerDevice* speaker = speakerList->GetItem(i);
                if (speaker) {
                    QString speakerName = speaker->getDeviceName();
                    QString speakerId = speaker->getDeviceId();
                    m_speakerCombo->addItem(speakerName, speakerId);
                }
            }
            if (count > 0) m_speakerCombo->setCurrentIndex(0);
        }
    }
}

void QtMainWindow::onJoinSessionClicked()
{
    QString username = m_usernameEdit->text();
    QString sessionName = m_sessionNameEdit->text();
    QString sessionPassword = m_sessionPasswordEdit->text();
    QString signature = m_signatureEdit->text();

    if (username.isEmpty() || sessionName.isEmpty() || signature.isEmpty()) {
        updateStatus("Please fill in all required fields");
        return;
    }

    updateStatus("Joining session...");

    // Join the session
    joinVideoSDKSession(sessionName, sessionPassword, signature);

    // Populate device dropdowns after SDK initialization
    QTimer::singleShot(1000, this, &QtMainWindow::populateDeviceDropdowns);
}

void QtMainWindow::onLeaveSessionClicked()
{
    if (video_sdk_obj && g_in_session) {
        video_sdk_obj->leaveSession(false);
        g_in_session = false;
        updateButtonStates();
        updateStatus("Left session");
    }
}

void QtMainWindow::onMuteAudioClicked()
{
    printf("DEBUG: onMuteAudioClicked() called, current state: %s\n", g_audio_muted ? "muted" : "unmuted");

    if (video_sdk_obj && g_in_session) {
        IZoomVideoSDKAudioHelper* audioHelper = video_sdk_obj->getAudioHelper();
        if (audioHelper) {
            IZoomVideoSDKSession* session = video_sdk_obj->getSessionInfo();
            if (session) {
                IZoomVideoSDKUser* currentUser = session->getMyself();
                if (currentUser) {
                    printf("DEBUG: Current user obtained: %s\n", currentUser->getUserName());

                    if (g_audio_muted) {
                        printf("DEBUG: Calling audioHelper->unMuteAudio()\n");
                        ZoomVideoSDKErrors err = audioHelper->unMuteAudio(currentUser);
                        printf("DEBUG: unMuteAudio() returned: %d\n", (int)err);
                        if (err == ZoomVideoSDKErrors_Success) {
                            g_audio_muted = false;
                            updateStatus("Audio unmuted");
                        } else {
                            updateStatus("Failed to unmute audio");
                        }
                    } else {
                        printf("DEBUG: Calling audioHelper->muteAudio()\n");
                        ZoomVideoSDKErrors err = audioHelper->muteAudio(currentUser);
                        printf("DEBUG: muteAudio() returned: %d\n", (int)err);
                        if (err == ZoomVideoSDKErrors_Success) {
                            g_audio_muted = true;
                            updateStatus("Audio muted");
                        } else {
                            updateStatus("Failed to mute audio");
                        }
                    }
                    updateButtonStates();
                } else {
                    printf("DEBUG: ERROR - Current user is NULL!\n");
                    updateStatus("Failed to get current user");
                }
            } else {
                printf("DEBUG: ERROR - Session is NULL!\n");
                updateStatus("Session not available");
            }
        } else {
            printf("DEBUG: ERROR - Audio helper is NULL!\n");
            updateStatus("Audio helper not available");
        }
    } else {
        printf("DEBUG: Not in session or SDK not initialized\n");
        updateStatus("Not in session or SDK not initialized");
    }
}

void QtMainWindow::onSelfVideoClicked()
{
    printf("DEBUG: onSelfVideoClicked() called, current state: %s\n", m_selfVideoEnabled ? "enabled" : "disabled");

    m_selfVideoEnabled = !m_selfVideoEnabled;
    updateButtonStates();

    if (video_sdk_obj && g_in_session) {
        IZoomVideoSDKVideoHelper* videoHelper = video_sdk_obj->getVideoHelper();
        if (videoHelper) {
            if (m_selfVideoEnabled) {
                printf("DEBUG: Calling videoHelper->startVideo()\n");
                ZoomVideoSDKErrors err = videoHelper->startVideo();
                printf("DEBUG: videoHelper->startVideo() returned: %d\n", (int)err);
                updateStatus("Video started");
            } else {
                printf("DEBUG: Calling videoHelper->stopVideo()\n");
                ZoomVideoSDKErrors err = videoHelper->stopVideo();
                printf("DEBUG: videoHelper->stopVideo() returned: %d\n", (int)err);
                updateStatus("Video stopped");
            }
        } else {
            printf("DEBUG: videoHelper is NULL!\n");
            updateStatus("Video helper not available");
        }
    } else {
        printf("DEBUG: video_sdk_obj=%p, g_in_session=%s\n", video_sdk_obj, g_in_session ? "true" : "false");
        updateStatus("Not in session or SDK not initialized");
    }
}

void QtMainWindow::onCameraChanged()
{
    if (!video_sdk_obj || !g_in_session) return;

    QString selectedId = m_cameraCombo->currentData().toString();
    if (!selectedId.isEmpty()) {
        IZoomVideoSDKVideoHelper* videoHelper = video_sdk_obj->getVideoHelper();
        if (videoHelper) {
            bool result = videoHelper->selectCamera(selectedId.toStdString().c_str());
            updateStatus(QString("Camera changed to: %1 (%2)")
                        .arg(m_cameraCombo->currentText())
                        .arg(result ? "success" : "failed"));
        }
    }
}

void QtMainWindow::onMicrophoneChanged()
{
    if (!video_sdk_obj || !g_in_session) return;

    QString selectedId = m_microphoneCombo->currentData().toString();
    QString selectedText = m_microphoneCombo->currentText();
    if (!selectedId.isEmpty() && !selectedText.isEmpty()) {
        IZoomVideoSDKAudioHelper* audioHelper = video_sdk_obj->getAudioHelper();
        if (audioHelper) {
            ZOOMVIDEOSDK::ZoomVideoSDKErrors err = audioHelper->selectMic(selectedId.toStdString().c_str(),
                                                                          selectedText.toStdString().c_str());
            updateStatus(QString("Microphone changed to: %1 (result: %2)")
                        .arg(selectedText).arg((int)err));
        }
    }
}

void QtMainWindow::onSpeakerChanged()
{
    if (!video_sdk_obj || !g_in_session) return;

    QString selectedId = m_speakerCombo->currentData().toString();
    QString selectedText = m_speakerCombo->currentText();
    if (!selectedId.isEmpty() && !selectedText.isEmpty()) {
        IZoomVideoSDKAudioHelper* audioHelper = video_sdk_obj->getAudioHelper();
        if (audioHelper) {
            ZOOMVIDEOSDK::ZoomVideoSDKErrors err = audioHelper->selectSpeaker(selectedId.toStdString().c_str(),
                                                                              selectedText.toStdString().c_str());
            updateStatus(QString("Speaker changed to: %1 (result: %2)")
                        .arg(selectedText).arg((int)err));
        }
    }
}

void QtMainWindow::onResolutionChanged()
{
    if (!video_sdk_obj || !g_in_session) return;

    int resolutionValue = m_resolutionCombo->currentData().toInt();
    ZOOMVIDEOSDK::ZoomVideoSDKResolution resolution = static_cast<ZOOMVIDEOSDK::ZoomVideoSDKResolution>(resolutionValue);

    updateStatus(QString("Resolution changed to: %1 (enum value: %2)")
                .arg(m_resolutionCombo->currentText()).arg((int)resolution));
}
