#pragma once

#include <QObject>

#include <corelib_global.h>

class QTextBlock;
class QTextBlockFormat;
class QTextDocument;


namespace BusinessLayer {

class TextCursor;

/**
 * @brief Класс корректирующий текст документа
 */
class CORE_LIBRARY_EXPORT AbstractTextCorrector : public QObject
{
    Q_OBJECT

public:
    explicit AbstractTextCorrector(QTextDocument* _document);
    ~AbstractTextCorrector() override;

    /**
     * @brief Документ для корректировки
     */
    QTextDocument* document() const;

    /**
     * @brief Задать идентификатор шаблона, с которым работает корректор
     */
    void setTemplateId(const QString& _templateId);
    QString templateId() const;

    /**
     * @brief Установить необходимость корректировать текст блоков имён персонажей
     */
    virtual void setCorrectionOptions(const QStringList& _options) = 0;

    /**
     * @brief Очистить все сохранённые параметры
     */
    virtual void clear() = 0;

    /**
     * @brief Выполнить корректировки
     */
    virtual void correct(int _position = -1, int _charsChanged = 0) = 0;

    /**
     * @brief Подготовиться к корректировке и выполнить подготовленную корректировку
     */
    void planCorrection(int _position, int _charsRemoved, int _charsAdded);
    void makePlannedCorrection();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
