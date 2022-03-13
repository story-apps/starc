#include "simple_text_model_text_item.h"

#include "simple_text_model.h"

#include <business_layer/templates/simple_text_template.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QColor>
#include <QLocale>
#include <QVariant>
#include <QXmlStreamReader>

#include <optional>

namespace BusinessLayer {

SimpleTextModelTextItem::SimpleTextModelTextItem(const SimpleTextModel* _model)
    : TextModelTextItem(_model)
{
}

QVariant SimpleTextModelTextItem::data(int _role) const
{
    switch (_role) {
    case Qt::DecorationRole: {
        return u8"\U000F09A8";
    }

    default: {
        return TextModelTextItem::data(_role);
    }
    }
}

} // namespace BusinessLayer
