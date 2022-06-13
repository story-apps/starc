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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
