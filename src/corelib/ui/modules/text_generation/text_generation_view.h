#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Сайдбар с инструментами ИИ помощника
 */
class CORE_LIBRARY_EXPORT TextGenerationView : public Widget
{
    Q_OBJECT

public:
    /**
     * @brief Положение курсора для вставки сгенерированного текста
     */
    enum class TextInsertPosition {
        AtBeginning,
        AtCursorPosition,
        AtEnd,
    };

public:
    explicit TextGenerationView(QWidget* _parent = nullptr);
    ~TextGenerationView() override;

    /**
     * @brief Задать возможность редактирования
     */
    bool isReadOnly() const;
    void setReadOnly(bool _readOnly);

    /**
     * @brief Задать подсказку для генерации текста
     */
    void setGenerationPromptHint(const QString& _hint);

    /**
     * @brief Куда нужно вставлять сгенерированный текст
     */
    TextInsertPosition textInsertPosition() const;

    /**
     * @brief Задать результат генерации
     */
    void setRephraseResult(const QString& _text);
    void setExpandResult(const QString& _text);
    void setShortenResult(const QString& _text);
    void setInsertResult(const QString& _text);
    void setSummarizeResult(const QString& _text);
    void setTransateResult(const QString& _text);

    /**
     * @brief Задать количество доступных слов для генерации
     */
    void setAvailableWords(int _availableWords);

signals:
    /**
     * @brief Пользователь хочет перефразировать текст в заданном стиле
     */
    void rephraseRequested(const QString& _source, const QString& _style);

    /**
     * @brief Пользователь хочет расширить текст
     */
    void expandRequested(const QString& _source);

    /**
     * @brief Пользователь хочет сократить текст
     */
    void shortenRequested(const QString& _source);

    /**
     * @brief Пользователь хочет вставить текст между заданными
     */
    void insertRequested(const QString& _afterText, const QString& _beforeText);

    /**
     * @brief Пользователь хочет получить аннотацию текста
     */
    void summarizeRequested(const QString& _source);

    /**
     * @brief Пользователь хочет перевести текст на заданный язык
     */
    void translateRequested(const QString& _text, const QString& _language);

    /**
     * @brief Пользователь хочет сгенерировать текст по запросу
     */
    void generateRequested(const QString& _text);

    /**
     * @brief Пользователь хочет вставить заданный текст в редактор
     */
    void insertTextRequested(const QString& _text);

    /**
     * @brief Нажата кнопка покупки кредитов
     */
    void buyCreditsPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
