#include "TrackPreference.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace veylo {

int bestTrackId(const QList<TrackOption> &options,
                const SavedTrackPreference &preference,
                int fallbackId)
{
    QList<TrackOption> eligible;
    for (const TrackOption &option : options) {
        if (option.external || (preference.enabled ? option.id < 0 : option.id >= 0)) {
            continue;
        }
        eligible.append(option);
    }
    if (eligible.isEmpty()) {
        return fallbackId;
    }

    if (!preference.label.isEmpty()) {
        const auto match = std::find_if(eligible.cbegin(), eligible.cend(),
                                        [&preference](const TrackOption &option) {
            return option.label.compare(preference.label, Qt::CaseInsensitive) == 0;
        });
        if (match != eligible.cend()) {
            return match->id;
        }
    }

    const int ordinal = std::clamp(
        preference.ordinal, 0, static_cast<int>(eligible.size()) - 1);
    return eligible.at(ordinal).id;
}

QString folderPreferenceIdForMediaPath(const QString &mediaPath)
{
    const QFileInfo mediaInfo(mediaPath);
    QFileInfo folderInfo(mediaInfo.absolutePath());
    QString folderPath = folderInfo.canonicalFilePath();
    if (folderPath.isEmpty()) {
        folderPath = folderInfo.absoluteFilePath();
    }
    folderPath = QDir::cleanPath(folderPath);
#if defined(Q_OS_WIN)
    folderPath = folderPath.toCaseFolded();
#endif
    return QString::fromLatin1(QCryptographicHash::hash(
        folderPath.toUtf8(), QCryptographicHash::Sha256).toHex());
}

} // namespace veylo
