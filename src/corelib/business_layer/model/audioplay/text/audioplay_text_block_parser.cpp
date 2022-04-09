#include "audioplay_text_block_parser.h"

#include <QString>

namespace BusinessLayer {

QString AudioplayCharacterParser::name(const QString& _text)
{
    QString name = _text.trimmed();
    if (name.endsWith(':')) {
        name.chop(1);
    }
    return name;
}

} // namespace BusinessLayer
