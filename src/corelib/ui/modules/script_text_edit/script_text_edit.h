#pragma once

#include <ui/widgets/text_edit/base/base_text_edit.h>


namespace Ui {

/**
 * @brief Базовый класс текстовых редакторов сценария
 */
class CORE_LIBRARY_EXPORT ScriptTextEdit : public BaseTextEdit
{
    Q_OBJECT

public:
    explicit ScriptTextEdit(QWidget* _parent = nullptr);
    ~ScriptTextEdit() override;

    /**
     * @brief Показывать ли автодополнения в пустых блоках
     */
    bool showSuggestionsInEmptyBlocks() const;
    void setShowSuggestionsInEmptyBlocks(bool _show);

    /**
     * @brief Своя реализация установки курсора
     */
    void setTextCursorAndKeepScrollBars(const QTextCursor& _cursor);

protected:
    /**
     * @brief Дополнительная функция для обработки нажатий самим редактором
     * @return Обработано ли событие
     * @note В наследниках необходимо вызывать вручную при необходимости
     */
    bool keyPressEventReimpl(QKeyEvent* _event) override;

    /**
     * @brief Обрабатываем специфичные ситуации для редактора сценария
     */
    bool updateEnteredText(const QKeyEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
