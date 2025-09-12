#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QGroupBox>

class QtVideoWidget;

class QtMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QtMainWindow(QWidget* parent = nullptr);
    ~QtMainWindow();

    void updateButtonStates();
    void populateDeviceDropdowns();

    // Video widget accessors for delegate
    QtVideoWidget* getSelfVideoWidget() { return m_selfVideoWidget; }
    QtVideoWidget* getRemoteVideoWidget() { return m_remoteVideoWidget; }

public slots:
    void updateStatus(const QString& message);

private slots:
    void onJoinSessionClicked();
    void onLeaveSessionClicked();
    void onMuteAudioClicked();
    void onSelfVideoClicked();
    void onCameraChanged();
    void onMicrophoneChanged();
    void onSpeakerChanged();
    void onResolutionChanged();

private:
    // UI Components
    QLineEdit* m_usernameEdit;
    QLineEdit* m_sessionNameEdit;
    QLineEdit* m_sessionPasswordEdit;
    QLineEdit* m_signatureEdit;

    QPushButton* m_joinButton;
    QPushButton* m_leaveButton;
    QPushButton* m_muteAudioButton;
    QPushButton* m_selfVideoButton;

    QComboBox* m_cameraCombo;
    QComboBox* m_microphoneCombo;
    QComboBox* m_speakerCombo;
    QComboBox* m_resolutionCombo;

    QTextEdit* m_statusText;

    QtVideoWidget* m_selfVideoWidget;
    QtVideoWidget* m_remoteVideoWidget;

    // Video control variables
    bool m_selfVideoEnabled;
    bool m_remoteVideoEnabled;
};
