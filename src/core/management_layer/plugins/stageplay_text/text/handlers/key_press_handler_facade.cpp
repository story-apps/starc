#include "key_press_handler_facade.h"

#include "../stageplay_text_edit.h"
#include "action_handler.h"
#include "character_handler.h"
#include "dialog_handler.h"
#include "inline_note_handler.h"
#include "parenthetical_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "scene_heading_handler.h"
#include "splitter_handler.h"
#include "unformatted_text_handler.h"

#include <business_layer/templates/stageplay_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::StageplayTextEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(StageplayTextEdit* _editor);

    Ui::StageplayTextEdit* editor = nullptr;

    QScopedPointer<PrepareHandler> prepareHandler;
    QScopedPointer<PreHandler> preHandler;
    QScopedPointer<UnformattedTextHandler> unformattedTextHandler;
    QScopedPointer<SceneHeadingHandler> sceneHeaderHandler;
    QScopedPointer<CharacterHandler> characterHandler;
    QScopedPointer<DialogHandler> dialogHandler;
    QScopedPointer<ParentheticalHandler> parentheticalHandler;
    QScopedPointer<ActionHandler> actionHandler;
    QScopedPointer<InlineNoteHandler> inlineNoteHandler;
    QScopedPointer<SplitterHandler> splitterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::StageplayTextEdit* _editor)
    : editor(_editor)
    , prepareHandler(new PrepareHandler(_editor))
    , preHandler(new PreHandler(_editor))
    , unformattedTextHandler(new UnformattedTextHandler(_editor))
    , sceneHeaderHandler(new SceneHeadingHandler(_editor))
    , characterHandler(new CharacterHandler(_editor))
    , dialogHandler(new DialogHandler(_editor))
    , parentheticalHandler(new ParentheticalHandler(_editor))
    , actionHandler(new ActionHandler(_editor))
    , inlineNoteHandler(new InlineNoteHandler(_editor))
    , splitterHandler(new SplitterHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(StageplayTextEdit* _editor)
{
    static QHash<StageplayTextEdit*, KeyPressHandlerFacade*> instances;
    if (!instances.contains(_editor)) {
        instances.insert(_editor, new KeyPressHandlerFacade(_editor));
    }

    return instances.value(_editor);
}

KeyPressHandlerFacade::~KeyPressHandlerFacade() = default;

void KeyPressHandlerFacade::prepare(QKeyEvent* _event)
{
    d->prepareHandler->handle(_event);
}

void KeyPressHandlerFacade::prepareForHandle(QKeyEvent* _event)
{
    d->preHandler->handle(_event);
}

void KeyPressHandlerFacade::prehandle()
{
    const bool prepare = true;
    handle(nullptr, prepare);
}

void KeyPressHandlerFacade::handle(QEvent* _event, bool _pre)
{
    QTextBlock currentBlock = d->editor->textCursor().block();
    const auto currentType = BusinessLayer::TextBlockStyle::forBlock(currentBlock);
    auto currentHandler = handlerFor(currentType);

    if (currentHandler == nullptr) {
        return;
    }

    if (_pre) {
        currentHandler->prehandle();
    } else {
        currentHandler->handle(_event);
    }
}

bool KeyPressHandlerFacade::needSendEventToBaseClass() const
{
    return d->prepareHandler->needSendEventToBaseClass();
}

bool KeyPressHandlerFacade::needEnsureCursorVisible() const
{
    return d->prepareHandler->needEnsureCursorVisible();
}

bool KeyPressHandlerFacade::needPrehandle() const
{
    return d->prepareHandler->needPrehandle();
}

KeyPressHandlerFacade::KeyPressHandlerFacade(StageplayTextEdit* _editor)
    : d(new Implementation(_editor))
{
}

AbstractKeyHandler* KeyPressHandlerFacade::handlerFor(TextParagraphType _type)
{
    switch (_type) {
    case TextParagraphType::UnformattedText: {
        return d->unformattedTextHandler.data();
    }

    case TextParagraphType::SceneHeading: {
        return d->sceneHeaderHandler.data();
    }

    case TextParagraphType::Character: {
        return d->characterHandler.data();
    }

    case TextParagraphType::Dialogue: {
        return d->dialogHandler.data();
    }

    case TextParagraphType::Parenthetical: {
        return d->parentheticalHandler.data();
    }

    case TextParagraphType::Action: {
        return d->actionHandler.data();
    }

    case TextParagraphType::InlineNote: {
        return d->inlineNoteHandler.data();
    }

    case TextParagraphType::PageSplitter: {
        return d->splitterHandler.data();
    }

    default: {
        return nullptr;
    }
    }
}

} // namespace KeyProcessingLayer
