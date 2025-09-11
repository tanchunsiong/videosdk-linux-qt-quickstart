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
    m_muteVideoButton = new QPushButton("Mute Video");

    controlLayout->addWidget(m_joinButton);
    controlLayout->addWidget(m_leaveButton);
    controlLayout->addWidget(m_muteAudioButton);
    controlLayout->addWidget(m_muteVideoButton);

    mainLayout->addLayout(controlLayout);

    // Create video control buttons
    QHBoxLayout* videoControlLayout = new QHBoxLayout();

    m_selfVideoButton = new QPushButton("Start Self Video");
    m_remoteVideoButton = new QPushButton("Stop Remote Video");

    videoControlLayout->addWidget(m_selfVideoButton);
    videoControlLayout->addWidget(m_remoteVideoButton);

    mainLayout->addLayout(videoControlLayout);

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
    connect(m_muteVideoButton, &QPushButton::clicked, this, &QtMainWindow::onMuteVideoClicked);
    connect(m_selfVideoButton, &QPushButton::clicked, this, &QtMainWindow::onSelfVideoClicked);
    connect(m_remoteVideoButton, &QPushButton::clicked, this, &QtMainWindow::onRemoteVideoClicked);

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
    m_joinButton->setEnabled(!g_in_session);
    m_leaveButton->setEnabled(g_in_session);
    m_muteAudioButton->setEnabled(g_in_session);
    m_muteVideoButton->setEnabled(g_in_session);
    m_selfVideoButton->setEnabled(g_in_session);
    m_remoteVideoButton->setEnabled(g_in_session);

    m_muteAudioButton->setText(g_audio_muted ? "Unmute Audio" : "Mute Audio");
    m_muteVideoButton->setText(g_video_muted ? "Unmute Video" : "Mute Video");
    m_selfVideoButton->setText(m_selfVideoEnabled ? "Stop Self Video" : "Start Self Video");
    m_remoteVideoButton->setText(m_remoteVideoEnabled ? "Stop Remote Video" : "Start Remote Video");
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
    if (video_sdk_obj && g_in_session) {
        IZoomVideoSDKAudioHelper* audioHelper = video_sdk_obj->getAudioHelper();
        if (audioHelper) {
            IZoomVideoSDKSession* session = video_sdk_obj->getSessionInfo();
            if (session) {
                IZoomVideoSDKUser* currentUser = session->getMyself();
                if (currentUser) {
                    if (g_audio_muted) {
                        audioHelper->unMuteAudio(currentUser);
                    } else {
                        audioHelper->muteAudio(currentUser);
                    }
                    g_audio_muted = !g_audio_muted;
                    updateButtonStates();
                }
            }
        }
    }
}

void QtMainWindow::onMuteVideoClicked()
{
    if (video_sdk_obj && g_in_session) {
        IZoomVideoSDKVideoHelper* videoHelper = video_sdk_obj->getVideoHelper();
        if (videoHelper) {
            if (g_video_muted) {
                videoHelper->startVideo();
            } else {
                videoHelper->stopVideo();
            }
            g_video_muted = !g_video_muted;
            updateButtonStates();
        }
    }
}

void QtMainWindow::onSelfVideoClicked()
{
    m_selfVideoEnabled = !m_selfVideoEnabled;
    updateButtonStates();

    if (video_sdk_obj && g_in_session) {
        IZoomVideoSDKVideoHelper* videoHelper = video_sdk_obj->getVideoHelper();
        if (videoHelper) {
            if (m_selfVideoEnabled) {
                videoHelper->startVideo();
                updateStatus("Self video started");
            } else {
                videoHelper->stopVideo();
                updateStatus("Self video stopped");
            }
        }
    }
}

void QtMainWindow::onRemoteVideoClicked()
{
    m_remoteVideoEnabled = !m_remoteVideoEnabled;
    updateButtonStates();

    updateStatus(m_remoteVideoEnabled ? "Remote video enabled" : "Remote video disabled");
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
