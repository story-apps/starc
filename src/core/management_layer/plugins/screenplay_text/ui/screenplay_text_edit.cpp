#include "screenplay_text_edit.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>

#include <QScrollBar>


namespace Ui
{

class ScreenplayTextEdit::Implementation
{
public:
    BusinessLayer::ScreenplayTextModel* model = nullptr;
};


// ****


ScreenplayTextEdit::ScreenplayTextEdit(QWidget* _parent)
    : CompleterTextEdit(_parent),
      d(new Implementation)
{
}

ScreenplayTextEdit::~ScreenplayTextEdit()
{

}

void ScreenplayTextEdit::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->model = _model;
}

BusinessLayer::ScreenplayDictionariesModel* ScreenplayTextEdit::dictionaries() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->dictionariesModel();
}

BusinessLayer::CharactersModel* ScreenplayTextEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersModel();
}

BusinessLayer::LocationsModel* ScreenplayTextEdit::locations() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->locationsModel();
}

void ScreenplayTextEdit::addScenarioBlock(BusinessLayer::ScreenplayParagraphType _blockType)
{

}

void ScreenplayTextEdit::changeScenarioBlockType(BusinessLayer::ScreenplayParagraphType _blockType, bool _forced)
{

}

void ScreenplayTextEdit::changeScenarioBlockTypeForSelection(BusinessLayer::ScreenplayParagraphType _blockType)
{

}

void ScreenplayTextEdit::applyScenarioTypeToBlockText(BusinessLayer::ScreenplayParagraphType _blockType)
{

}

BusinessLayer::ScreenplayParagraphType ScreenplayTextEdit::scenarioBlockType() const
{

}

void ScreenplayTextEdit::setTextCursorReimpl(const QTextCursor& _cursor)
{
    //
    // TODO: пояснить зачем это необходимо делать?
    //
    const int verticalScrollValue = verticalScrollBar()->value();
    setTextCursor(_cursor);
    verticalScrollBar()->setValue(verticalScrollValue);
}

} // namespace Ui
