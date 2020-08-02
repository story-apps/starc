#include "kit_scenarist_importer.h"

#include "import_options.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariantMap>

namespace {
    const QString kSqlDriver = "QSQLITE";
    const QString kConnectionName = "import_database";
}


namespace BusinessLayer
{

AbstractImporter::Documents KitScenaristImporter::importDocuments(const ImportOptions& _options) const
{
    return {};
}

QVector<AbstractImporter::Screenplay> KitScenaristImporter::importScreenplays(const ImportOptions& _options) const
{
    QString sourceScreenplayXml;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(kSqlDriver, kConnectionName);
        database.setDatabaseName(_options.filePath);
        if (database.open()) {
            QSqlQuery query(database);
            query.exec("SELECT text FROM scenario WHERE is_draft = 0");
            query.next();
            sourceScreenplayXml = query.record().value("text").toString();
        }
    }
    QSqlDatabase::removeDatabase(kConnectionName);

    Screenplay result;
    return { result };
}

} // namespace BusinessLayer
