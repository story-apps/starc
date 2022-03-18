#include "key_press_handler_facade.h"

#include "../screenplay_treatment_edit.h"
#include "beat_heading_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "scene_characters_handler.h"
#include "scene_heading_handler.h"
#include "sequence_footer_handler.h"
#include "sequence_heading_handler.h"

#include <business_layer/templates/screenplay_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTreatmentEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(ScreenplayTreatmentEdit* _editor);

    Ui::ScreenplayTreatmentEdit* m_editor = nullptr;

    QScopedPointer<PrepareHandler> m_prepareHandler;
    QScopedPointer<PreHandler> m_preHandler;
    QScopedPointer<SceneHeadingHandler> m_sceneHeaderHandler;
    QScopedPointer<SceneCharactersHandler> m_sceneCharactersHandler;
    QScopedPointer<BeatHeadingHandler> m_beatHeadingHandler;
    QScopedPointer<SequenceHeadingHandler> m_sequenceHeadingHandler;
    QScopedPointer<SequenceFooterHandler> m_sequenceFooterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::ScreenplayTreatmentEdit* _editor)
    : m_editor(_editor)
    , m_prepareHandler(new PrepareHandler(_editor))
    , m_preHandler(new PreHandler(_editor))
    , m_sceneHeaderHandler(new SceneHeadingHandler(_editor))
    , m_sceneCharactersHandler(new SceneCharactersHandler(_editor))
    , m_beatHeadingHandler(new BeatHeadingHandler(_editor))
    , m_sequenceHeadingHandler(new SequenceHeadingHandler(_editor))
    , m_sequenceFooterHandler(new SequenceFooterHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(ScreenplayTreatmentEdit* _editor)
{
    static QHash<ScreenplayTreatmentEdit*, KeyPressHandlerFacade*> instances;
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

KeyPressHandlerFacade::KeyPressHandlerFacade(ScreenplayTreatmentEdit* _editor)
    : d(new Implementation(_editor))
{
}

AbstractKeyHandler* KeyPressHandlerFacade::handlerFor(TextParagraphType _type)
{
    switch (_type) {

    case TextParagraphType::SceneHeading: {
        return d->m_sceneHeaderHandler.data();
    }

    case TextParagraphType::SceneCharacters: {
        return d->m_sceneCharactersHandler.data();
    }

    case TextParagraphType::BeatHeading: {
        return d->m_beatHeadingHandler.data();
    }

    case TextParagraphType::SequenceHeading: {
        return d->m_sequenceHeadingHandler.data();
    }

    case TextParagraphType::SequenceFooter: {
        return d->m_sequenceFooterHandler.data();
    }

    default: {
        return nullptr;
    }
    }
}

} // namespace KeyProcessingLayer
