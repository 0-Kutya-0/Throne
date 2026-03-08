#pragma once
#include <QObject>
#include <QMap>
#include <QStringList>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class LocationMappingConfig
{
public:
    struct MappingRule {
        QString namePattern;
        QStringList statKeys;
    };

    bool loadFromFile(const QString& filePath);

    int calculateUsersCount(const QString& profileName,
        const QMap<QString, int>& serverStats) const;

    QString calculateUsersCountString(const QString& profileName,
        const QMap<QString, int>& serverStats) const;

    QStringList getAllStatKeys() const;
    QStringList getAllNamePatterns() const;

    QStringList findUnmatchedProfiles(const QStringList& profileNames) const;
    QStringList findMissingStatKeys(const QMap<QString, int>& serverStats) const;

    QStringList findUnusedStatKeys(const QMap<QString, int>& serverStats) const;

    QStringList getAllMappedStatKeys() const;

    QStringList findUnusedMappingRules(const QStringList& profileNames,
        const QMap<QString, int>& serverStats) const;

    QStringList skipProfiles;

    QString apiLink;

private:
    QList<MappingRule> m_mappings;
};