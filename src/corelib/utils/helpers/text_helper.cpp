#include "text_helper.h"

#include "measurement_helper.h"

#include <QApplication>
#include <QDebug>
#include <QFontMetricsF>
#include <QRegularExpression>
#include <QScreen>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QtMath>


namespace {
/**
 * @brief Протестировать корректность тюнинга метрик шрифтов для текущей системы
 */
void testFontsMetrics()
{
    //
    // В 249 миллиметров должны влезать 51,5 строка Courier New, или 58 строк Courier Prime
    // В 155 миллиметров должен влезать 61 символ обоих шрифтов
    //
    QString str = QString("DPIX: %1 | DPIY: %2\n\n")
                      .arg(QApplication::primaryScreen()->physicalDotsPerInchX())
                      .arg(QApplication::primaryScreen()->physicalDotsPerInchY());
    for (const auto& fn : { "Courier New", "Courier Prime" }) {
        QFont f(fn);
        f.setPixelSize(MeasurementHelper::ptToPx(12));
        QFontMetricsF fm(f);
        str += QString("%1 | %2 lines per page | %3 from 155 | horadv for 1 %4 and for 61 %5\n")
                   .arg(f.family())
                   .arg(249.0 / MeasurementHelper::pxToMm(TextHelper::fineLineSpacing(f), false))
                   .arg(MeasurementHelper::pxToMm(
                       TextHelper::fineTextWidthF(QString().fill('W', 61), f), true))
                   .arg(fm.horizontalAdvance("W"))
                   .arg(fm.horizontalAdvance(QString().fill('W', 61)));
    }
    qDebug() << str;
}

/**
 * @brief Дельты для высоты строки, которые должны учитываться на данной системе
 */
static QHash<QString, qreal> sFontToLineSpacing;

/**
 * @brief Настраиваем метрики шрифтов и коэффициент масштабровнаия таким образом,
 *        чтобы текст на всех платформах выглядел одинаково
 */
static void initFontMetrics()
{
    if (!sFontToLineSpacing.isEmpty()) {
        return;
    }

    //
    // Значения высоты шрифта из файла самого шрифта, платформы тут могут выдавать разные значения
    //
    constexpr auto courierNewHeight = 2320.0;

    //
    // Отстраиваем параметры по Courier New, а все остальные адаптируем исходя из этой настройки
    //
    QFont courierNewFont("Courier New");
    courierNewFont.setPixelSize(MeasurementHelper::ptToPx(12));
    QFontMetricsF courierNewMetrics(courierNewFont);
    qreal courierNewLineSpacing = courierNewMetrics.lineSpacing();
    qreal courierNewDelta = 0.0;
    //
    // ... в 249 миллиметров должны вмещаться 51.5 строки шрифтом Courier New
    //
    while ((249.0 / MeasurementHelper::pxToMm(courierNewLineSpacing, false)) > 51.8) {
        courierNewDelta += 0.1;
        courierNewLineSpacing = courierNewMetrics.lineSpacing() + courierNewDelta;
    }
    sFontToLineSpacing[courierNewFont.family()] = courierNewDelta;

    //
    // Остальные дельты высчитываются на основе Courier New
    //
    auto addFontDelta = [courierNewHeight, courierNewLineSpacing](const QString& _fontFamily,
                                                                  int _fontMetricsHeight) {
        if (!QFontDatabase().hasFamily(_fontFamily)) {
            return;
        }

        QFont font(_fontFamily);
        font.setPixelSize(MeasurementHelper::ptToPx(12));
        QFontMetricsF fontMetrics(font);
        const qreal fontDelta =
            //
            // ... такой Line Spacing должен быть у настраимого шрифта
            //
            courierNewLineSpacing * (_fontMetricsHeight / courierNewHeight)
            //
            // ... вычитая из него Line Spacing из метрики, получим дельту
            //
            - fontMetrics.lineSpacing();
        sFontToLineSpacing[_fontFamily] = fontDelta;
    };
    //
    // ... собственно высчитываем дельту для других шрифтов (HHead group summary)
    //
    addFontDelta("Courier Prime", 2060.0);
    addFontDelta("Courier Screenplay", 2370.0);
    addFontDelta("Courier Final Draft", 2318.0);
    addFontDelta("Arial", 2355.0);
    addFontDelta("Times New Roman", 2355.0);
}

} // namespace

qreal TextHelper::fineTextWidthF(const QString& _text, const QFont& _font)
{
    return fineTextWidthF(_text, QFontMetricsF(_font));
}

qreal TextHelper::fineTextWidthF(const QString& _text, const QFontMetricsF& _metrics)
{
    //
    // Не забываем прибавить волшебную единичку, а то так не работает :)
    //
    return _metrics.horizontalAdvance(_text) + 1.0;
}

int TextHelper::fineTextWidth(const QString& _text, const QFont& _font)
{
    //
    // Из-за того, что шрифт в целых пикселях, а масштабирование в дробных, чуть увеличиваем ширину,
    // чтобы при отрисовке, текст всегда точно влезал
    //
    return qCeil(fineTextWidthF(_text, _font));
}

qreal TextHelper::fineLineSpacing(const QFont& _font)
{
    initFontMetrics();

    const QFontMetricsF metrics(_font);

    const qreal platformDelta =
#ifdef Q_OS_LINUX
        //
        // 09.04.2020 Ubuntu 18.04.4
        // Не знаю почему, но ручками пришлось подобрать данный коэффициент,
        // только при нём получается такой же вывод как и в ворде для разных шрифтов
        // тестировал как минимум на Courier New, Courier Prime и ещё паре каких были в системе
        //
        0.2;
#else
        0;
#endif
    return metrics.lineSpacing() + sFontToLineSpacing.value(_font.family(), platformDelta);
}

void TextHelper::updateFontHinting(QFont& _font)
{
#ifdef Q_OS_WINDOWS
    //
    // Arial в Windows при масштабировании рисуется коряво, поэтому убираем ему хинтинг
    //
    if (_font.family() == "Arial") {
        _font.setHintingPreference(QFont::PreferNoHinting);
    }
#else
    Q_UNUSED(_font)
#endif
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
    textLayout.beginLayout();
    forever
    {
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

QString TextHelper::lastLineText(const QString& _text, const QFont& _font, qreal _width)
{
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
    QString lastLineText;
    forever
    {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(_width);
        lastLineText = correctedText.mid(line.textStart(), line.textLength());
    }
    textLayout.endLayout();

    return lastLineText;
}

QString TextHelper::elidedText(const QString& _text, const QFont& _font, const QRectF& _rect)
{
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
    forever
    {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(_rect.width());
        height += lineHeight;

        //
        // Если строка влезает, то оставляем её без изменений
        //
        const auto heightWithNextLine = height + lineHeight;
        if (heightWithNextLine <= _rect.height()) {
            elidedText += _text.midRef(line.textStart(), line.textLength());
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
            if (fineTextWidthF(lastLine, _font) <= _rect.width() && _text.endsWith(lastLine)) {
                elidedText += lastLine;
                break;
            }

            //
            // ... многоточим
            //
            lastLine += "…";
            while (lastLine.length() > 1 && fineTextWidthF(lastLine, _font) > _rect.width()) {
                lastLine.remove(lastLine.length() - 2, 1);
            }
            elidedText += lastLine;
            break;
        }
    }
    textLayout.endLayout();

    return elidedText;
}

QString TextHelper::elidedText(const QString& _text, const QFont& _font, qreal _width)
{
    return elidedText(_text, _font, QRectF(0, 0, _width, fineLineSpacing(_font)));
}

QString TextHelper::toHtmlEscaped(const QString& _text)
{
    const QHash<QChar, QString> map = { { QLatin1Char('<'), QLatin1String("&lt;") },
                                        { QLatin1Char('>'), QLatin1String("&gt;") },
                                        { QLatin1Char('&'), QLatin1String("&amp;") },
                                        { QLatin1Char('"'), QLatin1String("&quot;") },
                                        { QLatin1Char('\n'), QLatin1String("&#10;") } };

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

QString TextHelper::toRxEscaped(const QString& _text)
{
    const QHash<QChar, QString> map = {
        { QLatin1Char('?'), QLatin1String("[?]") },  { QLatin1Char('*'), QLatin1String("[*]") },
        { QLatin1Char('.'), QLatin1String("[.]") },  { QLatin1Char('+'), QLatin1String("[+]") },
        { QLatin1Char('-'), QLatin1String("[-]") },  { QLatin1Char('\\'), QLatin1String("[\\]") },
        { QLatin1Char('"'), QLatin1String("[\"]") }, { QLatin1Char('('), QLatin1String("[(]") },
        { QLatin1Char(')'), QLatin1String("[)]") },  { QLatin1Char('['), QLatin1String("[[]") },
        { QLatin1Char(']'), QLatin1String("[]]") },
    };

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
    const QHash<QString, QString> map = { { QLatin1String("&lt;"), QLatin1String("<") },
                                          { QLatin1String("&gt;"), QLatin1String(">") },
                                          { QLatin1String("&amp;"), QLatin1String("&") },
                                          { QLatin1String("&quot;"), QLatin1String("\"") },
                                          { QLatin1String("&#10;"), QLatin1String("\n") } };

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
        while (j < textLength && _escaped.at(j) != QLatin1Char(';')) {
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

QString TextHelper::toSentenceCase(const QString& _text, bool _capitalizeEveryWord)
{
    if (_text.isEmpty()) {
        return {};
    }

    //
    // TODO: реализовать возможность поднимать регистр каждого слова,
    //       либо только после знаков препинания
    //

    return smartToUpper(_text[0]) + smartToLower(_text.mid(1));
}

int TextHelper::wordsCount(const QString& _text)
{
    //
    // FIXME: Сделать более корректный подсчёт
    //        - слова разделённые знаками препинания без пробелов
    //        - не учитывать знаки препинания окружённые пробелами, типа " - "
    //
    static QRegularExpression wordCountExpression("[\\s.,!():;]+");
    return _text.split(wordCountExpression, Qt::SkipEmptyParts).count();
}

void TextHelper::updateSelectionFormatting(
    QTextCursor _cursor, std::function<QTextCharFormat(const QTextCharFormat&)> _updateFormat)
{
    if (!_cursor.hasSelection()) {
        return;
    }

    _cursor.beginEditBlock();

    int position = std::min(_cursor.selectionStart(), _cursor.selectionEnd());
    const int lastPosition = std::max(_cursor.selectionStart(), _cursor.selectionEnd());
    while (position < lastPosition) {
        const auto block = _cursor.document()->findBlock(position);
        const auto textFormats = block.textFormats();
        if (!textFormats.isEmpty()) {
            for (const auto& format : textFormats) {
                const auto formatStart = block.position() + format.start;
                const auto formatEnd = formatStart + format.length;
                if (position >= formatEnd) {
                    continue;
                } else if (formatStart >= lastPosition) {
                    break;
                }

                _cursor.setPosition(std::max(formatStart, position));
                _cursor.setPosition(std::min(formatEnd, lastPosition), QTextCursor::KeepAnchor);

                const auto newFormat = _updateFormat(format.format);
                _cursor.mergeCharFormat(newFormat);

                _cursor.clearSelection();
                position = _cursor.position();
            }
        } else {
            _cursor.setPosition(block.position());

            const auto newFormat = _updateFormat(block.charFormat());
            _cursor.mergeBlockCharFormat(newFormat);
        }

        if (!block.next().isValid()) {
            break;
        }

        position = block.next().position();
    }

    _cursor.endEditBlock();
}
