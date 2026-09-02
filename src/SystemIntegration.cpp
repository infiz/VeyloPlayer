#include "SystemIntegration.h"

#include <QDesktopServices>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>

namespace {

#ifdef Q_OS_WIN
bool hasWindowsRegistration(const QString &registryPath)
{
    const QSettings settings(registryPath, QSettings::NativeFormat);
    return settings.contains(QStringLiteral("VeyloPlayer"));
}
#endif

} // namespace

SystemIntegration::SystemIntegration(QObject *parent)
    : QObject(parent)
{
}

bool SystemIntegration::defaultPlayerRequestInProgress() const
{
    return defaultPlayerRequestInProgress_;
}

void SystemIntegration::requestDefaultPlayer()
{
    if (defaultPlayerRequestInProgress_) {
        return;
    }

    defaultPlayerRequestInProgress_ = true;
    emit defaultPlayerRequestInProgressChanged();

#ifdef Q_OS_WIN
    QUrl settingsUrl(QStringLiteral("ms-settings:defaultapps"));
    QUrlQuery query;
    if (hasWindowsRegistration(
            QStringLiteral("HKEY_CURRENT_USER\\Software\\RegisteredApplications"))) {
        query.addQueryItem(QStringLiteral("registeredAppUser"),
                           QStringLiteral("VeyloPlayer"));
    } else if (hasWindowsRegistration(
                   QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\RegisteredApplications"))) {
        query.addQueryItem(QStringLiteral("registeredAppMachine"),
                           QStringLiteral("VeyloPlayer"));
    }
    if (!query.isEmpty()) {
        settingsUrl.setQuery(query);
    }

    const bool opened = QDesktopServices::openUrl(settingsUrl);
    completeDefaultPlayerRequest(
        opened,
        opened
            ? tr("Windows Settings is open. Choose VeyloPlayer for the media file types you want it to open by default.")
            : tr("Windows Settings could not be opened. Open Settings > Apps > Default apps and choose VeyloPlayer."));
#elif defined(Q_OS_MACOS)
    requestMacDefaultPlayer();
#else
    completeDefaultPlayerRequest(
        false, tr("Setting the default player is not supported on this operating system."));
#endif
}

void SystemIntegration::completeDefaultPlayerRequest(bool success, const QString &message)
{
    defaultPlayerRequestInProgress_ = false;
    emit defaultPlayerRequestInProgressChanged();
    emit defaultPlayerRequestFinished(success, message);
}
