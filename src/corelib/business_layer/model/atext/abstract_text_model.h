#pragma once

#include <business_layer/model/abstract_model.h>

namespace BusinessLayer {

class AbstractTextModelItem;
class TextModel;

/**
 * @brief Модель текста сценария
 */
class CORE_LIBRARY_EXPORT AbstractTextModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit AbstractTextModel(QObject* _parent = nullptr);
    ~AbstractTextModel() override;

    /**
     * @brief Добавить элемент в конец
     */
    void appendItem(AbstractTextModelItem* _item, AbstractTextModelItem* _parentItem = nullptr);
    void appendItems(const QVector<AbstractTextModelItem*>& _items,
                     AbstractTextModelItem* _parentItem = nullptr);

    /**
     * @brief Добавить элемент в начало
     */
    void prependItem(AbstractTextModelItem* _item, AbstractTextModelItem* _parentItem = nullptr);

    /**
     * @brief Вставить элемент после заданного
     */
    void insertItem(AbstractTextModelItem* _item, AbstractTextModelItem* _afterSiblingItem);
    void insertItems(const QVector<AbstractTextModelItem*>& _items,
                     AbstractTextModelItem* _afterSiblingItem);

    /**
     * @brief Извлечь заданный элемент без удаления
     */
    void takeItem(AbstractTextModelItem* _item, AbstractTextModelItem* _parentItem = nullptr);
    void takeItems(AbstractTextModelItem* _fromItem, AbstractTextModelItem* _toItem,
                   AbstractTextModelItem* _parentItem = nullptr);

    /**
     * @brief Удалить заданный элемент
     */
    void removeItem(AbstractTextModelItem* _item);
    void removeItems(AbstractTextModelItem* _fromItem, AbstractTextModelItem* _toItem);

    /**
     * @brief Обновить заданный элемент
     */
    void updateItem(AbstractTextModelItem* _item);

    /**
     * @brief Реализация древовидной модели
     */
    /** @{ */
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount(const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex& _parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& _index) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;
    //! Реализация перетаскивания элементов
    bool canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column,
                         const QModelIndex& _parent = {}) const override;
    bool dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column,
                      const QModelIndex& _parent = {}) override;
    QMimeData* mimeData(const QModelIndexList& _indexes) const override;
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;
    /** @} */

    /**
     * @brief Сформировать mime-данные сценария в заданном диапазоне
     */
    QString mimeFromSelection(const QModelIndex& _from, int _fromPosition, const QModelIndex& _to,
                              int _toPosition, bool _clearUuid) const;

    /**
     * @brief Вставить контент из mime-данных со сценарием в заданной позиции
     */
    void insertFromMime(const QModelIndex& _index, int _positionInBlock, const QString& _mimeData);

    /**
     * @brief Получить элемент находящийся в заданном индексе
     */
    AbstractTextModelItem* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Получить индекс заданного элемента
     */
    QModelIndex indexForItem(AbstractTextModelItem* _item) const;

    /**
     * @brief Задать модель титульной страницы
     */
    void setTitlePageModel(TextModel* _model);
    TextModel* titlePageModel() const;

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    void applyPatch(const QByteArray& _patch) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
