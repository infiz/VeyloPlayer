#pragma once

#include <QString>
#include <QStringList>

namespace veylo {

enum class MediaKind {
    None,
    Audio,
    Video,
    Image,
};

MediaKind mediaKindForPath(const QString &path);
bool isAudioOrVideo(MediaKind kind);
QStringList audioExtensions();
QStringList videoExtensions();
QStringList imageExtensions();
QStringList subtitleExtensions();

} // namespace veylo
