#include "text_helper.h"

#include <QFontMetricsF>
#include <QtMath>
#include <QTextLayout>


qreal TextHelper::heightForWidth(const QString& _text, const QFont& _font, int _width)
{
    const QFontMetricsF metrics(_font);
    const qreal lineHeight = qMax(0.0, metrics.leading()) + qCeil(metrics.height());
    qreal height = 0;

    //
    // Корректируем текст, чтобы QTextLayout смог сам обработать переносы строк
    //
    QString correctedText = _text;
    correctedText.replace('\n', QChar::LineSeparator);

    //
    // Компануем текст и считаем, какой высоты получается результат
    //
    QTextLayout textLayout(correctedText, _font);
    textLayout.beginLayout();
    forever {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(_width);
        height += lineHeight;
    }
    textLayout.endLayout();

    return height;
}
