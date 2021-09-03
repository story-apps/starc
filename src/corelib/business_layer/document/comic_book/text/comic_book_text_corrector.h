#pragma once

#include <QObject>

#include <corelib_global.h>

class QTextBlock;
class QTextBlockFormat;
class QTextDocument;


namespace BusinessLayer {

class ComicBookTextCursor;

/**
 * @brief Класс корректирующий текст сценария
 */
class CORE_LIBRARY_EXPORT ComicBookTextCorrector : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    static QString continuedTerm();

public:
    explicit ComicBookTextCorrector(QTextDocument* _document);
    ~ComicBookTextCorrector() override;

    /**
     * @brief Задать идентификатор шаблона, с которым работает корректор
     */
    void setTemplateId(const QString& _templateId);

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
    void correct(int _position = -1, int _charsChanged = 0);

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
