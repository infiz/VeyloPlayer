#include "MediaFile.h"

#include <QFileInfo>
#include <QSet>

namespace {

const QSet<QString> &audioExtensionSet()
{
    static const QSet<QString> extensions = {
        QStringLiteral("mp3"), QStringLiteral("m4a"), QStringLiteral("aac"),
        QStringLiteral("wav"), QStringLiteral("flac"), QStringLiteral("ogg"),
    };
    return extensions;
}

const QSet<QString> &videoExtensionSet()
{
    static const QSet<QString> extensions = {
        QStringLiteral("mp4"), QStringLiteral("m4v"), QStringLiteral("mov"),
        QStringLiteral("mkv"), QStringLiteral("webm"), QStringLiteral("avi"),
    };
    return extensions;
}

const QSet<QString> &imageExtensionSet()
{
    static const QSet<QString> extensions = {
        QStringLiteral("jpg"), QStringLiteral("jpeg"),
    };
    return extensions;
}

QStringList sortedValues(const QSet<QString> &values)
{
    QStringList result(values.cbegin(), values.cend());
    result.sort(Qt::CaseInsensitive);
    return result;
}

} // namespace

namespace veylo {

MediaKind mediaKindForPath(const QString &path)
{
    const QString extension = QFileInfo(path).suffix().toCaseFolded();
    if (audioExtensionSet().contains(extension)) {
        return MediaKind::Audio;
    }
    if (videoExtensionSet().contains(extension)) {
        return MediaKind::Video;
    }
    if (imageExtensionSet().contains(extension)) {
        return MediaKind::Image;
    }
    return MediaKind::None;
}

bool isAudioOrVideo(MediaKind kind)
{
    return kind == MediaKind::Audio || kind == MediaKind::Video;
}

QStringList audioExtensions()
{
    return sortedValues(audioExtensionSet());
}

QStringList videoExtensions()
{
    return sortedValues(videoExtensionSet());
}

QStringList imageExtensions()
{
    return sortedValues(imageExtensionSet());
}

QStringList subtitleExtensions()
{
    return {
        QStringLiteral("ass"), QStringLiteral("srt"),
        QStringLiteral("ssa"), QStringLiteral("vtt"),
    };
}

} // namespace veylo
