#pragma once

#include <QObject>
#include <QString>

class SystemIntegration final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool defaultPlayerRequestInProgress
               READ defaultPlayerRequestInProgress
               NOTIFY defaultPlayerRequestInProgressChanged)

public:
    explicit SystemIntegration(QObject *parent = nullptr);

    bool defaultPlayerRequestInProgress() const;
    Q_INVOKABLE void requestDefaultPlayer();

signals:
    void defaultPlayerRequestInProgressChanged();
    void defaultPlayerRequestFinished(bool success, const QString &message);

private:
    void completeDefaultPlayerRequest(bool success, const QString &message);

#ifdef Q_OS_MACOS
    void requestMacDefaultPlayer();
#endif

    bool defaultPlayerRequestInProgress_ = false;
};
