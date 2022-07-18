#include "key_press_handler_facade.h"

#include "../screenplay_text_edit.h"
#include "act_footer_handler.h"
#include "act_heading_handler.h"
#include "action_handler.h"
#include "character_handler.h"
#include "dialog_handler.h"
#include "inline_note_handler.h"
#include "lyrics_handler.h"
#include "parenthetical_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "scene_characters_handler.h"
#include "scene_heading_handler.h"
#include "sequence_footer_handler.h"
#include "sequence_heading_handler.h"
#include "shot_handler.h"
#include "splitter_handler.h"
#include "transition_handler.h"
#include "unformatted_text_handler.h"

#include <business_layer/templates/screenplay_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTextEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(ScreenplayTextEdit* _editor);

    Ui::ScreenplayTextEdit* editor = nullptr;

    QScopedPointer<PrepareHandler> prepareHandler;
    QScopedPointer<PreHandler> preHandler;
    QScopedPointer<UnformattedTextHandler> unformattedTextHandler;
    QScopedPointer<SceneHeadingHandler> sceneHeaderHandler;
    QScopedPointer<SceneCharactersHandler> sceneCharactersHandler;
    QScopedPointer<ActionHandler> actionHandler;
    QScopedPointer<CharacterHandler> characterHandler;
    QScopedPointer<ParentheticalHandler> parentheticalHandler;
    QScopedPointer<DialogHandler> dialogHandler;
    QScopedPointer<LyricsHandler> lyricsHandler;
    QScopedPointer<TransitionHandler> transitionHandler;
    QScopedPointer<ShotHandler> shotHandler;
    QScopedPointer<InlineNoteHandler> inlineNoteHandler;
    QScopedPointer<SequenceHeadingHandler> sequenceHeadingHandler;
    QScopedPointer<SequenceFooterHandler> sequenceFooterHandler;
    QScopedPointer<ActHeadingHandler> actHeadingHandler;
    QScopedPointer<ActFooterHandler> actFooterHandler;
    QScopedPointer<SplitterHandler> splitterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::ScreenplayTextEdit* _editor)
    : editor(_editor)
    , prepareHandler(new PrepareHandler(_editor))
    , preHandler(new PreHandler(_editor))
    , unformattedTextHandler(new UnformattedTextHandler(_editor))
    , sceneHeaderHandler(new SceneHeadingHandler(_editor))
    , sceneCharactersHandler(new SceneCharactersHandler(_editor))
    , actionHandler(new ActionHandler(_editor))
    , characterHandler(new CharacterHandler(_editor))
    , parentheticalHandler(new ParentheticalHandler(_editor))
    , dialogHandler(new DialogHandler(_editor))
    , lyricsHandler(new LyricsHandler(_editor))
    , transitionHandler(new TransitionHandler(_editor))
    , shotHandler(new ShotHandler(_editor))
    , inlineNoteHandler(new InlineNoteHandler(_editor))
    , sequenceHeadingHandler(new SequenceHeadingHandler(_editor))
    , sequenceFooterHandler(new SequenceFooterHandler(_editor))
    , actHeadingHandler(new ActHeadingHandler(_editor))
    , actFooterHandler(new ActFooterHandler(_editor))
    , splitterHandler(new SplitterHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(ScreenplayTextEdit* _editor)
{
    static QHash<ScreenplayTextEdit*, KeyPressHandlerFacade*> instances;
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

KeyPressHandlerFacade::KeyPressHandlerFacade(ScreenplayTextEdit* _editor)
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

    case TextParagraphType::SceneCharacters: {
        return d->sceneCharactersHandler.data();
    }

    case TextParagraphType::Action: {
        return d->actionHandler.data();
    }

    case TextParagraphType::Character: {
        return d->characterHandler.data();
    }

    case TextParagraphType::Parenthetical: {
        return d->parentheticalHandler.data();
    }

    case TextParagraphType::Dialogue: {
        return d->dialogHandler.data();
    }

    case TextParagraphType::Lyrics: {
        return d->lyricsHandler.data();
    }

    case TextParagraphType::Transition: {
        return d->transitionHandler.data();
    }

    case TextParagraphType::Shot: {
        return d->shotHandler.data();
    }

    case TextParagraphType::InlineNote: {
        return d->inlineNoteHandler.data();
    }

    case TextParagraphType::SequenceHeading: {
        return d->sequenceHeadingHandler.data();
    }

    case TextParagraphType::SequenceFooter: {
        return d->sequenceFooterHandler.data();
    }

    case TextParagraphType::ActHeading: {
        return d->actHeadingHandler.data();
    }

    case TextParagraphType::ActFooter: {
        return d->actFooterHandler.data();
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
