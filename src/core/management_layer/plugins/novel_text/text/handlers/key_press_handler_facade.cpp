#include "key_press_handler_facade.h"

#include "../novel_text_edit.h"
#include "beat_heading_handler.h"
#include "chapter_footer_handler.h"
#include "chapter_heading_handler.h"
#include "inline_note_handler.h"
#include "part_footer_handler.h"
#include "part_heading_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "scene_heading_handler.h"
#include "text_handler.h"
#include "unformatted_text_handler.h"

#include <business_layer/templates/novel_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::NovelTextEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(NovelTextEdit* _editor);

    Ui::NovelTextEdit* editor = nullptr;

    QScopedPointer<PrepareHandler> prepareHandler;
    QScopedPointer<PreHandler> preHandler;
    QScopedPointer<UnformattedTextHandler> unformattedTextHandler;
    QScopedPointer<SceneHeadingHandler> sceneHeadingHandler;
    QScopedPointer<BeatHeadingHandler> beatHeadingHandler;
    QScopedPointer<TextHandler> textHandler;
    QScopedPointer<InlineNoteHandler> inlineNoteHandler;
    QScopedPointer<ChapterHeadingHandler> chapterHeadingHandler;
    QScopedPointer<ChapterFooterHandler> chapterFooterHandler;
    QScopedPointer<PartHeadingHandler> partHeadingHandler;
    QScopedPointer<PartFooterHandler> partFooterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::NovelTextEdit* _editor)
    : editor(_editor)
    , prepareHandler(new PrepareHandler(_editor))
    , preHandler(new PreHandler(_editor))
    , unformattedTextHandler(new UnformattedTextHandler(_editor))
    , sceneHeadingHandler(new SceneHeadingHandler(_editor))
    , beatHeadingHandler(new BeatHeadingHandler(_editor))
    , textHandler(new TextHandler(_editor))
    , inlineNoteHandler(new InlineNoteHandler(_editor))
    , chapterHeadingHandler(new ChapterHeadingHandler(_editor))
    , chapterFooterHandler(new ChapterFooterHandler(_editor))
    , partHeadingHandler(new PartHeadingHandler(_editor))
    , partFooterHandler(new PartFooterHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(NovelTextEdit* _editor)
{
    static QHash<NovelTextEdit*, KeyPressHandlerFacade*> instances;
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

KeyPressHandlerFacade::KeyPressHandlerFacade(NovelTextEdit* _editor)
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
        return d->sceneHeadingHandler.data();
    }

    case TextParagraphType::BeatHeading: {
        return d->beatHeadingHandler.data();
    }

    case TextParagraphType::Text: {
        return d->textHandler.data();
    }

    case TextParagraphType::InlineNote: {
        return d->inlineNoteHandler.data();
    }

    case TextParagraphType::ChapterHeading: {
        return d->chapterHeadingHandler.data();
    }

    case TextParagraphType::ChapterFooter: {
        return d->chapterFooterHandler.data();
    }

    case TextParagraphType::PartHeading: {
        return d->partHeadingHandler.data();
    }

    case TextParagraphType::PartFooter: {
        return d->partFooterHandler.data();
    }

    default: {
        return nullptr;
    }
    }
}

} // namespace KeyProcessingLayer
