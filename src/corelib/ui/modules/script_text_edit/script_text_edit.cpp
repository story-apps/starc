#include "script_text_edit.h"

namespace Ui {

class ScriptTextEdit::Implementation
{
public:
    /**
     * @brief Показывать автодополения в пустых блоках
     */
    bool showSuggestionsInEmptyBlocks = true;
};


// ****


ScriptTextEdit::ScriptTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent)
    , d(new Implementation)
{
}

ScriptTextEdit::~ScriptTextEdit() = default;

bool ScriptTextEdit::showSuggestionsInEmptyBlocks() const
{
    return d->showSuggestionsInEmptyBlocks;
}

void ScriptTextEdit::setShowSuggestionsInEmptyBlocks(bool _show)
{
    d->showSuggestionsInEmptyBlocks = _show;
}

} // namespace Ui
