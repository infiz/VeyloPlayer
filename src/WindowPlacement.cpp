#include "WindowPlacement.h"

#include <algorithm>

namespace veylo {

QRect visibleWindowGeometry(const QRect &savedGeometry,
                            const QList<QRect> &availableScreenGeometries,
                            const QSize &minimumSize)
{
    if (!savedGeometry.isValid() || availableScreenGeometries.isEmpty()) {
        return savedGeometry;
    }

    const QRect *targetScreen = &availableScreenGeometries.constFirst();
    qint64 largestVisibleArea = 0;
    for (const QRect &screenGeometry : availableScreenGeometries) {
        const QRect intersection = savedGeometry.intersected(screenGeometry);
        const qint64 visibleArea = intersection.isValid()
            ? static_cast<qint64>(intersection.width()) * intersection.height()
            : 0;
        if (visibleArea > largestVisibleArea) {
            largestVisibleArea = visibleArea;
            targetScreen = &screenGeometry;
        }
    }

    QSize restoredSize = savedGeometry.size().expandedTo(minimumSize);
    restoredSize.setWidth(std::min(restoredSize.width(), targetScreen->width()));
    restoredSize.setHeight(std::min(restoredSize.height(), targetScreen->height()));

    QPoint restoredPosition = savedGeometry.topLeft();
    if (largestVisibleArea == 0) {
        restoredPosition.setX(targetScreen->x() + (targetScreen->width() - restoredSize.width()) / 2);
        restoredPosition.setY(targetScreen->y() + (targetScreen->height() - restoredSize.height()) / 2);
    } else {
        restoredPosition.setX(std::clamp(restoredPosition.x(), targetScreen->left(),
                                         targetScreen->right() - restoredSize.width() + 1));
        restoredPosition.setY(std::clamp(restoredPosition.y(), targetScreen->top(),
                                         targetScreen->bottom() - restoredSize.height() + 1));
    }

    return QRect(restoredPosition, restoredSize);
}

}
