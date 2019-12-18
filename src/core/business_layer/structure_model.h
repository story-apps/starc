#pragma once

#include <QAbstractItemModel>

namespace Domain {
    class DocumentObject;
}


namespace BusinessLayer
{

/**
 * @brief Модель структуры документов проекта
 */
class Structure : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit Structure(QObject* _parent = nullptr);
    ~Structure() override;

    /**
     * @brief Задать документ со структурой
     */
    void setDocument(Domain::DocumentObject* _document);

    /**
     * @brief Реализация древовидной модели
     */
    /** @{ */
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount( const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex &_parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex &_index) const override;
    QVariant data(const QModelIndex &_index, int _role) const override;
    //! Реализация перетаскивания элементов
//    bool canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent = {}) const override;
//    bool dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent = {}) override;
//    QMimeData* mimeData(const QModelIndexList& _indexes) const override;
//    QStringList mimeTypes() const override;
//    Qt::DropActions supportedDragActions() const override;
//    Qt::DropActions supportedDropActions() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
