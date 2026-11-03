// Pre-optimization text layout, retained as a compatibility oracle.
#pragma once

#include <utils/helpers/text_helper.h>
#include <QFontMetricsF>
#include <QTextLayout>

QString legacyElidedText(const QString& _text, const QFont& _font, const QRectF& _rect)
{
    QFontMetricsF metrics(_font);

    //
    // Если влезает целиком, сразу выходим
    //
    if (metrics.boundingRect(_rect, Qt::TextWordWrap, _text).height() <= _rect.height()) {
        return _text;
    }

    QString elidedText;
    QString lastLine;

    QTextLayout layout(_text, _font);
    layout.beginLayout();
    forever
    {
        auto line = layout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(_rect.width());

        const auto nextLine = _text.mid(line.textStart(), line.textLength());
        const auto candidate = elidedText + nextLine;

        //
        // Проверяем, влезает ли текст в заданную область, если мы добавим к нему ещё одну линию
        //
        if (const auto usedHeight
            = metrics.boundingRect(_rect, Qt::TextWordWrap, candidate.trimmed()).height();
            usedHeight > _rect.height()) {
            //
            // ... если с этой линией уже не влезает, то многоточим последнюю строку
            //
            elidedText.chop(lastLine.length());
            lastLine += nextLine + "…";
            while (lastLine.length() > 1 && TextHelper::fineTextWidthF(lastLine, _font) > _rect.width()) {
                lastLine.remove(lastLine.length() - 2, 1);
            }
            if (lastLine.endsWith(" …")) {
                lastLine.chop(2);
                lastLine += "…";
            }
            elidedText += lastLine;
            break;
        }
        //
        // А если влезает, то идём за следующей строкой
        //
        elidedText = candidate;
        lastLine = nextLine;
    }
    layout.endLayout();

    return elidedText.trimmed();
}

