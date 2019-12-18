#include "structure_model.h"

#include <domain/document_object.h>


namespace BusinessLayer
{

class Structure::Implementation
{
public:
    /**
     * @brief Перестроить модель структуры
     */
    void rebuildModel();

    Domain::DocumentObject* structure = nullptr;
};


// ****


Structure::Structure(QObject* _parent)
    : QAbstractItemModel(_parent),
      d(new Implementation)
{
}

void Structure::setDocument(Domain::DocumentObject* _document)
{
    if (d->structure == _document) {
        return;
    }

    d->structure = _document;
    d->rebuildModel();
}

QModelIndex Structure::index(int _row, int _column, const QModelIndex& _parent) const
{

}

QModelIndex Structure::parent(const QModelIndex& _child) const
{

}

int Structure::columnCount(const QModelIndex& _parent) const
{

}

int Structure::rowCount(const QModelIndex& _parent) const
{

}

Qt::ItemFlags Structure::flags(const QModelIndex& _index) const
{

}

QVariant Structure::data(const QModelIndex& _index, int _role) const
{

}

//bool Structure::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent) const
//{

//}

//bool Structure::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent)
//{

//}

//QMimeData* Structure::mimeData(const QModelIndexList& _indexes) const
//{

//}

//QStringList Structure::mimeTypes() const
//{

//}

//Qt::DropActions Structure::supportedDragActions() const
//{

//}

//Qt::DropActions Structure::supportedDropActions() const
//{

//}

Structure::~Structure() = default;

} // namespace BusinessLayer
