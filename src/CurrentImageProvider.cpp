#include "CurrentImageProvider.h"

#include "PlayerController.h"

#include <QImageReader>

CurrentImageProvider::CurrentImageProvider(const PlayerController *controller)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , controller_(controller)
{
}

QImage CurrentImageProvider::requestImage(const QString &id,
                                          QSize *size,
                                          const QSize &requestedSize)
{
    Q_UNUSED(id)
    if (!controller_ || controller_->currentFilePath().isEmpty()) {
        return {};
    }
    QImageReader reader(controller_->currentFilePath());
    reader.setAutoTransform(true);
    const QSize originalSize = reader.size();
    if (size) {
        *size = originalSize;
    }
    if (requestedSize.isValid() && originalSize.isValid()) {
        const QSize scaledSize = originalSize.scaled(requestedSize, Qt::KeepAspectRatio);
        if (scaledSize.width() < originalSize.width()
            || scaledSize.height() < originalSize.height()) {
            reader.setScaledSize(scaledSize);
        }
    }
    return reader.read();
}
