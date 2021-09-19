#pragma once

#include <business_layer/model/abstract_model.h>


namespace BusinessLayer {

class TextModelItem;

/**
 * @brief Модель текстового документа
 */
class CORE_LIBRARY_EXPORT TextModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit TextModel(QObject* _parent = nullptr);
    ~TextModel() override;

    /**
     * @brief Название текстового документа
     */
    QString name() const;
    void setName(const QString& _name);
    void setDocumentName(const QString& _name) override;
    Q_SIGNAL void nameChanged(const QString& _name);

    /**
     * @brief Перезаписать содержимое документа
     */
    void setDocumentContent(const QByteArray& _content);

    /**
     * @brief Добавить элемент в конец
     */
    void appendItem(TextModelItem* _item, TextModelItem* _parentItem = nullptr);

    /**
     * @brief Добавить элемент в начало
     */
    void prependItem(TextModelItem* _item, TextModelItem* _parentItem = nullptr);

    /**
     * @brief Вставить элемент после заданного
     */
    void insertItem(TextModelItem* _item, TextModelItem* _afterSiblingItem);

    /**
     * @brief Извлечь заданный элемент без удаления
     */
    void takeItem(TextModelItem* _item, TextModelItem* _parentItem = nullptr);

    /**
     * @brief Удалить заданный элемент
     */
    void removeItem(TextModelItem* _item);

    /**
     * @brief Обновить заданный элемент
     */
    void updateItem(TextModelItem* _item);

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
     * @brief Сформировать mime-данные текста в заданном диапазоне
     */
    QString mimeFromSelection(const QModelIndex& _from, int _fromPosition, const QModelIndex& _to,
                              int _toPosition, bool _clearUuid) const;

    /**
     * @brief Вставить контент из mime-данных с текстом в заданной позиции
     */
    void insertFromMime(const QModelIndex& _index, int _positionInBlock, const QString& _mimeData);

    /**
     * @brief Получить элемент находящийся в заданном индексе
     */
    TextModelItem* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Получить индекс заданного элемента
     */
    QModelIndex indexForItem(TextModelItem* _item) const;

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
