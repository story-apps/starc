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

    Ui::AudioplayTextEdit* m_editor = nullptr;

    QScopedPointer<PrepareHandler> m_prepareHandler;
    QScopedPointer<PreHandler> m_preHandler;
    QScopedPointer<UnformattedTextHandler> m_unformattedTextHandler;
    QScopedPointer<SceneHeadingHandler> m_sceneHeaderHandler;
    QScopedPointer<CharacterHandler> m_characterHandler;
    QScopedPointer<DialogHandler> m_dialogHandler;
    QScopedPointer<SoundHandler> m_soundHandler;
    QScopedPointer<MusicHandler> m_musicHandler;
    QScopedPointer<CueHandler> m_cueHandler;
    QScopedPointer<InlineNoteHandler> m_inlineNoteHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::AudioplayTextEdit* _editor)
    : m_editor(_editor)
    , m_prepareHandler(new PrepareHandler(_editor))
    , m_preHandler(new PreHandler(_editor))
    , m_unformattedTextHandler(new UnformattedTextHandler(_editor))
    , m_sceneHeaderHandler(new SceneHeadingHandler(_editor))
    , m_characterHandler(new CharacterHandler(_editor))
    , m_dialogHandler(new DialogHandler(_editor))
    , m_soundHandler(new SoundHandler(_editor))
    , m_musicHandler(new MusicHandler(_editor))
    , m_cueHandler(new CueHandler(_editor))
    , m_inlineNoteHandler(new InlineNoteHandler(_editor))
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
    d->m_prepareHandler->handle(_event);
}

void KeyPressHandlerFacade::prepareForHandle(QKeyEvent* _event)
{
    d->m_preHandler->handle(_event);
}

void KeyPressHandlerFacade::prehandle()
{
    const bool prepare = true;
    handle(nullptr, prepare);
}

void KeyPressHandlerFacade::handle(QEvent* _event, bool _pre)
{
    QTextBlock currentBlock = d->m_editor->textCursor().block();
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
    return d->m_prepareHandler->needSendEventToBaseClass();
}

bool KeyPressHandlerFacade::needEnsureCursorVisible() const
{
    return d->m_prepareHandler->needEnsureCursorVisible();
}

bool KeyPressHandlerFacade::needPrehandle() const
{
    return d->m_prepareHandler->needPrehandle();
}

KeyPressHandlerFacade::KeyPressHandlerFacade(AudioplayTextEdit* _editor)
    : d(new Implementation(_editor))
{
}

AbstractKeyHandler* KeyPressHandlerFacade::handlerFor(TextParagraphType _type)
{
    switch (_type) {
    case TextParagraphType::UnformattedText: {
        return d->m_unformattedTextHandler.data();
    }

    case TextParagraphType::SceneHeading: {
        return d->m_sceneHeaderHandler.data();
    }

    case TextParagraphType::Character: {
        return d->m_characterHandler.data();
    }

    case TextParagraphType::Dialogue: {
        return d->m_dialogHandler.data();
    }

    case TextParagraphType::Sound: {
        return d->m_soundHandler.data();
    }

    case TextParagraphType::Music: {
        return d->m_musicHandler.data();
    }

    case TextParagraphType::Cue: {
        return d->m_cueHandler.data();
    }

    case TextParagraphType::InlineNote: {
        return d->m_inlineNoteHandler.data();
    }

    default: {
        return nullptr;
    }
    }
}

} // namespace KeyProcessingLayer
