#include "key_press_handler_facade.h"

#include "../comic_book_text_edit.h"
#include "character_handler.h"
#include "description_handler.h"
#include "dialog_handler.h"
#include "folder_footer_handler.h"
#include "folder_header_handler.h"
#include "inline_note_handler.h"
#include "page_handler.h"
#include "panel_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "unformatted_text_handler.h"

#include <business_layer/templates/comic_book_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::ComicBookTextEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(ComicBookTextEdit* _editor);

    Ui::ComicBookTextEdit* m_editor = nullptr;

    QScopedPointer<PrepareHandler> m_prepareHandler;
    QScopedPointer<PreHandler> m_preHandler;
    QScopedPointer<UnformattedTextHandler> m_unformattedTextHandler;
    QScopedPointer<PageHandler> m_pageHandler;
    QScopedPointer<PanelHandler> m_panelHandler;
    QScopedPointer<DescriptionHandler> m_actionHandler;
    QScopedPointer<CharacterHandler> m_characterHandler;
    QScopedPointer<DialogHandler> m_dialogHandler;
    QScopedPointer<InlineNoteHandler> m_inlineNoteHandler;
    QScopedPointer<FolderHeaderHandler> m_folderHeaderHandler;
    QScopedPointer<FolderFooterHandler> m_folderFooterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::ComicBookTextEdit* _editor)
    : m_editor(_editor)
    , m_prepareHandler(new PrepareHandler(_editor))
    , m_preHandler(new PreHandler(_editor))
    , m_unformattedTextHandler(new UnformattedTextHandler(_editor))
    , m_pageHandler(new PageHandler(_editor))
    , m_panelHandler(new PanelHandler(_editor))
    , m_actionHandler(new DescriptionHandler(_editor))
    , m_characterHandler(new CharacterHandler(_editor))
    , m_dialogHandler(new DialogHandler(_editor))
    , m_inlineNoteHandler(new InlineNoteHandler(_editor))
    , m_folderHeaderHandler(new FolderHeaderHandler(_editor))
    , m_folderFooterHandler(new FolderFooterHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(ComicBookTextEdit* _editor)
{
    static QHash<ComicBookTextEdit*, KeyPressHandlerFacade*> instances;
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

KeyPressHandlerFacade::KeyPressHandlerFacade(ComicBookTextEdit* _editor)
    : d(new Implementation(_editor))
{
}

AbstractKeyHandler* KeyPressHandlerFacade::handlerFor(TextParagraphType _type)
{
    switch (_type) {
    case TextParagraphType::UnformattedText: {
        return d->m_unformattedTextHandler.data();
    }

    case TextParagraphType::PageHeading: {
        return d->m_pageHandler.data();
    }

    case TextParagraphType::PanelHeading: {
        return d->m_panelHandler.data();
    }

    case TextParagraphType::Description: {
        return d->m_actionHandler.data();
    }

    case TextParagraphType::Character: {
        return d->m_characterHandler.data();
    }

    case TextParagraphType::Dialogue: {
        return d->m_dialogHandler.data();
    }

    case TextParagraphType::InlineNote: {
        return d->m_inlineNoteHandler.data();
    }

    case TextParagraphType::SequenceHeading: {
        return d->m_folderHeaderHandler.data();
    }

    case TextParagraphType::SequenceFooter: {
        return d->m_folderFooterHandler.data();
    }

    default: {
        return nullptr;
    }
    }
}

} // namespace KeyProcessingLayer
