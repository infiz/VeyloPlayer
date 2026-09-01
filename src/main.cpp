#include "Application.h"
#include "CurrentImageProvider.h"
#include "PlayerController.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QTimer>
#include <QWindow>

#ifdef Q_OS_WIN
#include <shobjidl.h>
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    SetCurrentProcessExplicitAppUserModelID(L"org.veyloplayer.app");
#endif

    QCoreApplication::setOrganizationName(QStringLiteral("VeyloPlayer"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("veyloplayer.org"));
    QCoreApplication::setApplicationName(QStringLiteral("VeyloPlayer"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(VEYLO_APP_VERSION));

    Application application(argc, argv);
    QSettings().remove(QStringLiteral("appearance"));
    const QIcon applicationIcon(QStringLiteral(":/branding/veylo-player.png"));
    application.setWindowIcon(applicationIcon);
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("A modern open-source media player"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Local media file to open."));
    parser.process(application);

    PlayerController player;
    qmlRegisterSingletonInstance("Veylo.Core", 1, 0, "Player", &player);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("current"), new CurrentImageProvider(&player));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &application, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    QObject::connect(&application, &Application::fileOpenRequested,
                     &player, &PlayerController::openFile);
    engine.loadFromModule("Veylo.UI", "Main");
    for (QObject *rootObject : engine.rootObjects()) {
        if (auto *window = qobject_cast<QWindow *>(rootObject)) {
            window->setIcon(applicationIcon);
        }
    }

    QString startupFile;
    const QStringList pendingFileRequests = application.takePendingFileOpenRequests();
    if (!pendingFileRequests.isEmpty()) {
        startupFile = pendingFileRequests.constLast();
    } else if (!parser.positionalArguments().isEmpty()) {
        startupFile = parser.positionalArguments().constFirst();
    }
    if (!startupFile.isEmpty()) {
        const QString startupPath = QFileInfo(startupFile).absoluteFilePath();
        QTimer::singleShot(0, &player, [&player, startupPath] { player.openFile(startupPath); });
    }
    return application.exec();
}
