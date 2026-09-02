#pragma once

#include <QObject>
#include <QString>

class LegalInfo final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString licenseText READ licenseText CONSTANT)
    Q_PROPERTY(QString thirdPartyNoticesText READ thirdPartyNoticesText CONSTANT)

public:
    explicit LegalInfo(QObject *parent = nullptr);

    QString licenseText() const;
    QString thirdPartyNoticesText() const;

private:
    static QString readTextResource(const QString &path);

    QString licenseText_;
    QString thirdPartyNoticesText_;
};
