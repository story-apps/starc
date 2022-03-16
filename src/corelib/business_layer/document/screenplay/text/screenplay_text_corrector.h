#pragma once

#include <business_layer/document/text/abstract_text_corrector.h>

class QTextBlock;
class QTextBlockFormat;
class QTextDocument;


namespace BusinessLayer {

class TextCursor;

/**
 * @brief Класс корректирующий текст сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextCorrector : public AbstractTextCorrector
{
    Q_OBJECT

public:
    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    static QString continuedTerm();

public:
    explicit ScreenplayTextCorrector(QTextDocument* _document);
    ~ScreenplayTextCorrector() override;

    /**
     * @brief Установить необходимость корректировать текст блоков имён персонажей
     */
    void setCorrectionOptions(const QStringList& _options) override;

    /**
     * @brief Очистить все сохранённые параметры
     */
    void clear() override;

    /**
     * @brief Выполнить корректировки
     */
    void correct(int _position = -1, int _charsChanged = 0) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
