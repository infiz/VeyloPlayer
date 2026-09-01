#pragma once

#include <QQuickImageProvider>

class PlayerController;

class CurrentImageProvider final : public QQuickImageProvider
{
public:
    explicit CurrentImageProvider(const PlayerController *controller);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    const PlayerController *controller_;
};
