#pragma once

#include <corelib_global.h>

#include <QTextDocument>


namespace BusinessLayer {
    class TextModel;
    enum class TextParagraphType;
}

namespace BusinessLayer
{

/**
 * @brief Класс текстового документа
 */
class CORE_LIBRARY_EXPORT TextDocument : public QTextDocument
{
    Q_OBJECT

public:
    explicit TextDocument(QObject* _parent = nullptr);
    ~TextDocument() override;

    /**
     * @brief Задать идентификатор шаблона, с которым работает документ
     */
    void setTemplateId(const QString& _templateId);

    /**
     * @brief Задать модель текста
     */
    void setModel(BusinessLayer::TextModel* _model, bool _canChangeModel = true);

    /**
     * @brief Получить позицию элемента в заданном индексе
     * @param _fromStart - true начальная позиция, false конечная позиция
     * @return Позицию элемента, -1 если элемент не удалось найти
     */
    int itemPosition(const QModelIndex& _index, bool _fromStart);
    int itemStartPosition(const QModelIndex& _index);
    int itemEndPosition(const QModelIndex& _index);

    /**
     * @brief Получить номер главы для заданного блока
     */
    QString chapterNumber(const QTextBlock& _forBlock) const;

    /**
     * @brief Сформировать mime-данные текста в заданном диапазоне
     */
    QString mimeFromSelection(int _fromPosition, int _toPosition) const;

    /**
     * @brief Вставить контент из mime-данных с текстом в заданной позиции
     */
    void insertFromMime(int _position, const QString& _mimeData);

    /**
     * @brief Вставить новый блок заданного типа
     */
    void addParagraph(BusinessLayer::TextParagraphType _type,
        QTextCursor _cursor);

    /**
     * @brief Установить тип блока для заданного курсора
     */
    void setParagraphType(BusinessLayer::TextParagraphType _type,
        const QTextCursor& _cursor);

    /**
     * @brief Применить заданный тип блока к тексту, на который указывает курсор
     */
    void applyParagraphType(BusinessLayer::TextParagraphType _type,
        const QTextCursor& _cursor);

    /**
     * @brief Добавить редакторсую заметку в текущее выделение
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
        const QString& _comment, const QTextCursor& _cursor);

private:
    /**
     * @brief Обновить содержимое модели, при изменение текста документа
     */
    void updateModelOnContentChange(int _position, int _charsRemoved, int _charsAdded);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
