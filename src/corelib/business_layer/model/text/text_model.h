#pragma once

#include <business_layer/model/abstract_model.h>

class QXmlStreamReader;


namespace BusinessLayer {

enum class TextGroupType;
class SimpleTextModel;
class TextModelItem;
class TextModelGroupItem;
class TextModelFolderItem;
class TextModelSplitterItem;
class TextModelTextItem;

/**
 * @brief Модель текста сценария
 */
class CORE_LIBRARY_EXPORT TextModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit TextModel(QObject* _parent, TextModelFolderItem* _rootItem);
    ~TextModel() override;

    /**
     * @brief Создать элементы модели
     */
    virtual TextModelFolderItem* createFolderItem() const = 0;
    virtual TextModelFolderItem* createFolderItem(QXmlStreamReader& _contentReader) const;
    virtual TextModelGroupItem* createGroupItem(TextGroupType _type) const = 0;
    virtual TextModelGroupItem* createGroupItem(QXmlStreamReader& _contentReader) const;
    virtual TextModelSplitterItem* createSplitterItem() const;
    virtual TextModelSplitterItem* createSplitterItem(QXmlStreamReader& _contentReader) const;
    virtual TextModelTextItem* createTextItem() const = 0;
    virtual TextModelTextItem* createTextItem(QXmlStreamReader& _contentReader) const;

    /**
     * @brief Добавить элемент в конец
     */
    void appendItem(TextModelItem* _item, TextModelItem* _parentItem = nullptr);
    void appendItems(const QVector<TextModelItem*>& _items, TextModelItem* _parentItem = nullptr);

    /**
     * @brief Добавить элемент в начало
     */
    void prependItem(TextModelItem* _item, TextModelItem* _parentItem = nullptr);

    /**
     * @brief Вставить элемент после заданного
     */
    void insertItem(TextModelItem* _item, TextModelItem* _afterSiblingItem);
    void insertItems(const QVector<TextModelItem*>& _items, TextModelItem* _afterSiblingItem);

    /**
     * @brief Извлечь заданный элемент без удаления
     */
    void takeItem(TextModelItem* _item, TextModelItem* _parentItem = nullptr);
    void takeItems(TextModelItem* _fromItem, TextModelItem* _toItem,
                   TextModelItem* _parentItem = nullptr);

    /**
     * @brief Удалить заданный элемент
     */
    void removeItem(TextModelItem* _item);
    void removeItems(TextModelItem* _fromItem, TextModelItem* _toItem);

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
    TextModelItem* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Получить индекс заданного элемента
     */
    QModelIndex indexForItem(TextModelItem* _item) const;

    /**
     * @brief Задать модель титульной страницы
     */
    void setTitlePageModel(SimpleTextModel* _model);
    SimpleTextModel* titlePageModel() const;

    /**
     * @brief Задать модель синопсиса
     */
    void setSynopsisModel(SimpleTextModel* _model);
    SimpleTextModel* synopsisModel() const;

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

    /**
     * @brief Инициилизировать пустой документ
     */
    virtual void initEmptyDocument() = 0;

    /**
     * @brief Финализировать инициилизацию
     */
    virtual void finalizeInitialization() = 0;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
