#include "key_press_handler_facade.h"

#include "action_handler.h"
#include "character_handler.h"
#include "dialog_handler.h"
#include "folder_header_handler.h"
#include "folder_footer_handler.h"
#include "inline_note_handler.h"
#include "lyrics_handler.h"
#include "parenthetical_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "scene_characters_handler.h"
#include "scene_description_handler.h"
#include "scene_heading_handler.h"
#include "scene_name_handler.h"
#include "shot_handler.h"
#include "transition_handler.h"
#include "unformatted_text_handler.h"

#include <business_layer/templates/screenplay_template.h>

#include <ui/screenplay_text_edit.h>

#include <QTextBlock>
#include <QKeyEvent>

using BusinessLayer::ScreenplayParagraphType;
using Ui::ScreenplayTextEdit;

namespace KeyProcessingLayer
{

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(ScreenplayTextEdit* _editor);

    Ui::ScreenplayTextEdit* m_editor = nullptr;

    QScopedPointer<PrepareHandler> m_prepareHandler;
    QScopedPointer<PreHandler> m_preHandler;
    QScopedPointer<UnformattedTextHandler> m_unformattedTextHandler;
    QScopedPointer<SceneNameHandler> m_sceneNameHandler;
    QScopedPointer<SceneDescriptionHandler> m_sceneDescriptionHandler;
    QScopedPointer<SceneHeadingHandler> m_sceneHeaderHandler;
    QScopedPointer<SceneCharactersHandler> m_sceneCharactersHandler;
    QScopedPointer<ActionHandler> m_actionHandler;
    QScopedPointer<CharacterHandler> m_characterHandler;
    QScopedPointer<ParentheticalHandler> m_parentheticalHandler;
    QScopedPointer<DialogHandler> m_dialogHandler;
    QScopedPointer<LyricsHandler> m_lyricsHandler;
    QScopedPointer<TransitionHandler> m_transitionHandler;
    QScopedPointer<ShotHandler> m_shotHandler;
    QScopedPointer<InlineNoteHandler> m_inlineNoteHandler;
    QScopedPointer<FolderHeaderHandler> m_folderHeaderHandler;
    QScopedPointer<FolderFooterHandler> m_folderFooterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::ScreenplayTextEdit* _editor)
    : m_editor(_editor),
      m_prepareHandler(new PrepareHandler(_editor)),
      m_preHandler(new PreHandler(_editor)),
      m_unformattedTextHandler(new UnformattedTextHandler(_editor)),
      m_sceneNameHandler(new SceneNameHandler(_editor)),
      m_sceneDescriptionHandler(new SceneDescriptionHandler(_editor)),
      m_sceneHeaderHandler(new SceneHeadingHandler(_editor)),
      m_sceneCharactersHandler(new SceneCharactersHandler(_editor)),
      m_actionHandler(new ActionHandler(_editor)),
      m_characterHandler(new CharacterHandler(_editor)),
      m_parentheticalHandler(new ParentheticalHandler(_editor)),
      m_dialogHandler(new DialogHandler(_editor)),
      m_lyricsHandler(new LyricsHandler(_editor)),
      m_transitionHandler(new TransitionHandler(_editor)),
      m_shotHandler(new ShotHandler(_editor)),
      m_inlineNoteHandler(new InlineNoteHandler(_editor)),
      m_folderHeaderHandler(new FolderHeaderHandler(_editor)),
      m_folderFooterHandler(new FolderFooterHandler(_editor))
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
    const auto currentType = BusinessLayer::ScreenplayBlockStyle::forBlock(currentBlock);
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

KeyPressHandlerFacade::KeyPressHandlerFacade(ScreenplayTextEdit* _editor)
    : d(new Implementation(_editor))
{
}

AbstractKeyHandler* KeyPressHandlerFacade::handlerFor(ScreenplayParagraphType _type)
{
    switch (_type) {
        case ScreenplayParagraphType::UnformattedText: {
            return d->m_unformattedTextHandler.data();
        }

        case ScreenplayParagraphType::SceneName: {
            return d->m_sceneNameHandler.data();
        }

        case ScreenplayParagraphType::SceneDescription: {
            return d->m_sceneDescriptionHandler.data();
        }

        case ScreenplayParagraphType::SceneHeading: {
            return d->m_sceneHeaderHandler.data();
        }

        case ScreenplayParagraphType::SceneCharacters: {
            return d->m_sceneCharactersHandler.data();
        }

        case ScreenplayParagraphType::Action: {
            return d->m_actionHandler.data();
        }

        case ScreenplayParagraphType::Character: {
            return d->m_characterHandler.data();
        }

        case ScreenplayParagraphType::Parenthetical: {
            return d->m_parentheticalHandler.data();
        }

        case ScreenplayParagraphType::Dialogue: {
            return d->m_dialogHandler.data();
        }

        case ScreenplayParagraphType::Lyrics: {
            return d->m_lyricsHandler.data();
        }

        case ScreenplayParagraphType::Transition: {
            return d->m_transitionHandler.data();
        }

        case ScreenplayParagraphType::Shot: {
            return d->m_shotHandler.data();
        }

        case ScreenplayParagraphType::InlineNote: {
            return d->m_inlineNoteHandler.data();
        }

        case ScreenplayParagraphType::FolderHeader: {
            return d->m_folderHeaderHandler.data();
        }

        case ScreenplayParagraphType::FolderFooter: {
            return d->m_folderFooterHandler.data();
        }

        default: {
            return nullptr;
        }
    }
}

} // namespace KeyProcessingLayer
