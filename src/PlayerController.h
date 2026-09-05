#pragma once

#include "MediaFile.h"

#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QWindow>
#include <QStringList>

#include <vlc/vlc.h>

#include <memory>

class VideoSurfaceWindow;

class PlayerController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentMediaChanged)
    Q_PROPERTY(QString title READ title NOTIFY currentMediaChanged)
    Q_PROPERTY(MediaKind mediaKind READ mediaKind NOTIFY currentMediaChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY currentMediaChanged)
    Q_PROPERTY(bool isAudio READ isAudio NOTIFY currentMediaChanged)
    Q_PROPERTY(bool isVideo READ isVideo NOTIFY currentMediaChanged)
    Q_PROPERTY(bool isImage READ isImage NOTIFY currentMediaChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY seekableChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QVariantList chapterPositions READ chapterPositions NOTIFY chaptersChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QUrl imageSource READ imageSource NOTIFY imageSourceChanged)
    Q_PROPERTY(bool canPreviousImage READ canPreviousImage NOTIFY imageNavigationChanged)
    Q_PROPERTY(bool canNextImage READ canNextImage NOTIFY imageNavigationChanged)
    Q_PROPERTY(int imageIndex READ imageIndex NOTIFY imageNavigationChanged)
    Q_PROPERTY(int imageCount READ imageCount NOTIFY imageNavigationChanged)
    Q_PROPERTY(bool canPreviousMedia READ canPreviousMedia NOTIFY mediaNavigationChanged)
    Q_PROPERTY(bool canNextMedia READ canNextMedia NOTIFY mediaNavigationChanged)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY tracksChanged)
    Q_PROPERTY(int activeAudioTrack READ activeAudioTrack NOTIFY activeTracksChanged)
    Q_PROPERTY(int activeSubtitleTrack READ activeSubtitleTrack NOTIFY activeTracksChanged)
    Q_PROPERTY(QVariantList audioOutputDevices READ audioOutputDevices
               NOTIFY audioOutputDevicesChanged)
    Q_PROPERTY(QString selectedAudioOutputDeviceKey READ selectedAudioOutputDeviceKey
               NOTIFY selectedAudioOutputDeviceChanged)
    Q_PROPERTY(bool resumeAvailable READ resumeAvailable NOTIFY resumeChanged)
    Q_PROPERTY(QString resumeTitle READ resumeTitle NOTIFY resumeChanged)
    Q_PROPERTY(qint64 resumePosition READ resumePosition NOTIFY resumeChanged)
    Q_PROPERTY(QWindow *videoWindow READ videoWindow CONSTANT)

public:
    enum MediaKind { NoMedia, Audio, Video, Image };
    Q_ENUM(MediaKind)

    explicit PlayerController(QObject *parent = nullptr);
    ~PlayerController() override;

    PlayerController(const PlayerController &) = delete;
    PlayerController &operator=(const PlayerController &) = delete;

    QString currentFilePath() const;
    QString title() const;
    MediaKind mediaKind() const;
    bool hasMedia() const;
    bool isAudio() const;
    bool isVideo() const;
    bool isImage() const;
    bool playing() const;
    bool loading() const;
    bool seekable() const;
    qint64 position() const;
    qint64 duration() const;
    QVariantList chapterPositions() const;
    int volume() const;
    bool muted() const;
    QString errorMessage() const;
    QUrl imageSource() const;
    bool canPreviousImage() const;
    bool canNextImage() const;
    int imageIndex() const;
    int imageCount() const;
    bool canPreviousMedia() const;
    bool canNextMedia() const;
    QVariantList audioTracks() const;
    QVariantList subtitleTracks() const;
    int activeAudioTrack() const;
    int activeSubtitleTrack() const;
    QVariantList audioOutputDevices() const;
    QString selectedAudioOutputDeviceKey() const;
    bool resumeAvailable() const;
    QString resumeTitle() const;
    qint64 resumePosition() const;
    QWindow *videoWindow() const;

    Q_INVOKABLE bool openFile(const QString &path);
    Q_INVOKABLE bool openFolder(const QString &path);
    Q_INVOKABLE bool openUrl(const QUrl &url);
    Q_INVOKABLE bool openUrls(const QList<QUrl> &urls);
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void setVolume(int volume);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void previousImage();
    Q_INVOKABLE void nextImage();
    Q_INVOKABLE void previousMedia();
    Q_INVOKABLE void nextMedia();
    Q_INVOKABLE bool addExternalSubtitle(const QUrl &url);
    Q_INVOKABLE void selectAudioTrack(int id);
    Q_INVOKABLE void selectSubtitleTrack(int id);
    Q_INVOKABLE void refreshTracks();
    Q_INVOKABLE void refreshAudioOutputDevices();
    Q_INVOKABLE void selectAudioOutputDevice(const QString &deviceKey);
    Q_INVOKABLE bool resumeLastVideo();
    Q_INVOKABLE void dismissResume();
    Q_INVOKABLE void clearError();

signals:
    void currentMediaChanged();
    void playingChanged();
    void loadingChanged();
    void seekableChanged();
    void positionChanged();
    void durationChanged();
    void chaptersChanged();
    void volumeChanged();
    void mutedChanged();
    void errorMessageChanged();
    void imageSourceChanged();
    void imageNavigationChanged();
    void mediaNavigationChanged();
    void tracksChanged();
    void activeTracksChanged();
    void audioOutputDevicesChanged();
    void selectedAudioOutputDeviceChanged();
    void resumeChanged();
    void fullscreenToggleRequested();
    void mediaSurfaceClicked();
    void mediaSurfaceActivity();

private:
    static void vlcEventCallback(const libvlc_event_t *event, void *userData);

    bool openFileInternal(const QString &path, bool automaticAdvance);
    bool openRecursiveQueueItem(qsizetype index, int direction, bool automaticAdvance);
    bool openImage(const QString &path);
    bool openPlayableMedia(const QString &path, veylo::MediaKind kind, bool automaticAdvance);
    bool ensureMediaEngine();
    void attachVlcEvents();
    void detachVlcEvents();
    void attachVideoOutput();
    void handleVlcEvent(const libvlc_event_t &event);
    void handlePlaybackError();
    void advanceAfterPlayback();
    void loadResumeState();
    void rememberOpenedVideo(const QString &path);
    void persistCurrentVideoProgress(bool force = false);
    void applyPendingResumePosition();
    void applyPendingSeekPosition();
    void applyPreferredAudioOutput(bool selectModule);
    void rememberAudioTrackPreference(int id);
    void rememberSubtitleTrackPreference(int id);
    bool applyFolderTrackPreferences();
    void markCurrentVideoFinished();
    void clearRecursiveQueue();
    void updateImageNavigation();
    void updateMediaNavigation();
    void refreshChapters();
    bool goToAdjacentChapter(int direction);
    void resetTimeline();
    void setError(const QString &message);
    void setLoading(bool loading);
    void setPlaying(bool playing);
    void setSeekable(bool seekable);
    void setCurrentKind(veylo::MediaKind kind);
    static MediaKind toPublicKind(veylo::MediaKind kind);
    static QString displayTrackName(const char *name, const QString &fallback);

    std::unique_ptr<VideoSurfaceWindow> videoSurface_;
    libvlc_instance_t *vlcInstance_ = nullptr;
    libvlc_media_player_t *mediaPlayer_ = nullptr;
    QString currentFilePath_;
    QString title_;
    veylo::MediaKind currentKind_ = veylo::MediaKind::None;
    bool playing_ = false;
    bool loading_ = false;
    bool seekable_ = false;
    qint64 position_ = 0;
    qint64 duration_ = 0;
    QVariantList chapterPositions_;
    int volume_ = 80;
    bool muted_ = false;
    QString errorMessage_;
    quint64 imageRevision_ = 0;
    bool canPreviousImage_ = false;
    bool canNextImage_ = false;
    int imageIndex_ = 0;
    int imageCount_ = 0;
    bool canPreviousMedia_ = false;
    bool canNextMedia_ = false;
    QVariantList audioTracks_;
    QVariantList subtitleTracks_;
    int activeAudioTrack_ = -1;
    int activeSubtitleTrack_ = -1;
    QVariantList audioOutputDevices_;
    QString selectedAudioOutputDeviceKey_;
    QString preferredAudioOutputModule_;
    QString preferredAudioOutputDeviceId_;
    bool hasPreferredAudioOutput_ = false;
    bool pauseAfterAudioOutputRestart_ = false;
    QString externalSubtitlePath_;
    bool audioPreferenceApplied_ = false;
    bool subtitlePreferenceApplied_ = false;
    bool automaticAdvance_ = false;
    bool ended_ = false;
    int consecutiveAdvanceFailures_ = 0;
    QStringList recursiveQueue_;
    qsizetype recursiveQueueIndex_ = -1;
    veylo::MediaKind recursiveQueueKind_ = veylo::MediaKind::None;
    bool resumeAvailable_ = false;
    QString resumeFilePath_;
    QString resumeTitle_;
    qint64 resumePosition_ = 0;
    qint64 pendingResumePosition_ = -1;
    qint64 pendingSeekPosition_ = -1;
    qint64 lastPersistedPosition_ = -1;
};
