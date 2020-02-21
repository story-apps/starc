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
#include "shot_handler.h"
#include "transition_handler.h"

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

    PrepareHandler* m_prepareHandler = nullptr;
    PreHandler* m_preHandler = nullptr;
    SceneHeadingHandler* m_sceneHeaderHandler = nullptr;
    SceneCharactersHandler* m_sceneCharactersHandler = nullptr;
    ActionHandler* m_actionHandler = nullptr;
    CharacterHandler* m_characterHandler = nullptr;
    ParentheticalHandler* m_parentheticalHandler = nullptr;
    DialogHandler* m_dialogHandler = nullptr;
    TransitionHandler* m_transitionHandler = nullptr;
    NoteHandler* m_shotHandler = nullptr;
    TitleHeaderHandler* m_titleheaderHandler = nullptr;
    TitleHandler* m_titleHandler = nullptr;
    InlineNoteHandler* m_inlineNoteHandler = nullptr;
    FolderHeaderHandler* m_folderHeaderHandler = nullptr;
    FolderFooterHandler* m_folderFooterHandler = nullptr;
    SceneDescriptionHandler* m_sceneDescriptionHandler = nullptr;
    LyricsHandler* m_lyricsHandler = nullptr;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::ScreenplayTextEdit* _editor)
    : m_editor(_editor),
      m_prepareHandler(new PrepareHandler(_editor)),
      m_preHandler(new PreHandler(_editor)),
      m_sceneHeaderHandler(new SceneHeadingHandler(_editor)),
      m_sceneCharactersHandler(new SceneCharactersHandler(_editor)),
      m_actionHandler(new ActionHandler(_editor)),
      m_characterHandler(new CharacterHandler(_editor)),
      m_parentheticalHandler(new ParentheticalHandler(_editor)),
      m_dialogHandler(new DialogHandler(_editor)),
      m_transitionHandler(new TransitionHandler(_editor)),
      m_shotHandler(new NoteHandler(_editor)),
      m_titleheaderHandler(new TitleHeaderHandler(_editor)),
      m_titleHandler(new TitleHandler(_editor)),
      m_inlineNoteHandler(new InlineNoteHandler(_editor)),
      m_folderHeaderHandler(new FolderHeaderHandler(_editor)),
      m_folderFooterHandler(new FolderFooterHandler(_editor)),
      m_sceneDescriptionHandler(new SceneDescriptionHandler(_editor)),
      m_lyricsHandler(new LyricsHandler(_editor))
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
    const ScreenplayParagraphType currentType = BusinessLayer::ScreenplayBlockStyle::forBlock(currentBlock);
    AbstractKeyHandler* currentHandler = handlerFor(currentType);

    if (currentHandler != nullptr) {
        if (_pre) {
            currentHandler->prehandle();
        } else {
            currentHandler->handle(_event);
        }
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
        case ScreenplayParagraphType::SceneHeading: {
            return d->m_sceneHeaderHandler;
        }

        case ScreenplayParagraphType::SceneCharacters: {
            return d->m_sceneCharactersHandler;
        }

        case ScreenplayParagraphType::Action: {
            return d->m_actionHandler;
        }

        case ScreenplayParagraphType::Character: {
            return d->m_characterHandler;
        }
        case ScreenplayParagraphType::Parenthetical: {
            return d->m_parentheticalHandler;
        }

        case ScreenplayParagraphType::Dialogue: {
            return d->m_dialogHandler;
        }

        case ScreenplayParagraphType::Transition: {
            return d->m_transitionHandler;
        }

        case ScreenplayParagraphType::Shot: {
            return d->m_shotHandler;
        }

        case ScreenplayParagraphType::InlineNote: {
            return d->m_inlineNoteHandler;
        }

        case ScreenplayParagraphType::FolderHeader: {
            return d->m_folderHeaderHandler;
        }

        case ScreenplayParagraphType::FolderFooter: {
            return d->m_folderFooterHandler;
        }

        case ScreenplayParagraphType::SceneDescription: {
            return d->m_sceneDescriptionHandler;
        }

        case ScreenplayParagraphType::Lyrics: {
            return d->m_lyricsHandler;
        }

        default: {
            return nullptr;
        }
    }
}

} // namespace KeyProcessingLayer
