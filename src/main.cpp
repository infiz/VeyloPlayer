#include "Application.h"
#include "CurrentImageProvider.h"
#include "PlayerController.h"
#include "WindowPlacement.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QScreen>
#include <QTimer>
#include <QWindow>

#include <memory>

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

            QSettings settings;
            const QRect savedGeometry = settings.value(
                QStringLiteral("window/normalGeometry")).toRect();
            if (savedGeometry.isValid()) {
                QList<QRect> availableScreenGeometries;
                for (const QScreen *screen : application.screens()) {
                    availableScreenGeometries.append(screen->availableGeometry());
                }
                window->setGeometry(veylo::visibleWindowGeometry(
                    savedGeometry, availableScreenGeometries,
                    QSize(window->minimumWidth(), window->minimumHeight())));
            }

            struct RememberedWindowState {
                QRect normalGeometry;
                QWindow::Visibility visibility = QWindow::Windowed;
            };
            const auto remembered = std::make_shared<RememberedWindowState>();
            remembered->normalGeometry = savedGeometry.isValid()
                ? window->geometry() : QRect();
            if (settings.value(QStringLiteral("window/fullScreen"), false).toBool()) {
                remembered->visibility = QWindow::FullScreen;
            } else if (settings.value(QStringLiteral("window/maximized"), false).toBool()) {
                remembered->visibility = QWindow::Maximized;
            }

            const auto rememberNormalGeometry = [window, remembered] {
                if (window->windowState() == Qt::WindowNoState
                    && window->visibility() != QWindow::Hidden) {
                    remembered->normalGeometry = window->geometry();
                }
            };
            auto *geometryTimer = new QTimer(window);
            geometryTimer->setSingleShot(true);
            geometryTimer->setInterval(200);
            QObject::connect(geometryTimer, &QTimer::timeout, window, rememberNormalGeometry);
            const auto scheduleGeometryRemember = [geometryTimer] { geometryTimer->start(); };
            QObject::connect(window, &QWindow::xChanged, window, scheduleGeometryRemember);
            QObject::connect(window, &QWindow::yChanged, window, scheduleGeometryRemember);
            QObject::connect(window, &QWindow::widthChanged, window, scheduleGeometryRemember);
            QObject::connect(window, &QWindow::heightChanged, window, scheduleGeometryRemember);
            QObject::connect(window, &QWindow::windowStateChanged, window,
                             [remembered, geometryTimer](Qt::WindowState state) {
                if (state == Qt::WindowNoState) {
                    remembered->visibility = QWindow::Windowed;
                    geometryTimer->start();
                } else if (state == Qt::WindowMaximized) {
                    remembered->visibility = QWindow::Maximized;
                } else if (state == Qt::WindowFullScreen) {
                    remembered->visibility = QWindow::FullScreen;
                }
            });
            QObject::connect(&application, &QCoreApplication::aboutToQuit, window,
                             [window, remembered] {
                if (window->windowState() == Qt::WindowNoState) {
                    remembered->normalGeometry = window->geometry();
                }
                QSettings settings;
                if (remembered->normalGeometry.isValid()) {
                    settings.setValue(QStringLiteral("window/normalGeometry"),
                                      remembered->normalGeometry);
                }
                settings.setValue(QStringLiteral("window/maximized"),
                                  remembered->visibility == QWindow::Maximized);
                settings.setValue(QStringLiteral("window/fullScreen"),
                                  remembered->visibility == QWindow::FullScreen);
            });

            if (remembered->visibility == QWindow::FullScreen) {
                window->showFullScreen();
            } else if (remembered->visibility == QWindow::Maximized) {
                window->showMaximized();
            } else {
                window->show();
            }
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
