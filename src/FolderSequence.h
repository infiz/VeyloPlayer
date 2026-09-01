#pragma once

#include "MediaFile.h"

#include <QString>
#include <QStringList>

namespace veylo {

class FolderSequence
{
public:
    static QStringList filesFor(const QString &currentPath, MediaKind category);
    static QStringList recursiveFiles(const QString &folderPath, MediaKind category);
    static QString adjacentFile(const QString &currentPath, MediaKind category, int direction);
};

} // namespace veylo
