#include "include/global/LocationMappingConfig.hpp"
#include "include/global/Utils.hpp"
#include <QJsonDocument>


bool LocationMappingConfig::loadFromFile(const QString& filePath)
{
	m_mappings.clear();
	skipProfiles.clear();
	apiLink = "";

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		MW_show_log(">>>>>>>> ОШИБКА!!! Файл location_mapping.json не найден!");
		return false;
	}

	QByteArray data = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

	if (parseError.error != QJsonParseError::NoError) {
		MW_show_log(">>>>>>>> Ошибка парсинга location_mapping.json : " + parseError.errorString());
		return false;
	}

	QJsonObject root = doc.object();

	if (root.contains("secret") && root["secret"].isString()) {
		auto temp_link = root["secret"].toString();
		apiLink = DecodeB64IfValid(temp_link);
		if (!QUrl(apiLink).isValid()) {
			MW_show_log(">>>>>>>> ОШИБКА : неверный секрет");
			return false;
		}
	}

	if (apiLink.isEmpty()) {
		MW_show_log(">>>>>>>> ОШИБКА : отсутствует секрет");
		return false;
	}

	if (root.contains("skip") && root["skip"].isArray()) {
		QJsonArray keysArray = root["skip"].toArray();
		for (const QJsonValue& keyValue : keysArray) {
			if (keyValue.isString()) {
				skipProfiles.append(keyValue.toString());
			}
		}
	}

	if (root.contains("mappings") && root["mappings"].isArray()) {
		QJsonArray mappingsArray = root["mappings"].toArray();
		for (const QJsonValue& mappingValue : mappingsArray) {
			if (mappingValue.isObject()) {
				QJsonObject mappingObj = mappingValue.toObject();
				MappingRule rule;

				if (mappingObj.contains("name_pattern") &&
					mappingObj["name_pattern"].isString()) {
					rule.namePattern = mappingObj["name_pattern"].toString();
				}

				if (mappingObj.contains("stat_keys") &&
					mappingObj["stat_keys"].isArray()) {
					QJsonArray keysArray = mappingObj["stat_keys"].toArray();
					for (const QJsonValue& keyValue : keysArray) {
						if (keyValue.isString()) {
							rule.statKeys.append(keyValue.toString());
						}
					}
				}

				if (!rule.namePattern.isEmpty() && !rule.statKeys.isEmpty()) {
					m_mappings.append(rule);
				}
			}
		}
	}
	return true;
}

int LocationMappingConfig::calculateUsersCount(const QString& profileName,
	const QMap<QString, int>& serverStats) const
{
	for (const MappingRule& rule : m_mappings) {
		if (profileName.contains(rule.namePattern)) {
			if (rule.statKeys.isEmpty()) {
				return -1;
			}
			if (rule.statKeys.count() == 1) {
				return serverStats.value(rule.statKeys[0], -1);
			}
			int total = 0;
			for (const QString& statKey : rule.statKeys) {
				total += serverStats.value(statKey, 0);
			}
			return total;
		}
	}
	return -1;
}

QString LocationMappingConfig::calculateUsersCountString(const QString& profileName,
	const QMap<QString, int>& serverStats) const
{
	for (const MappingRule& rule : m_mappings) {
		if (profileName.contains(rule.namePattern)) {
			if (rule.statKeys.isEmpty()) {
				return "Нет данных";
			}
			int total = 0;
			if (rule.statKeys.count() == 1) {
				total = serverStats.value(rule.statKeys[0], -1);
				if (total != -1) return Int2String(total);
				else return "Нет данных";
			}
			int countNA = 0, countZero = 0;
			QString countByServers = "(";
			for (const QString& statKey : rule.statKeys) {
				int value = serverStats.value(statKey, -1);

				if (value != -1) {
					total += value;
					countByServers += Int2String(value) + " + ";
					if (value == 0) countZero++;
				}
				else {
					countByServers += "N/A + ";
					countNA++;
				}
			}
			if (countNA == rule.statKeys.count())  return "Нет данных";
			if (countByServers.endsWith(" + ")) {
				countByServers.resize(countByServers.length() - 3);
			}
			countByServers += ")";
			if (rule.statKeys.count() <= 4) return Int2String(total) + " " + countByServers;

			int countOfValid = rule.statKeys.count() - countNA - countZero;
			QString NAstring = countNA > 0 ? " + N/A x" + QString::number(countNA) : "";
			QString ZeroString = countZero > 0 ? " + 0 x" + QString::number(countZero) : "";
			if (countOfValid != 0) 
				return Int2String(total) + " (≈" + Int2String(total / countOfValid) + " x" + Int2String(countOfValid) + NAstring + ZeroString + ")";
			return Int2String(total);
		}
	}
	return "Нет данных";
}

QStringList LocationMappingConfig::getAllStatKeys() const
{
	QStringList allKeys;
	for (const MappingRule& rule : m_mappings) {
		allKeys.append(rule.statKeys);
	}
	allKeys.removeDuplicates();
	return allKeys;
}

QStringList LocationMappingConfig::getAllNamePatterns() const
{
	QStringList patterns;
	for (const MappingRule& rule : m_mappings) {
		patterns.append(rule.namePattern);
	}
	return patterns;
}

QStringList LocationMappingConfig::findUnmatchedProfiles(const QStringList& profileNames) const
{
	QStringList unmatched;

	for (const QString& profileName : profileNames) {
		bool found = false;
		for (const MappingRule& rule : m_mappings) {
			if (profileName.contains(rule.namePattern)) {
				found = true;
				break;
			}
		}
		if (!found) {
			unmatched.append(profileName);
		}
	}

	return unmatched;
}

QStringList LocationMappingConfig::findMissingStatKeys(const QMap<QString, int>& serverStats) const
{
	QStringList missingKeys;
	QStringList allStatKeys = getAllStatKeys();

	for (const QString& expectedKey : allStatKeys) {
		if (!serverStats.contains(expectedKey)) {
			missingKeys.append(expectedKey);
		}
	}

	return missingKeys;
}

QStringList LocationMappingConfig::getAllMappedStatKeys() const
{
	QStringList allKeys;

	for (const MappingRule& rule : m_mappings) {
		allKeys.append(rule.statKeys);
	}

	allKeys.removeDuplicates();
	allKeys.sort();

	return allKeys;
}

QStringList LocationMappingConfig::findUnusedStatKeys(const QMap<QString, int>& serverStats) const
{
	QStringList unusedKeys;
	QStringList mappedKeys = getAllMappedStatKeys();

	QStringList apiKeys = serverStats.keys();
	apiKeys.sort();

	for (const QString& apiKey : apiKeys) {
		bool isUsed = false;

		for (const QString& mappedKey : mappedKeys) {
			if (apiKey == mappedKey) {
				isUsed = true;
				break;
			}
		}

		if (!isUsed) {
			unusedKeys.append(apiKey);
		}
	}

	return unusedKeys;
}

QStringList LocationMappingConfig::findUnusedMappingRules(
	const QStringList& profileNames,
	const QMap<QString, int>& serverStats) const
{
	QStringList unusedRules;

	for (const MappingRule& rule : m_mappings) {
		bool profileFound = false;
		bool statKeyFound = false;

		for (const QString& profileName : profileNames) {
			if (profileName.contains(rule.namePattern)) {
				profileFound = true;
				break;
			}
		}

		bool allKeysExist = true;
		for (const QString& statKey : rule.statKeys) {
			if (!serverStats.contains(statKey)) {
				allKeysExist = false;
				break;
			}
		}
		statKeyFound = allKeysExist;

		QStringList processedKeys;
		for (int i = 0; i < rule.statKeys.length(); i++) {
			if (serverStats.contains(rule.statKeys[i])) {
				processedKeys << rule.statKeys[i] + "(👤: " + QString::number(serverStats[rule.statKeys[i]]) + ")";
			} else  processedKeys << rule.statKeys[i] + " ❌";
		}

		if (!profileFound || !statKeyFound) {
			QString ruleInfo = QString("\"%1\"%2 -> [%3]")
				.arg(rule.namePattern)
				.arg(!profileFound ? "❌" : "✅")
				.arg(processedKeys.join(", "));
			unusedRules.append(ruleInfo);
		}
	}

	return unusedRules;
}