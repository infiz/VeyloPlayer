#pragma once

#include <QList>
#include <QString>

namespace veylo {

struct TrackOption {
    QString label;
    int id = -1;
    bool external = false;
};

struct SavedTrackPreference {
    QString label;
    int ordinal = 0;
    bool enabled = true;
};

int bestTrackId(const QList<TrackOption> &options,
                const SavedTrackPreference &preference,
                int fallbackId);

QString folderPreferenceIdForMediaPath(const QString &mediaPath);

} // namespace veylo
