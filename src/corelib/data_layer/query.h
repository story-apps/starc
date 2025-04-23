#pragma once

#include <QString>
#include <QUuid>
#include <QVariantList>

namespace DatabaseLayer {

struct Query {
    QUuid uuid;
    QString queryString;
    QVariantList bindValues;
};

} // namespace DatabaseLayer

Q_DECLARE_METATYPE(DatabaseLayer::Query)
