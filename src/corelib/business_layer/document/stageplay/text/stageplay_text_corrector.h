#pragma once

#include <business_layer/document/text/abstract_text_corrector.h>


namespace BusinessLayer {

/**
 * @brief Класс корректирующий текст пьесы
 */
class CORE_LIBRARY_EXPORT StageplayTextCorrector : public AbstractTextCorrector
{
    Q_OBJECT

public:
    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    static QString continuedTerm();

public:
    explicit StageplayTextCorrector(QTextDocument* _document);
    ~StageplayTextCorrector() override;

    /**
     * @brief Установить необходимость корректировки текста
     */
    void setCorrectionOptions(const QStringList& _options) override;

protected:
    /**
     * @brief Очистить все сохранённые параметры
     */
    void clearImpl() override;

    /**
     * @brief Выполнить корректировки
     */
    void makeCorrections(int _position = -1, int _charsChanged = 0) override;

    /**
     * @brief Выполнить "мягкие" корректировки
     */
    void makeSoftCorrections(int _position = -1, int _charsChanged = 0) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
