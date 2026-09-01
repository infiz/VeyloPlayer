#include "ChapterNavigation.h"

#include <algorithm>

namespace veylo {

std::optional<int> adjacentChapterIndex(int currentChapter,
                                        int chapterCount,
                                        int direction)
{
    if (direction == 0 || currentChapter < 0 || chapterCount <= 1
        || currentChapter >= chapterCount) {
        return std::nullopt;
    }
    const int target = currentChapter + (direction < 0 ? -1 : 1);
    if (target < 0 || target >= chapterCount) {
        return std::nullopt;
    }
    return target;
}

QList<qint64> timelineChapterPositions(const QList<qint64> &offsets,
                                       qint64 duration)
{
    QList<qint64> positions;
    positions.reserve(offsets.size());
    for (const qint64 offset : offsets) {
        if (offset > 0 && (duration <= 0 || offset < duration)) {
            positions.append(offset);
        }
    }
    std::sort(positions.begin(), positions.end());
    positions.erase(std::unique(positions.begin(), positions.end()), positions.end());
    return positions;
}

} // namespace veylo
