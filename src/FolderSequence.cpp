#include "FolderSequence.h"

#include "NaturalSort.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>

namespace {

bool pathsMatch(const QString &left, const QString &right)
{
#if defined(Q_OS_WIN)
    return QDir::cleanPath(left).compare(QDir::cleanPath(right), Qt::CaseInsensitive) == 0;
#else
    return QDir::cleanPath(left) == QDir::cleanPath(right);
#endif
}

bool matchesCategory(const QString &path, veylo::MediaKind category)
{
    const veylo::MediaKind kind = veylo::mediaKindForPath(path);
    return category == veylo::MediaKind::Image
        ? kind == veylo::MediaKind::Image
        : veylo::isAudioOrVideo(kind);
}

} // namespace

namespace veylo {

QStringList FolderSequence::filesFor(const QString &currentPath, MediaKind category)
{
    const QFileInfo currentInfo(currentPath);
    const QDir directory = currentInfo.dir();
    const QFileInfoList entries = directory.entryInfoList(
        QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
        QDir::NoSort);

    QStringList result;
    for (const QFileInfo &entry : entries) {
        if (matchesCategory(entry.filePath(), category)) {
            result.append(entry.absoluteFilePath());
        }
    }

    std::sort(result.begin(), result.end(), [](const QString &left, const QString &right) {
        const QFileInfo leftInfo(left);
        const QFileInfo rightInfo(right);
        const int nameComparison = naturalCompare(leftInfo.fileName(), rightInfo.fileName());
        if (nameComparison != 0) {
            return nameComparison < 0;
        }
        return left < right;
    });
    return result;
}

QStringList FolderSequence::recursiveFiles(const QString &folderPath, MediaKind category)
{
    const QFileInfo folderInfo(folderPath);
    if (!folderInfo.exists() || !folderInfo.isDir() || !folderInfo.isReadable()) {
        return {};
    }

    const QDir root(folderInfo.absoluteFilePath());
    QDirIterator iterator(
        root.absolutePath(),
        QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories);

    QStringList result;
    while (iterator.hasNext()) {
        const QString path = iterator.next();
        if (matchesCategory(path, category)) {
            result.append(QFileInfo(path).absoluteFilePath());
        }
    }

    std::sort(result.begin(), result.end(), [&root](const QString &left, const QString &right) {
        const QString leftRelative = QDir::fromNativeSeparators(root.relativeFilePath(left));
        const QString rightRelative = QDir::fromNativeSeparators(root.relativeFilePath(right));
        const int comparison = naturalCompare(leftRelative, rightRelative);
        if (comparison != 0) {
            return comparison < 0;
        }
        return left < right;
    });
    return result;
}

QString FolderSequence::adjacentFile(const QString &currentPath, MediaKind category, int direction)
{
    if (direction == 0 || currentPath.isEmpty()) {
        return {};
    }

    const QStringList files = filesFor(currentPath, category);
    for (qsizetype index = 0; index < files.size(); ++index) {
        if (!pathsMatch(files.at(index), currentPath)) {
            continue;
        }
        const qsizetype adjacent = index + (direction < 0 ? -1 : 1);
        if (adjacent >= 0 && adjacent < files.size()) {
            return files.at(adjacent);
        }
        break;
    }
    return {};
}

} // namespace veylo
