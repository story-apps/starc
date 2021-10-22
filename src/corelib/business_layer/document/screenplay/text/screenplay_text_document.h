#pragma once

#include <QTextDocument>

#include <corelib_global.h>


namespace BusinessLayer {
class ScreenplayTextModel;
enum class ScreenplayParagraphType;
} // namespace BusinessLayer

namespace BusinessLayer {
class ScreenplayTextCursor;

/**
 * @brief Класс документа сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextDocument : public QTextDocument
{
    Q_OBJECT

public:
    explicit ScreenplayTextDocument(QObject* _parent = nullptr);
    ~ScreenplayTextDocument() override;

    /**
     * @brief Идентификатор шаблона, с которым работает документ
     */
    void setTemplateId(const QString& _templateId);
    QString templateId() const;

    /**
     * @brief Задать модель текста сценария
     */
    void setModel(BusinessLayer::ScreenplayTextModel* _model, bool _canChangeModel = true);

    /**
     * @brief Получить позицию элемента в заданном индексе
     * @param _fromStart - true начальная позиция, false конечная позиция
     * @return Позицию элемента, -1 если элемент не удалось найти
     */
    int itemPosition(const QModelIndex& _index, bool _fromStart);
    int itemStartPosition(const QModelIndex& _index);
    int itemEndPosition(const QModelIndex& _index);

    /**
     * @brief Получить номер сцены для заданного блока
     */
    QString sceneNumber(const QTextBlock& _forBlock) const;

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
    void addParagraph(BusinessLayer::ScreenplayParagraphType _type, ScreenplayTextCursor _cursor);

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

} // namespace BusinessLayer
