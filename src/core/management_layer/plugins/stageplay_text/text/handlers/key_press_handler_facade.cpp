#include "key_press_handler_facade.h"

#include "../stageplay_text_edit.h"
#include "action_handler.h"
#include "character_handler.h"
#include "dialog_handler.h"
#include "inline_note_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "scene_heading_handler.h"
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

    Ui::StageplayTextEdit* m_editor = nullptr;

    QScopedPointer<PrepareHandler> m_prepareHandler;
    QScopedPointer<PreHandler> m_preHandler;
    QScopedPointer<UnformattedTextHandler> m_unformattedTextHandler;
    QScopedPointer<SceneHeadingHandler> m_sceneHeaderHandler;
    QScopedPointer<CharacterHandler> m_characterHandler;
    QScopedPointer<DialogHandler> m_dialogHandler;
    QScopedPointer<ActionHandler> m_actionHandler;
    QScopedPointer<InlineNoteHandler> m_inlineNoteHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::StageplayTextEdit* _editor)
    : m_editor(_editor)
    , m_prepareHandler(new PrepareHandler(_editor))
    , m_preHandler(new PreHandler(_editor))
    , m_unformattedTextHandler(new UnformattedTextHandler(_editor))
    , m_sceneHeaderHandler(new SceneHeadingHandler(_editor))
    , m_characterHandler(new CharacterHandler(_editor))
    , m_dialogHandler(new DialogHandler(_editor))
    , m_actionHandler(new ActionHandler(_editor))
    , m_inlineNoteHandler(new InlineNoteHandler(_editor))
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

KeyPressHandlerFacade::KeyPressHandlerFacade(StageplayTextEdit* _editor)
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

    case TextParagraphType::Action: {
        return d->m_actionHandler.data();
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
