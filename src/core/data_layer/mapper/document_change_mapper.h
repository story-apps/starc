#pragma once

#include "abstract_mapper.h"

namespace Domain {
    class DocumentChangeObject;
}


namespace DataMappingLayer
{

/**
 * @brief Отображатель данных изменений документов из БД в объекты
 */
class DocumentChangeMapper : public AbstractMapper
{
public:
    Domain::DocumentChangeObject* find(const Domain::Identifier& _id);
    Domain::DocumentChangeObject* find(const QUuid& _uuid);
    Domain::DocumentChangeObject* findLast(int _size);

    void insert(Domain::DocumentChangeObject* _object);
    bool update(Domain::DocumentChangeObject* _object);
    void remove(Domain::DocumentChangeObject* _object);

protected:
    QString findStatement(const Domain::Identifier& _id) const override;
    QString findAllStatement() const override;
    QString insertStatement(Domain::DomainObject* _object, QVariantList& _insertValues) const override;
    QString updateStatement(Domain::DomainObject* _object, QVariantList& _updateValues) const override;
    QString deleteStatement(Domain::DomainObject* _object, QVariantList& _deleteValues) const override;

protected:
    Domain::DomainObject* doLoad(const Domain::Identifier& _id, const QSqlRecord& _record) override;
    void doLoad(Domain::DomainObject* _object, const QSqlRecord& _record) override;

private:
    DocumentChangeMapper() = default;
    friend class MapperFacade;
};

} // namespace DataMappingLayer

