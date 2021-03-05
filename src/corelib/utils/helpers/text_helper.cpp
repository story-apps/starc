#include "text_helper.h"

#include <QFontMetricsF>
#include <QtMath>
#include <QTextLayout>


qreal TextHelper::fineTextWidth(const QString& _text, const QFont& _font)
{
    return fineTextWidth(_text, QFontMetricsF(_font));
}

qreal TextHelper::fineTextWidth(const QString& _text, const QFontMetricsF& _metrics)
{
    //
    // Чтобы корректно разместить текст нужна максимальная ширина, которую текст может занимать
    // используемые методы реализуют разные механизмы определения ширины, поэтому выбираем больший
    // и не забываем прибавить волшебную единичку, а то так не работает :)
    //
    return qMax(_metrics.boundingRect(_text).width(), _metrics.horizontalAdvance(_text)) + 1.0;
}

qreal TextHelper::fineLineSpacing(const QFont& _font)
{
    return fineLineSpacing(QFontMetricsF(_font));

}

qreal TextHelper::fineLineSpacing(const QFontMetricsF& _metrics)
{
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
    return _metrics.lineSpacing() + platformDelta;
}

qreal TextHelper::heightForWidth(const QString& _text, const QFont& _font, qreal _width)
{
    const qreal lineHeight = fineLineSpacing(_font);
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
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textLayout.setTextOption(textOption);
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

QString TextHelper::elidedText(const QString& _text, const QFont& _font, const QRectF& _rect)
{
    const QFontMetricsF metrics(_font);
    const qreal lineHeight = fineLineSpacing(_font);
    qreal height = 0;

    //
    // Корректируем текст, чтобы QTextLayout смог сам обработать переносы строк
    //
    QString correctedText = _text;
    correctedText.replace('\n', QChar::LineSeparator);

    //
    // Компануем текст и определяем текст, который влезает в заданную область
    //
    QString elidedText;
    QTextLayout textLayout(correctedText, _font);
    textLayout.beginLayout();
    forever {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(_rect.width());
        height += lineHeight;

        //
        // Если строка влезает, то оставляем её без изменений
        //
        if (height < _rect.height()) {
            elidedText += _text.mid(line.textStart(), line.textLength());
        }
        //
        // А если это последняя строка, то многоточим её
        //
        else {
            //
            // ... при этом берём не только влезающий текст, а чуть больше,
            //     чтобы корректно обработать ситуацию длинного слова в конце строки
            //
            QString lastLine = _text.mid(line.textStart(), line.textLength() * 2);

            //
            // ... если весь текст влез, не надо добавлять многоточие в конце
            //
            if (metrics.horizontalAdvance(lastLine) <= _rect.width()
                && _text.endsWith(lastLine)) {
                elidedText += lastLine;
                break;
            }

            //
            // ... многоточим
            //
            lastLine += "…";
            while (lastLine.length() > 1
                   && metrics.horizontalAdvance(lastLine) > _rect.width()) {
                lastLine.remove(lastLine.length() - 2, 1);
            }
            elidedText += lastLine;
            break;
        }
    }
    textLayout.endLayout();

    return elidedText;
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
    result = result.toLower();
    return result;
}

QChar TextHelper::smartToLower(const QChar& _char)
{
    if (_char == QString("ẞ")[0]) {
        return QString("ß")[0];
    }

    return _char.toLower();
}

