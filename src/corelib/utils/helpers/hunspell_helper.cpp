#include "hunspell_helper.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>


QString HunspellHelper::validateDictionaryFiles(const QString& _affPath, const QString& _dicPath)
{
    // 1. Validate .aff file
    auto validationResult = validateAffFile(_affPath);
    if (!validationResult.isEmpty()) {
        return validationResult;
    }

    // 2. Validate .dic file
    validationResult = validateDicFile(_dicPath);
    if (!validationResult.isEmpty()) {
        return validationResult;
    }

    // No errors found, return an empty string
    return {};
}

QString HunspellHelper::validateAffFile(const QString& _affPath)
{
    QFile affFile(_affPath);
    if (!affFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Failed to open .aff file: %1").arg(affFile.errorString());
    }

    QTextStream affStream(&affFile);
    bool hasSetDirective = false;

    // Read line by line until the encoding directive is found
    while (!affStream.atEnd()) {
        QString line = affStream.readLine().trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // Check if the line starts with the "SET " directive
        if (line.startsWith("SET ", Qt::CaseSensitive)) {
            hasSetDirective = true;
            break;
        }
    }
    affFile.close();

    if (!hasSetDirective) {
        return "Invalid .aff format: Missing the mandatory encoding directive 'SET ...'.";
    }

    return {};
}

QString HunspellHelper::validateDicFile(const QString& _dicPath)
{
    QFile dicFile(_dicPath);
    if (!dicFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Failed to open .dic file: %1").arg(dicFile.errorString());
    }

    QTextStream dicStream(&dicFile);
    QString firstLine = dicStream.readLine().trimmed();
    dicFile.close();

    if (firstLine.isEmpty()) {
        return "The .dic file is empty.";
    }

    // Regex: checks if the first line consists only of digits (word count)
    static const QRegularExpression numRegex("^\\d+$");
    if (!numRegex.match(firstLine).hasMatch()) {
        return QString(
                   "Invalid .dic format: The first line must be a number (word count). Found: '%1'")
            .arg(firstLine);
    }

    return {};
}
