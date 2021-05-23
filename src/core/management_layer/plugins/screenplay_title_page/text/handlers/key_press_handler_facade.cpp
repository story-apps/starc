#include "key_press_handler_facade.h"

#include "pre_handler.h"
#include "prepare_handler.h"
#include "text_handler.h"

#include "../screenplay_title_page_edit.h"

#include <business_layer/templates/text_template.h>

#include <QTextBlock>
#include <QKeyEvent>

using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTitlePageEdit;

namespace KeyProcessingLayer
{

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(ScreenplayTitlePageEdit* _editor);

    Ui::ScreenplayTitlePageEdit* editor = nullptr;

    QScopedPointer<PrepareHandler> prepareHandler;
    QScopedPointer<PreHandler> preHandler;
    QScopedPointer<TextHandler> textHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::ScreenplayTitlePageEdit* _editor)
    : editor(_editor),
      prepareHandler(new PrepareHandler(_editor)),
      preHandler(new PreHandler(_editor)),
      textHandler(new TextHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(Ui::ScreenplayTitlePageEdit* _editor)
{
    static QHash<ScreenplayTitlePageEdit*, KeyPressHandlerFacade*> instances;
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
    const auto currentType = d->editor->currentParagraphType();
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

KeyPressHandlerFacade::KeyPressHandlerFacade(ScreenplayTitlePageEdit* _editor)
    : d(new Implementation(_editor))
{
}

AbstractKeyHandler* KeyPressHandlerFacade::handlerFor(TextParagraphType _type)
{
    switch (_type) {
        case TextParagraphType::Heading1:
        case TextParagraphType::Heading2:
        case TextParagraphType::Heading3:
        case TextParagraphType::Heading4:
        case TextParagraphType::Heading5:
        case TextParagraphType::Heading6:
        case TextParagraphType::Text:
        case TextParagraphType::InlineNote: {
            return d->textHandler.data();
        }

        default: {
            return nullptr;
        }
    }
}

} // namespace KeyProcessingLayer
