#include "stageplay_text_block_parser.h"

#include <utils/helpers/text_helper.h>

#include <QString>


namespace BusinessLayer {

QString StageplayCharacterParser::name(const QString& _text)
{
    QString name = _text.trimmed();
    if (name.endsWith(':')) {
        name.chop(1);
    }
    return TextHelper::smartToUpper(name);
}

} // namespace BusinessLayer
