#pragma once

#include <ui/widgets/text_edit/page/page_text_edit.h>

class SpellCheckHighlighter;

/**
 * @brief Политика обновления состояние проверки орфографии
 */
enum class SpellCheckPolicy {
    //
    // Автоматическое обновление, как реакция на соответствующее событие
    //
    Auto,
    //
    // Ручное управление, только через метод класса
    //
    Manual,
};

/**
 * @brief Класс текстового редактора с проверкой орфографии
 */
class CORE_LIBRARY_EXPORT SpellCheckTextEdit : public PageTextEdit
{
    Q_OBJECT

public:
    explicit SpellCheckTextEdit(QWidget* _parent = nullptr);
    ~SpellCheckTextEdit() override;

    /**
     * @brief Задать политику обновления состояния проверки орфографии
     * @default Auto
     */
    void setSpellCheckPolicy(SpellCheckPolicy _policy);

    /**
     * @brief Включить/выключить проверку правописания
     */
    void setUseSpellChecker(bool _use);

    /**
     * @brief Используется ли проверка орфографии
     */
    bool useSpellChecker() const;

    /**
     * @brief Установить язык для проверки орфографии
     */
    void setSpellCheckLanguage(const QString& _languageCode);

    /**
     * @brief Переопределяем для очистки собственных параметров, перед очисткой в  базовом классе
     */
    void prepareToClear() override;

protected:
    /**
     * @brief Пересоздание подсвечивающего класса
     */
    void setHighlighterDocument(QTextDocument* _document);

    /**
     * @brief Добавляем реакцию на событие включения/отключения проверки орфографии
     */
    bool event(QEvent* _event) override;

private:
    /**
     * @brief Игнорировать слово под курсором
     */
    void ignoreWord() const;

    /**
     * @brief Добавить слово под курсором в пользовательский словарь
     */
    void addWordToUserDictionary() const;

    /**
     * @brief Заменить слово под курсором на выбранный вариант из предложенных
     */
    void replaceWordOnSuggestion();

    /**
     * @brief Сменилась позиция курсора
     */
    void rehighlighWithNewCursor();

    /**
     * @brief Найти слово в позиции
     * @param Позиция в тексте
     * @return Слово, находящееся в данной позиции
     */
    QString wordOnPosition(const QPoint& _position) const;

    /**
     * @brief Удаляет пунктуацию в слове
     */
    QString removePunctutaion(const QString& _word) const;

    /**
     * @brief Перемещает курсор в начало слова (с учетом - и ')
     */
    QTextCursor moveCursorToStartWord(QTextCursor cursor) const;

    /**
     * @brief Перемещает курсор в конец слова (с учетом - и ')
     */
    QTextCursor moveCursorToEndWord(QTextCursor cursor) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
