#include "key_press_handler_facade.h"

#include "../novel_outline_edit.h"
#include "beat_heading_handler.h"
#include "chapter_footer_handler.h"
#include "chapter_heading_handler.h"
#include "part_footer_handler.h"
#include "part_heading_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "scene_heading_handler.h"

#include <business_layer/templates/novel_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::NovelOutlineEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(NovelOutlineEdit* _editor);

    Ui::NovelOutlineEdit* m_editor = nullptr;

    QScopedPointer<PrepareHandler> m_prepareHandler;
    QScopedPointer<PreHandler> m_preHandler;
    QScopedPointer<SceneHeadingHandler> m_sceneHeaderHandler;
    QScopedPointer<BeatHeadingHandler> m_beatHeadingHandler;
    QScopedPointer<ChapterHeadingHandler> m_chapterHeadingHandler;
    QScopedPointer<ChapterFooterHandler> m_chapterFooterHandler;
    QScopedPointer<PartHeadingHandler> m_partHeadingHandler;
    QScopedPointer<PartFooterHandler> m_partFooterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::NovelOutlineEdit* _editor)
    : m_editor(_editor)
    , m_prepareHandler(new PrepareHandler(_editor))
    , m_preHandler(new PreHandler(_editor))
    , m_sceneHeaderHandler(new SceneHeadingHandler(_editor))
    , m_beatHeadingHandler(new BeatHeadingHandler(_editor))
    , m_chapterHeadingHandler(new ChapterHeadingHandler(_editor))
    , m_chapterFooterHandler(new ChapterFooterHandler(_editor))
    , m_partHeadingHandler(new PartHeadingHandler(_editor))
    , m_partFooterHandler(new PartFooterHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(NovelOutlineEdit* _editor)
{
    static QHash<NovelOutlineEdit*, KeyPressHandlerFacade*> instances;
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

KeyPressHandlerFacade::KeyPressHandlerFacade(NovelOutlineEdit* _editor)
    : d(new Implementation(_editor))
{
}

AbstractKeyHandler* KeyPressHandlerFacade::handlerFor(TextParagraphType _type)
{
    switch (_type) {

    case TextParagraphType::SceneHeading: {
        return d->m_sceneHeaderHandler.data();
    }

    case TextParagraphType::BeatHeading: {
        return d->m_beatHeadingHandler.data();
    }

    case TextParagraphType::ChapterHeading: {
        return d->m_chapterHeadingHandler.data();
    }

    case TextParagraphType::ChapterFooter: {
        return d->m_chapterFooterHandler.data();
    }

    case TextParagraphType::PartHeading: {
        return d->m_partHeadingHandler.data();
    }

    case TextParagraphType::PartFooter: {
        return d->m_partFooterHandler.data();
    }

    default: {
        return nullptr;
    }
    }
}

} // namespace KeyProcessingLayer
