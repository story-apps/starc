#include "key_press_handler_facade.h"

#include "../title_page_edit.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "text_handler.h"

#include <business_layer/templates/text_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::TitlePageEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(TitlePageEdit* _editor);

    Ui::TitlePageEdit* editor = nullptr;

    QScopedPointer<PrepareHandler> prepareHandler;
    QScopedPointer<PreHandler> preHandler;
    QScopedPointer<TextHandler> textHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::TitlePageEdit* _editor)
    : editor(_editor)
    , prepareHandler(new PrepareHandler(_editor))
    , preHandler(new PreHandler(_editor))
    , textHandler(new TextHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(Ui::TitlePageEdit* _editor)
{
    static QHash<TitlePageEdit*, KeyPressHandlerFacade*> instances;
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
    if (_pre) {
        d->textHandler->prehandle();
    } else {
        d->textHandler->handle(_event);
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

KeyPressHandlerFacade::KeyPressHandlerFacade(TitlePageEdit* _editor)
    : d(new Implementation(_editor))
{
}

} // namespace KeyProcessingLayer
