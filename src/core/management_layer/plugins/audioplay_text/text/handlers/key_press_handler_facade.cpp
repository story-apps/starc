#include "key_press_handler_facade.h"

#include "../audioplay_text_edit.h"
#include "character_handler.h"
#include "cue_handler.h"
#include "dialog_handler.h"
#include "inline_note_handler.h"
#include "music_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "scene_heading_handler.h"
#include "sound_handler.h"
#include "splitter_handler.h"
#include "unformatted_text_handler.h"

#include <business_layer/templates/audioplay_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::AudioplayTextEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(AudioplayTextEdit* _editor);

    Ui::AudioplayTextEdit* editor = nullptr;

    QScopedPointer<PrepareHandler> prepareHandler;
    QScopedPointer<PreHandler> preHandler;
    QScopedPointer<UnformattedTextHandler> unformattedTextHandler;
    QScopedPointer<SceneHeadingHandler> sceneHeaderHandler;
    QScopedPointer<CharacterHandler> characterHandler;
    QScopedPointer<DialogHandler> dialogHandler;
    QScopedPointer<SoundHandler> soundHandler;
    QScopedPointer<MusicHandler> musicHandler;
    QScopedPointer<CueHandler> cueHandler;
    QScopedPointer<InlineNoteHandler> inlineNoteHandler;
    QScopedPointer<SplitterHandler> splitterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::AudioplayTextEdit* _editor)
    : editor(_editor)
    , prepareHandler(new PrepareHandler(_editor))
    , preHandler(new PreHandler(_editor))
    , unformattedTextHandler(new UnformattedTextHandler(_editor))
    , sceneHeaderHandler(new SceneHeadingHandler(_editor))
    , characterHandler(new CharacterHandler(_editor))
    , dialogHandler(new DialogHandler(_editor))
    , soundHandler(new SoundHandler(_editor))
    , musicHandler(new MusicHandler(_editor))
    , cueHandler(new CueHandler(_editor))
    , inlineNoteHandler(new InlineNoteHandler(_editor))
    , splitterHandler(new SplitterHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(AudioplayTextEdit* _editor)
{
    static QHash<AudioplayTextEdit*, KeyPressHandlerFacade*> instances;
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

KeyPressHandlerFacade::KeyPressHandlerFacade(AudioplayTextEdit* _editor)
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

    case TextParagraphType::Sound: {
        return d->soundHandler.data();
    }

    case TextParagraphType::Music: {
        return d->musicHandler.data();
    }

    case TextParagraphType::Cue: {
        return d->cueHandler.data();
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
