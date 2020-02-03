#pragma once

#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <QTextBlock>

class SpellCheckHighlighter;
enum class SpellCheckerLanguage;


/**
 * @brief Класс текстового редактора с проверкой орфографии
 */
class SpellCheckTextEdit : public PageTextEdit
{
    Q_OBJECT

public:
    explicit SpellCheckTextEdit(QWidget* _parent = nullptr);
    ~SpellCheckTextEdit() override;

public slots:
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
     * @param Язык
     */
    void setSpellCheckLanguage(SpellCheckerLanguage _language);

    /**
     * @brief Переопределяем для очистки собственных параметров, перед очисткой в  базовом классе
     */
    void prepareToClear() override;

protected:
    /**
     * @brief Пересоздание подсвечивающего класса
     */
    void setHighlighterDocument(QTextDocument* _document);

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
