#include "PlayerController.h"

#include "ChapterNavigation.h"
#include "FolderSequence.h"
#include "NaturalSort.h"
#include "PlaybackResume.h"
#include "TrackPreference.h"
#include "VideoSurfaceWindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QMetaObject>
#include <QSettings>
#include <QSet>
#include <QTimer>
#include <QWindow>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <optional>
#include <utility>

namespace {

constexpr std::array<libvlc_event_e, 11> observedEvents = {
    libvlc_MediaPlayerOpening, libvlc_MediaPlayerPlaying,
    libvlc_MediaPlayerPaused, libvlc_MediaPlayerStopped,
    libvlc_MediaPlayerEndReached, libvlc_MediaPlayerEncounteredError,
    libvlc_MediaPlayerTimeChanged, libvlc_MediaPlayerLengthChanged,
    libvlc_MediaPlayerSeekableChanged, libvlc_MediaPlayerVout,
    libvlc_MediaPlayerChapterChanged,
};

QString pathFromUrl(const QUrl &url)
{
    if (!url.isValid() || !url.isLocalFile()) {
        return {};
    }
    return QFileInfo(url.toLocalFile()).absoluteFilePath();
}

QVariantMap trackEntry(const QString &label, int id, bool external = false)
{
    return {
        {QStringLiteral("label"), label},
        {QStringLiteral("id"), id},
        {QStringLiteral("external"), external},
    };
}

QList<veylo::TrackOption> trackOptions(const QVariantList &tracks)
{
    QList<veylo::TrackOption> options;
    options.reserve(tracks.size());
    for (const QVariant &track : tracks) {
        const QVariantMap entry = track.toMap();
        options.append({
            entry.value(QStringLiteral("label")).toString(),
            entry.value(QStringLiteral("id"), -1).toInt(),
            entry.value(QStringLiteral("external"), false).toBool(),
        });
    }
    return options;
}

std::optional<veylo::SavedTrackPreference> preferenceForTrack(
    const QVariantList &tracks, int selectedId)
{
    const bool enabled = selectedId >= 0;
    int ordinal = 0;
    for (const veylo::TrackOption &option : trackOptions(tracks)) {
        if (option.external || (enabled ? option.id < 0 : option.id >= 0)) {
            continue;
        }
        if (option.id == selectedId) {
            return veylo::SavedTrackPreference{option.label, ordinal, enabled};
        }
        ++ordinal;
    }
    return std::nullopt;
}

QString trackPreferenceKey(const QString &mediaPath, const QString &kind)
{
    return QStringLiteral("trackPreferences/%1/%2")
        .arg(veylo::folderPreferenceIdForMediaPath(mediaPath), kind);
}

void writeTrackPreference(const QString &mediaPath,
                          const QString &kind,
                          const veylo::SavedTrackPreference &preference)
{
    QSettings settings;
    const QString key = trackPreferenceKey(mediaPath, kind);
    settings.setValue(key + QStringLiteral("/enabled"), preference.enabled);
    settings.setValue(key + QStringLiteral("/label"), preference.label);
    settings.setValue(key + QStringLiteral("/ordinal"), preference.ordinal);
}

std::optional<veylo::SavedTrackPreference> readTrackPreference(
    const QSettings &settings, const QString &mediaPath, const QString &kind)
{
    const QString key = trackPreferenceKey(mediaPath, kind);
    if (!settings.contains(key + QStringLiteral("/enabled"))) {
        return std::nullopt;
    }
    return veylo::SavedTrackPreference{
        settings.value(key + QStringLiteral("/label")).toString(),
        settings.value(key + QStringLiteral("/ordinal"), 0).toInt(),
        settings.value(key + QStringLiteral("/enabled"), true).toBool(),
    };
}

bool pathsMatch(const QString &left, const QString &right)
{
#if defined(Q_OS_WIN)
    return QDir::cleanPath(left).compare(QDir::cleanPath(right), Qt::CaseInsensitive) == 0;
#else
    return QDir::cleanPath(left) == QDir::cleanPath(right);
#endif
}

QStringList naturallySortedUniquePaths(QStringList paths)
{
    std::sort(paths.begin(), paths.end(), veylo::NaturalLess());

    QStringList result;
    result.reserve(paths.size());
    QSet<QString> seen;
    for (const QString &path : std::as_const(paths)) {
        QString key = QDir::cleanPath(path);
#if defined(Q_OS_WIN)
        key = key.toCaseFolded();
#endif
        if (!seen.contains(key)) {
            seen.insert(key);
            result.append(path);
        }
    }
    return result;
}

} // namespace

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
    , videoSurface_(std::make_unique<VideoSurfaceWindow>())
{
    connect(videoSurface_.get(), &VideoSurfaceWindow::doubleClicked,
            this, &PlayerController::fullscreenToggleRequested);
    connect(videoSurface_.get(), &VideoSurfaceWindow::clicked,
            this, &PlayerController::mediaSurfaceClicked);
    connect(videoSurface_.get(), &VideoSurfaceWindow::pointerActivity,
            this, &PlayerController::mediaSurfaceActivity);
    loadResumeState();
}

bool PlayerController::ensureMediaEngine()
{
    if (vlcInstance_ && mediaPlayer_) {
        return true;
    }

#if defined(Q_OS_MACOS)
    const QString bundledPluginPath = QDir(QCoreApplication::applicationDirPath())
                                          .filePath(QStringLiteral("plugins"));
    if (QDir(bundledPluginPath).exists()) {
        qputenv("VLC_PLUGIN_PATH", bundledPluginPath.toUtf8());
    }
#endif

    const char *arguments[] = {"--no-video-title-show", "--quiet"};
    vlcInstance_ = libvlc_new(static_cast<int>(std::size(arguments)), arguments);
    if (!vlcInstance_) {
        setError(tr("The media engine could not be started."));
        return false;
    }
    mediaPlayer_ = libvlc_media_player_new(vlcInstance_);
    if (!mediaPlayer_) {
        setError(tr("The media player could not be created."));
        libvlc_release(vlcInstance_);
        vlcInstance_ = nullptr;
        return false;
    }
    attachVideoOutput();
    attachVlcEvents();
    libvlc_video_set_mouse_input(mediaPlayer_, 0);
    libvlc_video_set_key_input(mediaPlayer_, 0);
    libvlc_audio_set_volume(mediaPlayer_, volume_);
    libvlc_audio_set_mute(mediaPlayer_, muted_ ? 1 : 0);
    return true;
}

PlayerController::~PlayerController()
{
    persistCurrentVideoProgress(true);
    if (mediaPlayer_) {
        detachVlcEvents();
        libvlc_media_player_stop(mediaPlayer_);
        libvlc_media_player_release(mediaPlayer_);
    }
    if (vlcInstance_) {
        libvlc_release(vlcInstance_);
    }
}

QString PlayerController::currentFilePath() const { return currentFilePath_; }
QString PlayerController::title() const { return title_; }
PlayerController::MediaKind PlayerController::mediaKind() const { return toPublicKind(currentKind_); }
bool PlayerController::hasMedia() const { return currentKind_ != veylo::MediaKind::None; }
bool PlayerController::isAudio() const { return currentKind_ == veylo::MediaKind::Audio; }
bool PlayerController::isVideo() const { return currentKind_ == veylo::MediaKind::Video; }
bool PlayerController::isImage() const { return currentKind_ == veylo::MediaKind::Image; }
bool PlayerController::playing() const { return playing_; }
bool PlayerController::loading() const { return loading_; }
bool PlayerController::seekable() const { return seekable_; }
qint64 PlayerController::position() const { return position_; }
qint64 PlayerController::duration() const { return duration_; }
QVariantList PlayerController::chapterPositions() const { return chapterPositions_; }
int PlayerController::volume() const { return volume_; }
bool PlayerController::muted() const { return muted_; }
QString PlayerController::errorMessage() const { return errorMessage_; }
bool PlayerController::canPreviousImage() const { return canPreviousImage_; }
bool PlayerController::canNextImage() const { return canNextImage_; }
int PlayerController::imageIndex() const { return imageIndex_; }
int PlayerController::imageCount() const { return imageCount_; }
bool PlayerController::canPreviousMedia() const { return canPreviousMedia_; }
bool PlayerController::canNextMedia() const { return canNextMedia_; }
QVariantList PlayerController::audioTracks() const { return audioTracks_; }
QVariantList PlayerController::subtitleTracks() const { return subtitleTracks_; }
int PlayerController::activeAudioTrack() const { return activeAudioTrack_; }
int PlayerController::activeSubtitleTrack() const { return activeSubtitleTrack_; }
bool PlayerController::resumeAvailable() const { return resumeAvailable_; }
QString PlayerController::resumeTitle() const { return resumeTitle_; }
qint64 PlayerController::resumePosition() const { return resumePosition_; }
QWindow *PlayerController::videoWindow() const { return videoSurface_.get(); }

QUrl PlayerController::imageSource() const
{
    return isImage() ? QUrl(QStringLiteral("image://current/%1").arg(imageRevision_)) : QUrl();
}

bool PlayerController::openFile(const QString &path)
{
    pendingResumePosition_ = -1;
    dismissResume();
    clearRecursiveQueue();
    consecutiveAdvanceFailures_ = 0;
    return openFileInternal(path, false);
}

bool PlayerController::openFolder(const QString &path)
{
    const QFileInfo folderInfo(path);
    if (!folderInfo.exists() || !folderInfo.isDir() || !folderInfo.isReadable()) {
        setError(tr("The folder is missing or cannot be read: %1").arg(folderInfo.fileName()));
        return false;
    }

    pendingResumePosition_ = -1;
    dismissResume();
    clearRecursiveQueue();
    recursiveQueue_ = veylo::FolderSequence::recursiveFiles(
        folderInfo.absoluteFilePath(), veylo::MediaKind::Video);
    recursiveQueueKind_ = veylo::MediaKind::Video;
    if (recursiveQueue_.isEmpty()) {
        recursiveQueue_ = veylo::FolderSequence::recursiveFiles(
            folderInfo.absoluteFilePath(), veylo::MediaKind::Image);
        recursiveQueueKind_ = veylo::MediaKind::Image;
    }
    if (recursiveQueue_.isEmpty()) {
        clearRecursiveQueue();
        setError(tr("No supported media files were found in %1.").arg(folderInfo.fileName()));
        return false;
    }

    consecutiveAdvanceFailures_ = 0;
    const bool automaticAdvance = recursiveQueueKind_ != veylo::MediaKind::Image;
    if (openRecursiveQueueItem(0, 1, automaticAdvance)) {
        return true;
    }
    clearRecursiveQueue();
    return false;
}

bool PlayerController::openUrl(const QUrl &url)
{
    const QString path = pathFromUrl(url);
    if (path.isEmpty()) {
        setError(tr("Only local files and folders can be opened in this version."));
        return false;
    }
    return QFileInfo(path).isDir() ? openFolder(path) : openFile(path);
}

bool PlayerController::openUrls(const QList<QUrl> &urls)
{
    if (urls.size() == 1) {
        return openUrl(urls.constFirst());
    }

    QStringList playableMedia;
    QStringList images;
    for (const QUrl &url : urls) {
        const QString path = pathFromUrl(url);
        if (path.isEmpty()) {
            continue;
        }

        const QFileInfo info(path);
        if (!info.exists() || !info.isReadable()) {
            continue;
        }
        if (info.isDir()) {
            playableMedia.append(veylo::FolderSequence::recursiveFiles(
                info.absoluteFilePath(), veylo::MediaKind::Video));
            images.append(veylo::FolderSequence::recursiveFiles(
                info.absoluteFilePath(), veylo::MediaKind::Image));
            continue;
        }
        if (!info.isFile()) {
            continue;
        }

        const QString absolutePath = info.absoluteFilePath();
        const veylo::MediaKind kind = veylo::mediaKindForPath(absolutePath);
        if (veylo::isAudioOrVideo(kind)) {
            playableMedia.append(absolutePath);
        } else if (kind == veylo::MediaKind::Image) {
            images.append(absolutePath);
        }
    }

    recursiveQueue_ = naturallySortedUniquePaths(
        playableMedia.isEmpty() ? images : playableMedia);
    recursiveQueueKind_ = playableMedia.isEmpty()
        ? veylo::MediaKind::Image
        : veylo::MediaKind::Video;
    if (recursiveQueue_.isEmpty()) {
        clearRecursiveQueue();
        setError(tr("No supported media files were selected."));
        return false;
    }

    pendingResumePosition_ = -1;
    dismissResume();
    consecutiveAdvanceFailures_ = 0;
    const bool automaticAdvance = recursiveQueueKind_ != veylo::MediaKind::Image;
    if (openRecursiveQueueItem(0, 1, automaticAdvance)) {
        return true;
    }
    clearRecursiveQueue();
    return false;
}

bool PlayerController::openFileInternal(const QString &path, bool automaticAdvance)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        setError(tr("The file is missing or cannot be read: %1").arg(info.fileName()));
        return false;
    }
    const QString absolutePath = info.absoluteFilePath();
    const veylo::MediaKind kind = veylo::mediaKindForPath(absolutePath);
    if (kind == veylo::MediaKind::None) {
        setError(tr("This file type is not supported: %1").arg(info.suffix()));
        return false;
    }
    if (!currentFilePath_.isEmpty() && currentFilePath_ != absolutePath) {
        persistCurrentVideoProgress(true);
    }
    clearError();
    automaticAdvance_ = automaticAdvance;
    return kind == veylo::MediaKind::Image
        ? openImage(absolutePath)
        : openPlayableMedia(absolutePath, kind, automaticAdvance);
}

bool PlayerController::openRecursiveQueueItem(qsizetype index,
                                              int direction,
                                              bool automaticAdvance)
{
    if (direction == 0) {
        return false;
    }
    const qsizetype previousIndex = recursiveQueueIndex_;
    while (index >= 0 && index < recursiveQueue_.size()) {
        recursiveQueueIndex_ = index;
        if (openFileInternal(recursiveQueue_.at(index), automaticAdvance)) {
            return true;
        }
        index += direction < 0 ? -1 : 1;
    }
    recursiveQueueIndex_ = previousIndex;
    automaticAdvance_ = false;
    updateImageNavigation();
    updateMediaNavigation();
    return false;
}

bool PlayerController::openImage(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    if (!reader.canRead()) {
        setError(tr("The image could not be opened: %1").arg(reader.errorString()));
        return false;
    }
    if (mediaPlayer_) {
        libvlc_media_player_stop(mediaPlayer_);
        libvlc_media_player_set_media(mediaPlayer_, nullptr);
    }
    currentFilePath_ = path;
    title_ = QFileInfo(path).fileName();
    setCurrentKind(veylo::MediaKind::Image);
    setPlaying(false);
    setLoading(false);
    resetTimeline();
    audioTracks_.clear();
    subtitleTracks_.clear();
    activeAudioTrack_ = -1;
    activeSubtitleTrack_ = -1;
    externalSubtitlePath_.clear();
    audioPreferenceApplied_ = false;
    subtitlePreferenceApplied_ = false;
    emit tracksChanged();
    emit activeTracksChanged();
    ++imageRevision_;
    updateImageNavigation();
    emit imageSourceChanged();
    return true;
}

bool PlayerController::openPlayableMedia(const QString &path,
                                         veylo::MediaKind kind,
                                         bool automaticAdvance)
{
    if (!ensureMediaEngine()) {
        return false;
    }
    const QByteArray location = QUrl::fromLocalFile(path).toEncoded();
    libvlc_media_t *media = libvlc_media_new_location(vlcInstance_, location.constData());
    if (!media) {
        setError(tr("The media engine could not open %1.").arg(QFileInfo(path).fileName()));
        return false;
    }
    libvlc_media_player_stop(mediaPlayer_);
    attachVideoOutput();
    libvlc_media_player_set_media(mediaPlayer_, media);
    libvlc_media_release(media);

    currentFilePath_ = path;
    title_ = QFileInfo(path).fileName();
    setCurrentKind(kind);
    updateMediaNavigation();
    resetTimeline();
    audioTracks_.clear();
    subtitleTracks_ = {trackEntry(tr("Off"), -1)};
    activeAudioTrack_ = -1;
    activeSubtitleTrack_ = -1;
    externalSubtitlePath_.clear();
    audioPreferenceApplied_ = false;
    subtitlePreferenceApplied_ = false;
    automaticAdvance_ = automaticAdvance;
    setLoading(true);
    emit tracksChanged();
    emit activeTracksChanged();
    if (libvlc_media_player_play(mediaPlayer_) != 0) {
        setLoading(false);
        setError(tr("Playback could not be started for %1.").arg(title_));
        return false;
    }
    if (kind == veylo::MediaKind::Video) {
        rememberOpenedVideo(path);
    }
    return true;
}

void PlayerController::playPause()
{
    if (mediaPlayer_ && veylo::isAudioOrVideo(currentKind_)) {
        playing_ ? pause() : play();
    }
}

void PlayerController::play()
{
    if (mediaPlayer_ && veylo::isAudioOrVideo(currentKind_)) {
        if (ended_) {
            ended_ = false;
            pendingSeekPosition_ = 0;
            if (position_ != 0) {
                position_ = 0;
                emit positionChanged();
            }
            setLoading(true);
            libvlc_media_player_stop(mediaPlayer_);
        }
        libvlc_media_player_play(mediaPlayer_);
    }
}

void PlayerController::pause()
{
    if (mediaPlayer_ && veylo::isAudioOrVideo(currentKind_)) {
        persistCurrentVideoProgress(true);
        libvlc_media_player_set_pause(mediaPlayer_, 1);
    }
}

void PlayerController::stop()
{
    if (mediaPlayer_) {
        persistCurrentVideoProgress(true);
        automaticAdvance_ = false;
        libvlc_media_player_stop(mediaPlayer_);
    }
}

void PlayerController::seek(qint64 positionMs)
{
    if (!mediaPlayer_ || !veylo::isAudioOrVideo(currentKind_)
        || (!seekable_ && !ended_ && pendingSeekPosition_ < 0)) {
        return;
    }

    const qint64 bounded = std::clamp<qint64>(
        positionMs, 0, std::max<qint64>(duration_, 0));
    if (ended_ || pendingSeekPosition_ >= 0) {
        pendingSeekPosition_ = bounded;
        if (position_ != bounded) {
            position_ = bounded;
            emit positionChanged();
        }
        if (ended_) {
            ended_ = false;
            automaticAdvance_ = false;
            setLoading(true);
            libvlc_media_player_stop(mediaPlayer_);
            if (libvlc_media_player_play(mediaPlayer_) != 0) {
                pendingSeekPosition_ = -1;
                setLoading(false);
                setError(tr("Playback could not be restarted for %1.").arg(title_));
            }
        }
        return;
    }

    libvlc_media_player_set_time(mediaPlayer_, static_cast<libvlc_time_t>(bounded));
}

void PlayerController::setVolume(int volume)
{
    const int bounded = std::clamp(volume, 0, 100);
    if (volume_ == bounded) {
        return;
    }
    volume_ = bounded;
    if (mediaPlayer_) {
        libvlc_audio_set_volume(mediaPlayer_, volume_);
    }
    emit volumeChanged();
}

void PlayerController::toggleMute()
{
    muted_ = !muted_;
    if (mediaPlayer_) {
        libvlc_audio_set_mute(mediaPlayer_, muted_ ? 1 : 0);
    }
    emit mutedChanged();
}

void PlayerController::previousImage()
{
    if (isImage() && recursiveQueueKind_ == veylo::MediaKind::Image) {
        openRecursiveQueueItem(recursiveQueueIndex_ - 1, -1, false);
        return;
    }
    const QString previous = isImage()
        ? veylo::FolderSequence::adjacentFile(currentFilePath_, veylo::MediaKind::Image, -1)
        : QString();
    if (!previous.isEmpty()) {
        openFile(previous);
    }
}

void PlayerController::nextImage()
{
    if (isImage() && recursiveQueueKind_ == veylo::MediaKind::Image) {
        openRecursiveQueueItem(recursiveQueueIndex_ + 1, 1, false);
        return;
    }
    const QString next = isImage()
        ? veylo::FolderSequence::adjacentFile(currentFilePath_, veylo::MediaKind::Image, 1)
        : QString();
    if (!next.isEmpty()) {
        openFile(next);
    }
}

void PlayerController::previousMedia()
{
    if (!veylo::isAudioOrVideo(currentKind_)) {
        return;
    }
    if (goToAdjacentChapter(-1)) {
        return;
    }
    if (veylo::isAudioOrVideo(recursiveQueueKind_)) {
        openRecursiveQueueItem(recursiveQueueIndex_ - 1, -1, false);
        return;
    }
    const QString previous = veylo::FolderSequence::adjacentFile(
        currentFilePath_, veylo::MediaKind::Video, -1);
    if (!previous.isEmpty()) {
        openFileInternal(previous, false);
    }
}

void PlayerController::nextMedia()
{
    if (!veylo::isAudioOrVideo(currentKind_)) {
        return;
    }
    if (goToAdjacentChapter(1)) {
        return;
    }
    if (veylo::isAudioOrVideo(recursiveQueueKind_)) {
        openRecursiveQueueItem(recursiveQueueIndex_ + 1, 1, false);
        return;
    }
    const QString next = veylo::FolderSequence::adjacentFile(
        currentFilePath_, veylo::MediaKind::Video, 1);
    if (!next.isEmpty()) {
        openFileInternal(next, false);
    }
}

bool PlayerController::addExternalSubtitle(const QUrl &url)
{
    if (!mediaPlayer_ || !isVideo()) {
        setError(tr("Open a video before loading subtitles."));
        return false;
    }
    const QString path = pathFromUrl(url);
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        setError(tr("The subtitle file could not be found."));
        return false;
    }
    const QByteArray location = QUrl::fromLocalFile(path).toEncoded();
    const int result = libvlc_media_player_add_slave(
        mediaPlayer_, libvlc_media_slave_type_subtitle, location.constData(), true);
    if (result != 0) {
        setError(tr("The subtitle file could not be loaded."));
        return false;
    }
    externalSubtitlePath_ = path;
    clearError();
    QTimer::singleShot(150, this, &PlayerController::refreshTracks);
    return true;
}

void PlayerController::selectAudioTrack(int id)
{
    if (!mediaPlayer_ || libvlc_audio_set_track(mediaPlayer_, id) != 0) {
        setError(tr("The selected audio track is unavailable."));
        return;
    }
    activeAudioTrack_ = id;
    audioPreferenceApplied_ = true;
    rememberAudioTrackPreference(id);
    emit activeTracksChanged();
}

void PlayerController::selectSubtitleTrack(int id)
{
    if (!mediaPlayer_ || libvlc_video_set_spu(mediaPlayer_, id) != 0) {
        setError(tr("The selected subtitle track is unavailable."));
        return;
    }
    activeSubtitleTrack_ = id;
    subtitlePreferenceApplied_ = true;
    rememberSubtitleTrackPreference(id);
    emit activeTracksChanged();
}

void PlayerController::refreshTracks()
{
    if (!mediaPlayer_ || !veylo::isAudioOrVideo(currentKind_)) {
        return;
    }
    QVariantList audio;
    libvlc_track_description_t *audioDescription = libvlc_audio_get_track_description(mediaPlayer_);
    for (auto *track = audioDescription; track; track = track->p_next) {
        audio.append(trackEntry(displayTrackName(
            track->psz_name, tr("Audio %1").arg(audio.size() + 1)), track->i_id));
    }
    if (audioDescription) {
        libvlc_track_description_list_release(audioDescription);
    }

    QVariantList subtitles = {trackEntry(tr("Off"), -1)};
    const QString externalName = QFileInfo(externalSubtitlePath_).fileName();
    libvlc_track_description_t *subtitleDescription = libvlc_video_get_spu_description(mediaPlayer_);
    for (auto *track = subtitleDescription; track; track = track->p_next) {
        if (track->i_id < 0) {
            continue;
        }
        QString label = displayTrackName(track->psz_name,
                                         tr("Subtitle %1").arg(subtitles.size()));
        const bool isExternal = !externalName.isEmpty()
            && label.contains(externalName, Qt::CaseInsensitive);
        if (isExternal) {
            label = tr("%1 — External").arg(label);
        }
        subtitles.append(trackEntry(label, track->i_id, isExternal));
    }
    if (subtitleDescription) {
        libvlc_track_description_list_release(subtitleDescription);
    }

    const int previousAudioId = activeAudioTrack_;
    const int previousSubtitleId = activeSubtitleTrack_;
    const bool tracksDiffer = audioTracks_ != audio || subtitleTracks_ != subtitles;
    audioTracks_ = audio;
    subtitleTracks_ = subtitles;
    activeAudioTrack_ = libvlc_audio_get_track(mediaPlayer_);
    activeSubtitleTrack_ = libvlc_video_get_spu(mediaPlayer_);
    applyFolderTrackPreferences();
    const bool activeDiffer = previousAudioId != activeAudioTrack_
        || previousSubtitleId != activeSubtitleTrack_;
    if (tracksDiffer) emit tracksChanged();
    if (activeDiffer) emit activeTracksChanged();
}

void PlayerController::rememberAudioTrackPreference(int id)
{
    if (currentFilePath_.isEmpty()) {
        return;
    }
    const auto preference = preferenceForTrack(audioTracks_, id);
    if (preference) {
        writeTrackPreference(currentFilePath_, QStringLiteral("audio"), *preference);
    }
}

void PlayerController::rememberSubtitleTrackPreference(int id)
{
    if (currentFilePath_.isEmpty()) {
        return;
    }
    const auto preference = preferenceForTrack(subtitleTracks_, id);
    if (preference) {
        writeTrackPreference(currentFilePath_, QStringLiteral("subtitle"), *preference);
    }
}

bool PlayerController::applyFolderTrackPreferences()
{
    if (!mediaPlayer_ || currentFilePath_.isEmpty()) {
        return false;
    }

    const QSettings settings;
    bool changed = false;

    if (!audioPreferenceApplied_) {
        const auto preference = readTrackPreference(
            settings, currentFilePath_, QStringLiteral("audio"));
        if (!preference) {
            audioPreferenceApplied_ = true;
        } else {
            const QList<veylo::TrackOption> options = trackOptions(audioTracks_);
            const bool hasEligibleTrack = std::any_of(
                options.cbegin(), options.cend(), [&preference](const veylo::TrackOption &option) {
                    return !option.external
                        && (preference->enabled ? option.id >= 0 : option.id < 0);
                });
            if (hasEligibleTrack) {
                const int targetId = veylo::bestTrackId(
                    options, *preference, activeAudioTrack_);
                if (targetId == activeAudioTrack_
                    || libvlc_audio_set_track(mediaPlayer_, targetId) == 0) {
                    changed = changed || targetId != activeAudioTrack_;
                    activeAudioTrack_ = targetId;
                    audioPreferenceApplied_ = true;
                }
            }
        }
    }

    if (!subtitlePreferenceApplied_) {
        const auto preference = readTrackPreference(
            settings, currentFilePath_, QStringLiteral("subtitle"));
        if (!preference) {
            subtitlePreferenceApplied_ = true;
        } else {
            const QList<veylo::TrackOption> options = trackOptions(subtitleTracks_);
            const bool hasEligibleTrack = std::any_of(
                options.cbegin(), options.cend(), [&preference](const veylo::TrackOption &option) {
                    return !option.external
                        && (preference->enabled ? option.id >= 0 : option.id < 0);
                });
            if (hasEligibleTrack) {
                const int targetId = veylo::bestTrackId(
                    options, *preference, activeSubtitleTrack_);
                if (targetId == activeSubtitleTrack_
                    || libvlc_video_set_spu(mediaPlayer_, targetId) == 0) {
                    changed = changed || targetId != activeSubtitleTrack_;
                    activeSubtitleTrack_ = targetId;
                    subtitlePreferenceApplied_ = true;
                }
            }
        }
    }

    return changed;
}

bool PlayerController::resumeLastVideo()
{
    const QFileInfo info(resumeFilePath_);
    if (!resumeAvailable_ || !info.exists() || !info.isFile() || !info.isReadable()
        || veylo::mediaKindForPath(info.absoluteFilePath()) != veylo::MediaKind::Video) {
        QSettings settings;
        settings.remove(QStringLiteral("playback/lastVideoPath"));
        settings.remove(QStringLiteral("playback/lastVideoPositionMs"));
        settings.remove(QStringLiteral("playback/lastVideoDurationMs"));
        dismissResume();
        setError(tr("The previous video is no longer available."));
        return false;
    }

    const QString path = info.absoluteFilePath();
    const qint64 position = resumePosition_;
    pendingResumePosition_ = position;
    dismissResume();
    clearRecursiveQueue();
    consecutiveAdvanceFailures_ = 0;
    if (!openFileInternal(path, false)) {
        pendingResumePosition_ = -1;
        return false;
    }
    return true;
}

void PlayerController::dismissResume()
{
    if (!resumeAvailable_) {
        return;
    }
    resumeAvailable_ = false;
    emit resumeChanged();
}

void PlayerController::clearError()
{
    if (!errorMessage_.isEmpty()) {
        errorMessage_.clear();
        emit errorMessageChanged();
    }
}

void PlayerController::vlcEventCallback(const libvlc_event_t *event, void *userData)
{
    if (!event || !userData) return;
    auto *controller = static_cast<PlayerController *>(userData);
    const libvlc_event_t eventCopy = *event;
    QMetaObject::invokeMethod(controller, [controller, eventCopy] {
        controller->handleVlcEvent(eventCopy);
    }, Qt::QueuedConnection);
}

void PlayerController::attachVlcEvents()
{
    libvlc_event_manager_t *manager = libvlc_media_player_event_manager(mediaPlayer_);
    for (const libvlc_event_e event : observedEvents)
        libvlc_event_attach(manager, event, &PlayerController::vlcEventCallback, this);
}

void PlayerController::detachVlcEvents()
{
    libvlc_event_manager_t *manager = libvlc_media_player_event_manager(mediaPlayer_);
    for (const libvlc_event_e event : observedEvents)
        libvlc_event_detach(manager, event, &PlayerController::vlcEventCallback, this);
}

void PlayerController::attachVideoOutput()
{
    if (!mediaPlayer_ || !videoSurface_) return;
    const WId handle = videoSurface_->winId();
#if defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(mediaPlayer_, reinterpret_cast<void *>(handle));
#elif defined(Q_OS_MACOS)
    libvlc_media_player_set_nsobject(mediaPlayer_, reinterpret_cast<void *>(handle));
#else
    Q_UNUSED(handle)
#endif
}

void PlayerController::handleVlcEvent(const libvlc_event_t &event)
{
    switch (event.type) {
    case libvlc_MediaPlayerOpening:
        setLoading(true);
        break;
    case libvlc_MediaPlayerPlaying:
        setLoading(false);
        setPlaying(true);
        ended_ = false;
        setSeekable(libvlc_media_player_is_seekable(mediaPlayer_) != 0
                    || pendingSeekPosition_ >= 0);
        applyPendingSeekPosition();
        applyPendingResumePosition();
        updateMediaNavigation();
        refreshChapters();
        QTimer::singleShot(100, this, &PlayerController::refreshChapters);
        QTimer::singleShot(100, this, &PlayerController::refreshTracks);
        break;
    case libvlc_MediaPlayerPaused:
        setPlaying(false);
        persistCurrentVideoProgress(true);
        break;
    case libvlc_MediaPlayerStopped:
        setLoading(false);
        setPlaying(false);
        persistCurrentVideoProgress(true);
        break;
    case libvlc_MediaPlayerEndReached:
        setLoading(false);
        setPlaying(false);
        ended_ = true;
        pendingSeekPosition_ = -1;
        if (duration_ > 0 && position_ != duration_) {
            position_ = duration_;
            emit positionChanged();
        }
        setSeekable(duration_ > 0);
        consecutiveAdvanceFailures_ = 0;
        markCurrentVideoFinished();
        advanceAfterPlayback();
        break;
    case libvlc_MediaPlayerEncounteredError:
        handlePlaybackError();
        break;
    case libvlc_MediaPlayerTimeChanged:
        position_ = static_cast<qint64>(event.u.media_player_time_changed.new_time);
        emit positionChanged();
        persistCurrentVideoProgress();
        break;
    case libvlc_MediaPlayerLengthChanged:
        duration_ = static_cast<qint64>(event.u.media_player_length_changed.new_length);
        emit durationChanged();
        refreshChapters();
        applyPendingSeekPosition();
        applyPendingResumePosition();
        break;
    case libvlc_MediaPlayerSeekableChanged:
        setSeekable(event.u.media_player_seekable_changed.new_seekable != 0
                    || (ended_ && duration_ > 0)
                    || pendingSeekPosition_ >= 0);
        applyPendingSeekPosition();
        applyPendingResumePosition();
        break;
    case libvlc_MediaPlayerVout:
        QTimer::singleShot(50, this, &PlayerController::refreshTracks);
        break;
    case libvlc_MediaPlayerChapterChanged:
        updateMediaNavigation();
        refreshChapters();
        break;
    default:
        break;
    }
}

void PlayerController::handlePlaybackError()
{
    setLoading(false);
    setPlaying(false);
    setError(tr("VeyloPlayer could not play %1.").arg(title_));
    if (automaticAdvance_ && consecutiveAdvanceFailures_ < 100) {
        ++consecutiveAdvanceFailures_;
        advanceAfterPlayback();
    }
}

void PlayerController::advanceAfterPlayback()
{
    if (!veylo::isAudioOrVideo(currentKind_)) return;
    if (veylo::isAudioOrVideo(recursiveQueueKind_)) {
        if (!openRecursiveQueueItem(recursiveQueueIndex_ + 1, 1, true)) {
            automaticAdvance_ = false;
        }
        return;
    }
    const QString next = veylo::FolderSequence::adjacentFile(
        currentFilePath_, veylo::MediaKind::Video, 1);
    if (next.isEmpty()) {
        automaticAdvance_ = false;
        return;
    }
    openFileInternal(next, true);
}

void PlayerController::loadResumeState()
{
    const QSettings settings;
    const QString path = settings.value(QStringLiteral("playback/lastVideoPath")).toString();
    const qint64 position = settings.value(
        QStringLiteral("playback/lastVideoPositionMs"), 0).toLongLong();
    const qint64 duration = settings.value(
        QStringLiteral("playback/lastVideoDurationMs"), 0).toLongLong();
    const QFileInfo info(path);
    const qint64 resumablePosition = veylo::normalizedResumePosition(position, duration);
    if (!info.exists() || !info.isFile() || !info.isReadable()
        || veylo::mediaKindForPath(info.absoluteFilePath()) != veylo::MediaKind::Video
        || resumablePosition <= 0) {
        return;
    }
    resumeFilePath_ = info.absoluteFilePath();
    resumeTitle_ = info.fileName();
    resumePosition_ = resumablePosition;
    resumeAvailable_ = true;
}

void PlayerController::rememberOpenedVideo(const QString &path)
{
    resumeFilePath_ = QFileInfo(path).absoluteFilePath();
    resumeTitle_ = QFileInfo(path).fileName();
    resumePosition_ = 0;
    lastPersistedPosition_ = 0;
    QSettings settings;
    settings.setValue(QStringLiteral("playback/lastVideoPath"), resumeFilePath_);
    settings.setValue(QStringLiteral("playback/lastVideoPositionMs"), 0);
    settings.setValue(QStringLiteral("playback/lastVideoDurationMs"), 0);
}

void PlayerController::persistCurrentVideoProgress(bool force)
{
    if (currentKind_ != veylo::MediaKind::Video || currentFilePath_.isEmpty()) {
        return;
    }
    if (!force && lastPersistedPosition_ >= 0
        && std::abs(position_ - lastPersistedPosition_) < 5'000) {
        return;
    }

    const qint64 resumablePosition = veylo::normalizedResumePosition(position_, duration_);
    QSettings settings;
    settings.setValue(QStringLiteral("playback/lastVideoPath"), currentFilePath_);
    settings.setValue(QStringLiteral("playback/lastVideoPositionMs"), resumablePosition);
    settings.setValue(QStringLiteral("playback/lastVideoDurationMs"), duration_);
    if (force) {
        settings.sync();
    }
    resumeFilePath_ = currentFilePath_;
    resumeTitle_ = QFileInfo(currentFilePath_).fileName();
    resumePosition_ = resumablePosition;
    lastPersistedPosition_ = position_;
}

void PlayerController::applyPendingResumePosition()
{
    if (pendingResumePosition_ <= 0 || !mediaPlayer_ || !seekable_) {
        return;
    }
    const qint64 target = veylo::normalizedResumePosition(
        pendingResumePosition_, duration_);
    pendingResumePosition_ = -1;
    if (target <= 0) {
        return;
    }
    libvlc_media_player_set_time(mediaPlayer_, static_cast<libvlc_time_t>(target));
    position_ = target;
    lastPersistedPosition_ = target;
    emit positionChanged();
    persistCurrentVideoProgress(true);
}

void PlayerController::applyPendingSeekPosition()
{
    if (pendingSeekPosition_ < 0 || !mediaPlayer_
        || libvlc_media_player_is_seekable(mediaPlayer_) == 0) {
        return;
    }

    const qint64 target = std::clamp<qint64>(
        pendingSeekPosition_, 0, std::max<qint64>(duration_, 0));
    libvlc_media_player_set_time(
        mediaPlayer_, static_cast<libvlc_time_t>(target));
    pendingSeekPosition_ = -1;
    if (position_ != target) {
        position_ = target;
        emit positionChanged();
    }
    setSeekable(true);
}

void PlayerController::markCurrentVideoFinished()
{
    if (currentKind_ != veylo::MediaKind::Video || currentFilePath_.isEmpty()) {
        return;
    }
    QSettings settings;
    settings.setValue(QStringLiteral("playback/lastVideoPath"), currentFilePath_);
    settings.setValue(QStringLiteral("playback/lastVideoPositionMs"), 0);
    settings.setValue(QStringLiteral("playback/lastVideoDurationMs"), duration_);
    resumePosition_ = 0;
    lastPersistedPosition_ = position_;
}

void PlayerController::clearRecursiveQueue()
{
    recursiveQueue_.clear();
    recursiveQueueIndex_ = -1;
    recursiveQueueKind_ = veylo::MediaKind::None;
    if (isImage()) {
        updateImageNavigation();
    } else if (veylo::isAudioOrVideo(currentKind_)) {
        updateMediaNavigation();
    }
}

void PlayerController::updateImageNavigation()
{
    const bool recursiveImages = recursiveQueueKind_ == veylo::MediaKind::Image;
    bool previous = false;
    bool next = false;
    int index = 0;
    int count = 0;

    if (isImage() && recursiveImages) {
        count = static_cast<int>(recursiveQueue_.size());
        if (recursiveQueueIndex_ >= 0 && recursiveQueueIndex_ < recursiveQueue_.size()) {
            index = static_cast<int>(recursiveQueueIndex_ + 1);
            previous = recursiveQueueIndex_ > 0;
            next = recursiveQueueIndex_ + 1 < recursiveQueue_.size();
        }
    } else if (isImage()) {
        const QStringList files = veylo::FolderSequence::filesFor(
            currentFilePath_, veylo::MediaKind::Image);
        count = static_cast<int>(files.size());
        for (qsizetype fileIndex = 0; fileIndex < files.size(); ++fileIndex) {
            if (!pathsMatch(files.at(fileIndex), currentFilePath_)) {
                continue;
            }
            index = static_cast<int>(fileIndex + 1);
            previous = fileIndex > 0;
            next = fileIndex + 1 < files.size();
            break;
        }
    }

    if (previous == canPreviousImage_ && next == canNextImage_
        && index == imageIndex_ && count == imageCount_) {
        return;
    }
    canPreviousImage_ = previous;
    canNextImage_ = next;
    imageIndex_ = index;
    imageCount_ = count;
    emit imageNavigationChanged();
}

void PlayerController::updateMediaNavigation()
{
    bool previous = false;
    bool next = false;

    if (veylo::isAudioOrVideo(currentKind_)
        && veylo::isAudioOrVideo(recursiveQueueKind_)) {
        previous = recursiveQueueIndex_ > 0;
        next = recursiveQueueIndex_ >= 0
            && recursiveQueueIndex_ + 1 < recursiveQueue_.size();
    } else if (veylo::isAudioOrVideo(currentKind_)) {
        previous = !veylo::FolderSequence::adjacentFile(
            currentFilePath_, veylo::MediaKind::Video, -1).isEmpty();
        next = !veylo::FolderSequence::adjacentFile(
            currentFilePath_, veylo::MediaKind::Video, 1).isEmpty();
    }

    if (mediaPlayer_ && currentKind_ == veylo::MediaKind::Video) {
        const int chapter = libvlc_media_player_get_chapter(mediaPlayer_);
        const int chapterCount = libvlc_media_player_get_chapter_count(mediaPlayer_);
        previous = previous || veylo::adjacentChapterIndex(chapter, chapterCount, -1).has_value();
        next = next || veylo::adjacentChapterIndex(chapter, chapterCount, 1).has_value();
    }

    if (previous == canPreviousMedia_ && next == canNextMedia_) {
        return;
    }
    canPreviousMedia_ = previous;
    canNextMedia_ = next;
    emit mediaNavigationChanged();
}

void PlayerController::refreshChapters()
{
    QVariantList positions;
    if (mediaPlayer_ && currentKind_ == veylo::MediaKind::Video) {
        libvlc_chapter_description_t **descriptions = nullptr;
        const int count = libvlc_media_player_get_full_chapter_descriptions(
            mediaPlayer_, -1, &descriptions);
        if (count > 0 && descriptions) {
            QList<qint64> offsets;
            offsets.reserve(count);
            for (int index = 0; index < count; ++index) {
                if (descriptions[index]) {
                    offsets.append(static_cast<qint64>(
                        descriptions[index]->i_time_offset));
                }
            }
            for (const qint64 offset : veylo::timelineChapterPositions(offsets, duration_)) {
                positions.append(offset);
            }
            libvlc_chapter_descriptions_release(
                descriptions, static_cast<unsigned>(count));
        }
    }

    if (positions == chapterPositions_) {
        return;
    }
    chapterPositions_ = positions;
    emit chaptersChanged();
}

bool PlayerController::goToAdjacentChapter(int direction)
{
    if (!mediaPlayer_ || currentKind_ != veylo::MediaKind::Video || direction == 0) {
        return false;
    }
    const int chapter = libvlc_media_player_get_chapter(mediaPlayer_);
    const int chapterCount = libvlc_media_player_get_chapter_count(mediaPlayer_);
    const std::optional<int> target = veylo::adjacentChapterIndex(
        chapter, chapterCount, direction);
    if (!target) {
        return false;
    }

    libvlc_media_player_set_chapter(mediaPlayer_, *target);
    QTimer::singleShot(0, this, &PlayerController::updateMediaNavigation);
    return true;
}

void PlayerController::resetTimeline()
{
    ended_ = false;
    pendingSeekPosition_ = -1;
    position_ = 0;
    duration_ = 0;
    if (!chapterPositions_.isEmpty()) {
        chapterPositions_.clear();
        emit chaptersChanged();
    }
    emit positionChanged();
    emit durationChanged();
    setSeekable(false);
}

void PlayerController::setError(const QString &message)
{
    if (errorMessage_ == message) return;
    errorMessage_ = message;
    emit errorMessageChanged();
}

void PlayerController::setLoading(bool loading)
{
    if (loading_ == loading) return;
    loading_ = loading;
    emit loadingChanged();
}

void PlayerController::setPlaying(bool playing)
{
    if (playing_ == playing) return;
    playing_ = playing;
    emit playingChanged();
}

void PlayerController::setSeekable(bool seekable)
{
    if (seekable_ == seekable) {
        return;
    }
    seekable_ = seekable;
    emit seekableChanged();
}

void PlayerController::setCurrentKind(veylo::MediaKind kind)
{
    currentKind_ = kind;
    canPreviousImage_ = false;
    canNextImage_ = false;
    canPreviousMedia_ = false;
    canNextMedia_ = false;
    if (kind != veylo::MediaKind::Image) {
        imageIndex_ = 0;
        imageCount_ = 0;
    }
    emit currentMediaChanged();
    emit imageNavigationChanged();
    emit mediaNavigationChanged();
}

PlayerController::MediaKind PlayerController::toPublicKind(veylo::MediaKind kind)
{
    switch (kind) {
    case veylo::MediaKind::Audio: return Audio;
    case veylo::MediaKind::Video: return Video;
    case veylo::MediaKind::Image: return Image;
    case veylo::MediaKind::None: return NoMedia;
    }
    return NoMedia;
}

QString PlayerController::displayTrackName(const char *name, const QString &fallback)
{
    if (!name || *name == '\0') return fallback;
    const QString decoded = QString::fromUtf8(name).trimmed();
    return decoded.isEmpty() ? fallback : decoded;
}
