#pragma once

#include <QGuiApplication>
#include <QString>
#include <QStringList>

class Application final : public QGuiApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    QStringList takePendingFileOpenRequests();

signals:
    void fileOpenRequested(const QString &path);

protected:
    bool event(QEvent *event) override;

private:
    QStringList m_pendingFileOpenRequests;
    bool m_fileOpenHandlerReady = false;
};
