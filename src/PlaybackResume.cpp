#include "PlaybackResume.h"

#include <algorithm>

namespace {

constexpr qint64 minimumResumePositionMs = 5'000;
constexpr qint64 endMarginMs = 10'000;

} // namespace

namespace veylo {

qint64 normalizedResumePosition(qint64 positionMs, qint64 durationMs)
{
    const qint64 position = std::max<qint64>(positionMs, 0);
    if (position < minimumResumePositionMs) {
        return 0;
    }
    if (durationMs > 0 && durationMs - position <= endMarginMs) {
        return 0;
    }
    return position;
}

} // namespace veylo
