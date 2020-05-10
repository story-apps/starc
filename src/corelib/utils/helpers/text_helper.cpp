#include "text_helper.h"

#include <QFontMetricsF>
#include <QtMath>
#include <QTextLayout>


int TextHelper::fineTextWidth(const QString& _text, const QFont& _font)
{
    const QFontMetricsF metrics(_font);
    // Чтобы корректно разместить текст нужна максимальная ширина, которую текст может занимать
    // используемые методы реализуют разные механизмы определения ширины, поэтому выбираем больший
    // и не забываем прибавить волшебную единичку, а то так не работает :)
    return qCeil(qMax(metrics.boundingRect(_text).width(), metrics.horizontalAdvance(_text))) + 1;
}

qreal TextHelper::fineLineSpacing(const QFont& _font)
{
    const QFontMetricsF metrics(_font);
    const qreal platformDelta =
#ifdef Q_OS_LINUX
            //
            // 09.04.2020 Ubuntu 18.04.4
            // Не знаю почему, но ручками пришлось подобрать данный коэффициент,
            // только при нём получается такой же вывод как и в ворде для разных шрифтов
            // тестировал как минимум на Courier New, Courier Prime и ещё паре каких были в системе
            //
            0.4;
#else
            0;
#endif
    return metrics.lineSpacing() + platformDelta;
}

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

QString TextHelper::toHtmlEscaped(const QString& _text)
{
    const QHash<QChar, QString> map = {{ QLatin1Char('<'), QLatin1String("&lt;") },
                                       { QLatin1Char('>'), QLatin1String("&gt;") },
                                       { QLatin1Char('&'), QLatin1String("&amp;") },
                                       { QLatin1Char('"'), QLatin1String("&quot;") },
                                       { QLatin1Char('\n'), QLatin1String("&#10;") }};

    QString escaped;
    const int textLength = _text.length();
    escaped.reserve(static_cast<int>(textLength * 1.1));
    for (int i = 0; i < textLength; ++i) {
        const auto character = _text.at(i);
        escaped += map.value(character, character);
    }
    escaped.squeeze();
    return escaped;
}

QString TextHelper::fromHtmlEscaped(const QString& _escaped)
{
    const QHash<QString, QString> map = {{ QLatin1String("&lt;"), QLatin1String("<") },
                                         { QLatin1String("&gt;"), QLatin1String(">") },
                                         { QLatin1String("&amp;"), QLatin1String("&") },
                                         { QLatin1String("&quot;"), QLatin1String("\"") },
                                         { QLatin1String("&#10;"), QLatin1String("\n") }};

    QString text;
    const int textLength = _escaped.length();
    text.reserve(textLength);
    for (int i = 0; i < textLength; ++i) {
        const auto character = _escaped.at(i);
        if (character != QLatin1Char('&')) {
            text += character;
            continue;
        }

        int j = i + 1;
        while (j < textLength
               && _escaped.at(j) != QLatin1Char(';')) {
            ++j;
        }
        const QString middle = _escaped.mid(i, j - i + 1);
        text += map.value(middle, middle);

        i = j;
    }
    text.squeeze();
    return text;
}

QString TextHelper::smartToUpper(const QString& _text)
{
    QString result = _text;
    result = result.replace("ß", "ẞ");
    result = result.toUpper();
    return result;
}
QChar TextHelper::smartToUpper(const QChar& _char)
{
    if (_char == QString("ß")[0]) {
        return QString("ẞ")[0];
    }

    return _char.toUpper();
}

QString TextHelper::smartToLower(const QString& _text)
{
    QString result = _text;
    result = result.replace("ẞ", "ß");
    result = result.toUpper();
    return result;
}

QChar TextHelper::smartToLower(const QChar& _char)
{
    if (_char == QString("ẞ")[0]) {
        return QString("ß")[0];
    }

    return _char.toUpper();
}

