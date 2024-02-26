#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Сайдбар с инструментами ИИ помощника
 */
class CORE_LIBRARY_EXPORT AiAssistantView : public Widget
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

    /**
     * @brief Тип представления генерации контента
     */
    enum class GenerationViewType {
        Text,
        CharacterInformation,
        MindMap,
    };

public:
    explicit AiAssistantView(QWidget* _parent = nullptr);
    ~AiAssistantView() override;

    /**
     * @brief Задать возможность редактирования
     */
    bool isReadOnly() const;
    void setReadOnly(bool _readOnly);

    /**
     * @brief Задать видимость кнопок вставки сгенерированного контента во внешний редактор
     */
    void setInsertionAvailable(bool _available);

    /**
     * @brief Настроить доступность возможности генерации синопсиса
     */
    void setSynopsisGenerationAvaiable(bool _available);

    /**
     * @brief Настроить доступность возможности генерации романа
     */
    void setNovelGenerationAvaiable(bool _available);

    /**
     * @brief Настроить доступность возможности генерации сценария
     */
    void setScriptGenerationAvaiable(bool _available);

    /**
     * @brief Задать подсказку для генерации синопсиса
     */
    void setGenerationSynopsisOptions(const QString& _hint);

    /**
     * @brief Задать подсказку для генерации романа
     */
    void setGenerationNovelOptions(const QString& _hint);

    /**
     * @brief Задать подсказку для генерации сценария
     */
    void setGenerationScriptOptions(const QString& _hint);

    /**
     * @brief Задать тип представления генерации контента
     */
    void setGenerationViewType(GenerationViewType _type);

    /**
     * @brief Задать подсказку для генерации текста
     */
    void setGenerationPromptHint(const QString& _hint);

    /**
     * @brief Задать промпт для генерации текста
     */
    void setGenerationPrompt(const QString& _prompt);

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
    void setGenerateSynopsisResult(const QString& _text);

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
     * @brief Пользователь хочет сгенерировать синопсис
     */
    void generateSynopsisRequested(int _maxWordsPerScene);

    /**
     * @brief Пользователь хочет сгенерировать роман из сценария
     */
    void generateNovelRequested();

    /**
     * @brief Пользователь хочет сгенерировать сценарий из романа
     */
    void generateScriptRequested();

    /**
     * @brief Пользователь хочет сгенерировать текст по запросу
     */
    void generateTextRequested(const QString& _text);

    /**
     * @brief Пользователь хочет сгенерировать параметры персонажа
     */
    void generateCharacterRequested(const QString& _text, bool _personalInfo, bool _physique,
                                    bool _life, bool _attitude, bool _biography, bool _image);

    /**
     * @brief Пользователь хочет сгенерировать интеллект карту по запросу
     */
    void generateMindMapRequested(const QString& _text);

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
