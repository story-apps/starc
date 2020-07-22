#pragma once

#include <QTextDocument>


namespace BusinessLayer {
    class ScreenplayTextModel;
    enum class ScreenplayParagraphType;
}

namespace BusinessLayer
{
class ScreenplayTextCursor;

/**
 * @brief Класс документа сценария
 */
class ScreenplayTextDocument : public QTextDocument
{
    Q_OBJECT

public:
    explicit ScreenplayTextDocument(QObject* _parent = nullptr);
    ~ScreenplayTextDocument() override;

    /**
     * @brief Задать модель текста сценария
     */
    void setModel(BusinessLayer::ScreenplayTextModel* _model);

    /**
     * @brief Получить позицию элемента в заданном индексе
     */
    int itemPosition(const QModelIndex& _index);

    //
    // FIXME: делать
    //
    QString mimeFromSelection(int, int) const { return {}; }
    void insertFromMime(int, const QString&) {}

    /**
     * @brief Вставить новый блок заданного типа
     */
    void addParagraph(BusinessLayer::ScreenplayParagraphType _type,
        ScreenplayTextCursor _cursor);

    /**
     * @brief Установить тип блока для заданного курсора
     */
    void setParagraphType(BusinessLayer::ScreenplayParagraphType _type,
        const ScreenplayTextCursor& _cursor);

    /**
     * @brief Очистить текущий блок от установленного в нём типа
     */
    void cleanParagraphType(const ScreenplayTextCursor& _cursor);

    /**
     * @brief Применить заданный тип блока к тексту, на который указывает курсор
     */
    void applyParagraphType(BusinessLayer::ScreenplayParagraphType _type,
        const ScreenplayTextCursor& _cursor);

    /**
     * @brief Разделить параграф на два
     */
    void splitParagraph(const ScreenplayTextCursor& _cursor);

    /**
     * @brief Соединить разделённый параграф
     */
    void mergeParagraph(const ScreenplayTextCursor& _cursor);

    /**
     * @brief Настроить необходимость корректировок
     */
    void setCorrectionOptions(bool _needToCorrectCharactersNames, bool _needToCorrectPageBreaks);

    /**
     * @brief Добавить редакторсую заметку в текущее выделение
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
        const QString& _comment, const ScreenplayTextCursor& _cursor);

private:
    /**
     * @brief Обновить содержимое модели, при изменение текста документа
     */
    void updateModelOnContentChange(int _position, int _charsRemoved, int _charsAdded);

    /**
     * @brief Вставить таблицу в заданном курсоре
     */
    void insertTable(const ScreenplayTextCursor& _cursor);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
