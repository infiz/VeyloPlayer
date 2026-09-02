#pragma once

#include <QList>
#include <QRect>
#include <QSize>

namespace veylo {

QRect visibleWindowGeometry(const QRect &savedGeometry,
                            const QList<QRect> &availableScreenGeometries,
                            const QSize &minimumSize);

}
