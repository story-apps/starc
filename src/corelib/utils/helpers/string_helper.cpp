#include "string_helper.h"

#include <QHash>
#include <QString>

namespace {
    const QHash<Qt::Alignment, QString> kAlignmentToString
    = {{ Qt::AlignLeft, "left" },
       { Qt::AlignCenter, "center" },
       { Qt::AlignRight, "right" },
       { Qt::AlignJustify, "justify" }};
}


QString toString(Qt::Alignment _alignment)
{
    return kAlignmentToString.value(_alignment);
}

Qt::Alignment alignmentFromString(const QString& _alignment)
{
    return kAlignmentToString.key(_alignment);
}
