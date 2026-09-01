#pragma once

#include <QList>

#include <optional>

namespace veylo {

std::optional<int> adjacentChapterIndex(int currentChapter,
                                        int chapterCount,
                                        int direction);

QList<qint64> timelineChapterPositions(const QList<qint64> &offsets,
                                       qint64 duration);

} // namespace veylo
