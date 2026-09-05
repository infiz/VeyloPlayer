#include "LegalInfo.h"

#include <QFile>

LegalInfo::LegalInfo(QObject *parent)
    : QObject(parent)
    , licenseText_(readTextResource(QStringLiteral(":/legal/LICENSE")))
    , thirdPartyNoticesText_(
          readTextResource(QStringLiteral(":/legal/THIRD_PARTY_NOTICES.md")))
{
}

QString LegalInfo::licenseText() const
{
    return licenseText_;
}

QString LegalInfo::thirdPartyNoticesText() const
{
    return thirdPartyNoticesText_;
}

QString LegalInfo::readTextResource(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("The requested legal document could not be loaded.");
    }
    return QString::fromUtf8(file.readAll());
}
