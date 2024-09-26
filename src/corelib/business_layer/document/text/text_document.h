#pragma once

#include <business_layer/model/text/text_model_text_item.h>

#include <QTextDocument>

#include <corelib_global.h>


namespace BusinessLayer {
class TextModel;
enum class TextParagraphType;
} // namespace BusinessLayer

namespace BusinessLayer {
class TextCursor;
class AbstractTextCorrector;

/**
 * @brief Класс документа текста
 */
class CORE_LIBRARY_EXPORT TextDocument : public QTextDocument
{
    Q_OBJECT

    friend class TextCursor;

public:
    explicit TextDocument(QObject* _parent = nullptr);
    ~TextDocument() override;

    /**
     * @brief Активна ли в данный момент транзакция по изменению документа
     */
    bool isEditTransactionActive();

    /**
     * @brief Модель текста
     */
    void setModel(BusinessLayer::TextModel* _model, bool _canChangeModel = true);
    BusinessLayer::TextModel* model() const;

    /**
     * @brief Настроить необходимость корректировок (переданные параметры будут активированы)
     */
    void setCorrectionOptions(const QStringList& _options);

    /**
     * @brief Видимый верхнеуровневый элемент
     */
    QModelIndex visibleTopLeveLItem() const;
    void setVisibleTopLevelItem(const QModelIndex& _index);

    /**
     * @brief Получить позицию элемента в заданном индексе
     * @param _fromStart - true начальная позиция, false конечная позиция
     * @return Позицию элемента, -1 если элемент не удалось найти
     */
    int itemPosition(const QModelIndex& _index, bool _fromStart);
    int itemStartPosition(const QModelIndex& _index);
    int itemEndPosition(const QModelIndex& _index);

    /**
     * @brief Получить индекс элемента модели для заданного блока
     */
    QModelIndex itemIndex(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить цвет сцены/папки для заданного блока
     */
    QColor itemColor(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить цвета всех родительских элементов заданного блока
     */
    QVector<QColor> itemColors(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить заголовк сцены для заданного блока
     */
    QString groupTitle(const QTextBlock& _forBlock) const;

    /**
     * @brief Сформировать mime-данные сценария в заданном диапазоне
     */
    QString mimeFromSelection(int _fromPosition, int _toPosition) const;

    /**
     * @brief Вставить контент из mime-данных со сценарием в заданной позиции
     * @return Позиция завершения вставки, либо -1 если вставки не было
     */
    int insertFromMime(int _position, const QString& _mimeData);

    /**
     * @brief Вставить новый блок заданного типа
     */
    void addParagraph(BusinessLayer::TextParagraphType _type, TextCursor _cursor);

    /**
     * @brief Установить тип блока для заданного курсора
     */
    void setParagraphType(BusinessLayer::TextParagraphType _type, const TextCursor& _cursor);

    /**
     * @brief Очистить текущий блок от установленного в нём типа
     */
    void cleanParagraphType(const TextCursor& _cursor);

    /**
     * @brief Применить заданный тип блока к тексту, на который указывает курсор
     */
    void applyParagraphType(BusinessLayer::TextParagraphType _type, const TextCursor& _cursor);

    /**
     * @brief Разделить параграф на два
     */
    void splitParagraph(const TextCursor& _cursor);

    /**
     * @brief Соединить разделённый параграф
     * @return Положение курсора, куда его нужно установить, после объединения
     */
    int mergeParagraph(const TextCursor& _cursor);

    /**
     * @brief Добавить редакторсую заметку в текущее выделение
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                       const QString& _comment, bool _isRevision, const TextCursor& _cursor);

    /**
     * @brief Добавить ресурсную заметку в текущее выделение
     */
    void addResourceMark(const QUuid& _resourceUuid, const TextCursor& _cursor);

    /**
     * @brief Получить закладку блока
     */
    TextModelTextItem::Bookmark bookmark(const QTextBlock& _forBlock) const;

protected:
    /**
     * @brief Может ли документ менять модель
     */
    bool canChangeModel() const;

    /**
     * @brief Задать корректировщик текста
     */
    void setCorrector(AbstractTextCorrector* _corrector);

    /**
     * @brief Интерфейс для обработки сброса модели
     */
    virtual void processModelReset();

private:
    /**
     * @brief Обновить содержимое модели, при изменение текста документа
     */
    void updateModelOnContentChange(int _position, int _charsRemoved, int _charsAdded);

    /**
     * @brief Вставить таблицу в заданном курсоре
     */
    void insertTable(const TextCursor& _cursor);

    /**
     * @brief Задать состояние изменения документа
     */
    void setEditTransactionActive(bool _active);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
