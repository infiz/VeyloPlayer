#include "Application.h"

#include <QEvent>
#include <QFileOpenEvent>

Application::Application(int &argc, char **argv)
    : QGuiApplication(argc, argv)
{
}

QStringList Application::takePendingFileOpenRequests()
{
    m_fileOpenHandlerReady = true;
    QStringList requests;
    requests.swap(m_pendingFileOpenRequests);
    return requests;
}

bool Application::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        const auto *fileEvent = static_cast<QFileOpenEvent *>(event);
        if (!fileEvent->file().isEmpty()) {
            if (m_fileOpenHandlerReady) {
                emit fileOpenRequested(fileEvent->file());
            } else {
                m_pendingFileOpenRequests.append(fileEvent->file());
            }
            return true;
        }
    }
    return QGuiApplication::event(event);
}
