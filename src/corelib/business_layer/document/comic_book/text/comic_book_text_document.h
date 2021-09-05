#pragma once

#include <QTextDocument>

#include <corelib_global.h>


namespace BusinessLayer {
class ComicBookTextModel;
enum class ComicBookParagraphType;
} // namespace BusinessLayer

namespace BusinessLayer {
class ComicBookTextCursor;

/**
 * @brief Класс документа сценария
 */
class CORE_LIBRARY_EXPORT ComicBookTextDocument : public QTextDocument
{
    Q_OBJECT

public:
    explicit ComicBookTextDocument(QObject* _parent = nullptr);
    ~ComicBookTextDocument() override;

    /**
     * @brief Задать идентификатор шаблона, с которым работает документ
     */
    void setTemplateId(const QString& _templateId);

    /**
     * @brief Задать модель текста сценария
     */
    void setModel(BusinessLayer::ComicBookTextModel* _model, bool _canChangeModel = true);

    /**
     * @brief Получить позицию элемента в заданном индексе
     * @param _fromStart - true начальная позиция, false конечная позиция
     * @return Позицию элемента, -1 если элемент не удалось найти
     */
    int itemPosition(const QModelIndex& _index, bool _fromStart);
    int itemStartPosition(const QModelIndex& _index);
    int itemEndPosition(const QModelIndex& _index);

    /**
     * @brief Получить номер страницы для заданного блока
     */
    QString pageNumber(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить номер панели для заданного блока
     */
    QString panelNumber(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить номер реплики для заданного блока
     */
    QString dialogueNumber(const QTextBlock& _forBlock) const;

    /**
     * @brief Получить цвет сцены/папки для заданного блока
     */
    QColor itemColor(const QTextBlock& _forBlock) const;

    /**
     * @brief Сформировать mime-данные сценария в заданном диапазоне
     */
    QString mimeFromSelection(int _fromPosition, int _toPosition) const;

    /**
     * @brief Вставить контент из mime-данных со сценарием в заданной позиции
     */
    void insertFromMime(int _position, const QString& _mimeData);

    /**
     * @brief Вставить новый блок заданного типа
     */
    void addParagraph(BusinessLayer::ComicBookParagraphType _type, ComicBookTextCursor _cursor);

    /**
     * @brief Установить тип блока для заданного курсора
     */
    void setParagraphType(BusinessLayer::ComicBookParagraphType _type,
                          const ComicBookTextCursor& _cursor);

    /**
     * @brief Очистить текущий блок от установленного в нём типа
     */
    void cleanParagraphType(const ComicBookTextCursor& _cursor);

    /**
     * @brief Применить заданный тип блока к тексту, на который указывает курсор
     */
    void applyParagraphType(BusinessLayer::ComicBookParagraphType _type,
                            const ComicBookTextCursor& _cursor);

    /**
     * @brief Разделить параграф на два
     */
    void splitParagraph(const ComicBookTextCursor& _cursor);

    /**
     * @brief Соединить разделённый параграф
     */
    void mergeParagraph(const ComicBookTextCursor& _cursor);

    /**
     * @brief Настроить необходимость корректировок
     */
    void setCorrectionOptions(bool _needToCorrectCharactersNames, bool _needToCorrectPageBreaks);

    /**
     * @brief Добавить редакторсую заметку в текущее выделение
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                       const QString& _comment, const ComicBookTextCursor& _cursor);

private:
    /**
     * @brief Обновить содержимое модели, при изменение текста документа
     */
    void updateModelOnContentChange(int _position, int _charsRemoved, int _charsAdded);

    /**
     * @brief Вставить таблицу в заданном курсоре
     */
    void insertTable(const ComicBookTextCursor& _cursor);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
