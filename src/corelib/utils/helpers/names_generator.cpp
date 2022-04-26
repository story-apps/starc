#include "names_generator.h"

#include <QBuffer>
#include <QHash>
#include <QRandomGenerator>
#include <QSet>
#include <qtzip/QtZipReader>


class NamesGenerator::Implementation
{
public:
    struct NameInfo {
        bool isMale = true;
        QString name;
    };

    /**
     * @brief <Country, NameInfo>
     */
    QHash<QString, QVector<NameInfo>> database;
};


// ****


const QVector<QString> NamesGenerator::types()
{
    QVector<QString> types;
    const auto database = instance().d->database;
    for (auto iter = database.constBegin(); iter != database.constEnd(); ++iter) {
        types.append(iter.key());
    }
    std::sort(types.begin(), types.end());
    return types;
}

QString NamesGenerator::generate(const QString& _type, int _gender)
{
    const auto database = instance().d->database;
    if (database.isEmpty()) {
        return {};
    }

    auto type = _type;
    if (type.isEmpty() || !database.contains(type)) {
        const auto types = NamesGenerator::types();
        type = types.at(QRandomGenerator::global()->bounded(0, types.size()));
    }

    auto names = database.value(type);
    if (_gender == 1) {
        std::remove_if(names.begin(), names.end(),
                       [](const Implementation::NameInfo& _info) { return !_info.isMale; });
    } else if (_gender == 2) {
        std::remove_if(names.begin(), names.end(),
                       [](const Implementation::NameInfo& _info) { return _info.isMale; });
    }
    return names.value(QRandomGenerator::global()->bounded(0, names.size())).name;
}

NamesGenerator::~NamesGenerator() = default;

NamesGenerator::NamesGenerator()
    : d(new Implementation)
{
    QtZipReader databaseFileReader(":/data/names_database");
    auto databaseFileData = databaseFileReader.fileData("names_database");
    if (databaseFileData.isEmpty()) {
        return;
    }

    QBuffer databaseFileStream(&databaseFileData);
    if (!databaseFileStream.open(QIODevice::ReadOnly)) {
        return;
    }

    QString typeOfNames;
    QVector<Implementation::NameInfo> names;
    while (!databaseFileStream.atEnd()) {
        const auto textLine = databaseFileStream.readLine().trimmed();
        if (textLine.isEmpty()) {
            continue;
        }

        switch (textLine.at(0)) {
        case 'N': {
            if (!names.isEmpty()) {
                d->database[typeOfNames] = names;
                typeOfNames.clear();
                names.clear();
            }

            typeOfNames = textLine.mid(1);
            break;
        }

        case 'M': {
            names.append({ true, textLine.mid(1) });
            break;
        }

        case 'F': {
            names.append({ false, textLine.mid(1) });
            break;
        }

        default: {
            break;
        }
        }
    }
}

const NamesGenerator& NamesGenerator::instance()
{
    static NamesGenerator database;
    return database;
}
