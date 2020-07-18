#pragma once

#include <QObject>

class QTextBlock;
class QTextBlockFormat;
class QTextDocument;


namespace BusinessLayer
{

class ScreenplayTextCursor;

/**
 * @brief Класс корректирующий текст сценария
 */
class ScreenplayTextCorrector : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    static QString continuedTerm();

public:
    explicit ScreenplayTextCorrector(QTextDocument* _document, const QString& _templateName = {});
    ~ScreenplayTextCorrector() override;

    /**
     * @brief Установить необходимость корректировать текст блоков имён персонажей
     */
    void setNeedToCorrectCharactersNames(bool _need);

    /**
     * @brief Установить необходимость корректировать текст на разрывах страниц
     */
    void setNeedToCorrectPageBreaks(bool _need);

    /**
     * @brief Очистить все сохранённые параметры
     */
    void clear();

    /**
     * @brief Выполнить корректировки
     */
    void correct(int _position = -1, int _charsRemoved = 0, int _charsAdded = 0);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
