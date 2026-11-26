#include "text_helper.h"

#include "measurement_helper.h"

#include <business_layer/templates/text_template.h>
#include <utils/diff_match_patch/diff_match_patch.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QCache>
#include <QDebug>
#include <QElapsedTimer>
#include <QFontMetricsF>
#include <QImage>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QThread>
#include <QtMath>


QPaintDevice* TextHelper::documentPaintDevice()
{
    // A process-lifetime device: layouts retain a non-owning pointer to it.
    // Never paint into this image or change its metrics after initialization.
    static QImage device = [] {
        QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
        const auto dotsPerMeter = qRound(MeasurementHelper::kDocumentDpi / 0.0254);
        image.setDotsPerMeterX(dotsPerMeter);
        image.setDotsPerMeterY(dotsPerMeter);
        return image;
    }();
    return &device;
}

void TextHelper::configureDocument(QTextDocument* _document)
{
    _document->setUseDesignMetrics(true);
    _document->documentLayout()->setPaintDevice(documentPaintDevice());
}

qreal TextHelper::fontPointSize(const QFont& _font)
{
    return _font.pointSizeF() > 0 ? _font.pointSizeF()
                                  : MeasurementHelper::pxToPt(_font.pixelSize());
}

QFont TextHelper::fontForPaintDevice(const QFont& _font, const QPaintDevice* _device)
{
    auto font = _font;
    if (font.pointSizeF() > 0) {
        // QPainter resolves point sizes against its target device even when the
        // painter already has a document-to-screen transform.
        font.setPointSizeF(font.pointSizeF() * MeasurementHelper::kDocumentDpi
                           / _device->logicalDpiY());
    }
    return font;
}

qreal TextHelper::fineTextWidthF(const QString& _text, const QFont& _font)
{
    return fineTextWidthF(_text, QFontMetricsF(_font, documentPaintDevice()));
}

qreal TextHelper::fineTextWidthF(const QString& _text, const QFontMetricsF& _metrics)
{
    return _metrics.horizontalAdvance(_text);
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
    return QFontMetricsF(_font, documentPaintDevice()).lineSpacing();
}

void TextHelper::updateFontHinting(QFont& _font)
{
    _font.setHintingPreference(QFont::PreferNoHinting);
}

qreal TextHelper::heightForWidth(const QString& _text, const QFont& _font, qreal _width)
{
    const QFontMetricsF metrics(_font, documentPaintDevice());
    return metrics
        .boundingRect(QRectF(0, 0, _width, 0), // width constraint, height=0 means unlimited
                      Qt::TextWordWrap | Qt::AlignLeft, _text)
        .height();
}

qreal TextHelper::layoutHeightForWidth(const QString& _text, const QFont& _font, qreal _width,
                                       qreal _lineHeight)
{
    if (_text.isEmpty() || _width <= 0.0) {
        return 0.0;
    }

    const auto lineHeight = qFuzzyCompare(_lineHeight, -1) ? fineLineSpacing(_font) : _lineHeight;
    if (lineHeight <= 0.0) {
        return 0.0;
    }

    auto correctedText = _text;
    correctedText.replace(QLatin1Char('\n'), QChar::LineSeparator);

    QTextLayout textLayout(correctedText, _font, documentPaintDevice());
    QTextOption textOption;
    textOption.setUseDesignMetrics(true);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textLayout.setTextOption(textOption);

    qreal height = 0.0;
    textLayout.beginLayout();
    while (true) {
        auto line = textLayout.createLine();
        if (!line.isValid()) {
            break;
        }

        line.setLineWidth(_width);
        line.setPosition(QPointF(0.0, height));
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
    QTextLayout textLayout(correctedText, _font, documentPaintDevice());
    QTextOption textOption;
    textOption.setUseDesignMetrics(true);
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

namespace {
QString elidedTextUncached(const QString& _text, const QFont& _font, const QRectF& _rect)
{
    QFontMetricsF metrics(_font, TextHelper::documentPaintDevice());

    //
    // Если влезает целиком, сразу выходим
    //
    if (metrics.boundingRect(_rect, Qt::TextWordWrap, _text).height() <= _rect.height()) {
        return _text;
    }

    QString elidedText;
    QString lastLine;

    QTextLayout layout(_text, _font, TextHelper::documentPaintDevice());
    auto option = layout.textOption();
    option.setUseDesignMetrics(true);
    layout.setTextOption(option);
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
            while (lastLine.length() > 1
                   && TextHelper::fineTextWidthF(lastLine, _font) > _rect.width()) {
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
} // namespace

QString TextHelper::elidedText(const QString& _text, const QFont& _font, const QRectF& _rect)
{
    //
    // Delegates repeatedly elide the same previews during hover, scrolling and click animations.
    // Keep the existing layout algorithm, and bound retained keys and results to 1 MiB.
    // Background callers use the uncached path so the GUI cache needs no synchronization.
    //

    if (QApplication::instance() == nullptr
        || QThread::currentThread() != QApplication::instance()->thread() || _text.size() > 16384) {
        return elidedTextUncached(_text, _font, _rect);
    }

    static QCache<QByteArray, QString> cache(1024 * 1024);

    QByteArray key;
    QDataStream stream(&key, QIODevice::WriteOnly);
    stream << _text << _font << _rect.size() << QFontMetricsF(_font).fontDpi();
    if (const auto result = cache.object(key)) {
        return *result;
    }

    const auto result = elidedTextUncached(_text, _font, _rect);
    const auto cost = key.size() + result.size() * int(sizeof(QChar)) + 256;
    cache.insert(key, new QString(result), cost);
    return result;
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
    return QRegularExpression::escape(_text);
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

QString TextHelper::removeControlCharacters(const QString& _text)
{
    QString result = _text;
    //
    // Убираем zero-width no-break пробел, также известный как BOM-метка
    //
    result.remove(QChar(65279));
    //
    // Убираем управляющие символы табуляции
    //
    result.remove(QChar('\0'));
    result.remove(QChar('\a'));
    result.remove(QChar('\b'));
    result.remove(QChar('\v'));
    result.remove(QChar('\f'));
    result.remove(QChar('\r'));

    return result;
}

QString TextHelper::simplified(const QString& _text, bool _keepLineBreaks)
{
    QString result = removeControlCharacters(_text);

    //
    // Убираем оставшийся whitespace в зависимости от необходимости сохранять переносы строк
    //
    if (_keepLineBreaks) {
        result = result.replace('\t', ' ');
        result = result.replace("  ", " ");
    } else {
        result = result.simplified();
    }

    return result;
}

QString TextHelper::toSentenceCase(const QString& _text, bool _capitalizeEveryWord,
                                   bool _capitalizeEverySentence)
{
    if (_text.isEmpty()) {
        return {};
    }

    if (_capitalizeEveryWord) {
        bool isNextWord = true;
        QString result;
        for (int index = 0; index < _text.length(); ++index) {
            const auto nextCharacter = _text[index];

            if (nextCharacter.isSpace() || nextCharacter.isPunct()) {
                isNextWord = true;
                result += nextCharacter;
                continue;
            }

            if (isNextWord) {
                isNextWord = false;
                result += smartToUpper(nextCharacter);
            } else {
                result += smartToLower(nextCharacter);
            }
        }

        return result;
    }

    if (_capitalizeEverySentence) {
        const QSet<QString> endOfSentence = { ".", "!", "?", "…" };
        bool isFirstSentenceWord = true;
        QString result;
        for (int index = 0; index < _text.length(); ++index) {
            const auto nextCharacter = _text[index];

            if (nextCharacter.isSpace()) {
                result += nextCharacter;
                continue;
            }

            if (endOfSentence.contains(nextCharacter)) {
                isFirstSentenceWord = true;
                result += nextCharacter;
                continue;
            }

            if (isFirstSentenceWord) {
                isFirstSentenceWord = false;
                result += smartToUpper(nextCharacter);
            } else {
                result += smartToLower(nextCharacter);
            }
        }

        return result;
    }

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

void TextHelper::clearSelectionFormatting(QTextCursor& _cursor)
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

                //
                // Пропускаем выделения с комментариями, т.к. пользователи считывают их не как
                // формат текста, а как отдельные сущности и поэтому для них неожиданно, когда
                // комментарии удаляются при очистке формата текста
                //
                if (const auto comments
                    = format.format.property(BusinessLayer::TextBlockStyle::PropertyComments)
                          .toStringList();
                    !comments.isEmpty() && !comments.constFirst().isEmpty()) {
                    continue;
                }

                _cursor.setPosition(std::max(formatStart, position));
                _cursor.setPosition(std::min(formatEnd, lastPosition), QTextCursor::KeepAnchor);
                _cursor.setCharFormat(_cursor.block().charFormat());

                _cursor.clearSelection();
                position = _cursor.position();
            }
        }

        if (!block.next().isValid()) {
            break;
        }

        position = block.next().position();
    }

    _cursor.endEditBlock();
}

void TextHelper::updateSelectionFormatting(
    QTextCursor& _cursor, std::function<QTextCharFormat(const QTextCharFormat&)> _updateFormat)
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
            _cursor.setPosition(position);
            const auto endPosition = std::min(position + _cursor.block().length(), lastPosition);
            _cursor.setPosition(endPosition);

            const auto newFormat = _updateFormat(block.charFormat());
            _cursor.mergeCharFormat(newFormat);

            _cursor.clearSelection();
        }

        if (!block.next().isValid()) {
            break;
        }

        position = block.next().position();
    }

    _cursor.endEditBlock();
}

void TextHelper::applyTextFormattingForBlock(QTextCursor& _cursor,
                                             const QTextCharFormat& _newFormat)
{
    _cursor.movePosition(QTextCursor::StartOfBlock);

    //
    // Если в блоке есть кастомные форматы текста, то обновляем только те параметры форматов,
    // которые соответствую дефолтным во всём абзаце, а кастомные оставляем без изменений
    //
    QTextBlock currentBlock = _cursor.block();
    const auto currentBlockCharFormat = currentBlock.charFormat();
    if (!currentBlock.textFormats().isEmpty()) {
        const auto formats = currentBlock.textFormats();
        for (const auto& range : formats) {
            auto rangeFormat = range.format;
            if (rangeFormat.fontFamilies() == currentBlockCharFormat.fontFamilies()) {
                rangeFormat.setFontFamilies(_newFormat.fontFamilies().toStringList());
            }
            if (rangeFormat.fontFamilies() == currentBlockCharFormat.fontFamilies()) {
                rangeFormat.setFontFamilies(_newFormat.fontFamilies().toStringList());
            }
            if (qFuzzyCompare(fontPointSize(rangeFormat.font()),
                              fontPointSize(currentBlockCharFormat.font()))) {
                auto font = rangeFormat.font();
                font.setPointSizeF(fontPointSize(_newFormat.font()));
                rangeFormat.setFont(font);
            }
            if (rangeFormat.fontCapitalization() == currentBlockCharFormat.fontCapitalization()) {
                rangeFormat.setFontCapitalization(_newFormat.fontCapitalization());
            }
            if (rangeFormat.fontWeight() == currentBlockCharFormat.fontWeight()) {
                rangeFormat.setFontWeight(_newFormat.fontWeight());
            }
            if (rangeFormat.fontItalic() == currentBlockCharFormat.fontItalic()) {
                rangeFormat.setFontItalic(_newFormat.fontItalic());
            }
            if (rangeFormat.fontUnderline() == currentBlockCharFormat.fontUnderline()) {
                rangeFormat.setFontUnderline(_newFormat.fontUnderline());
            }
            if (rangeFormat.fontStrikeOut() == currentBlockCharFormat.fontStrikeOut()) {
                rangeFormat.setFontStrikeOut(_newFormat.fontStrikeOut());
            }

            _cursor.setPosition(currentBlock.position() + range.start);
            _cursor.setPosition(_cursor.position() + range.length, QTextCursor::KeepAnchor);
            _cursor.mergeCharFormat(rangeFormat);
        }
    }
    //
    // Если выделений нет, обновляем блок целиком
    //
    else {
        _cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        _cursor.mergeCharFormat(_newFormat);
    }

    _cursor.clearSelection();
}

QTextCharFormat TextHelper::fineBlockCharFormat(const QTextBlock& _block)
{
    //
    // Если в блоке лишь один формат, охватывающий весь текст блока, то его и будем считать
    // правильным форматом блока
    //
    if (_block.textFormats().size() == 1 && _block.textFormats().constFirst().start == 0
        && _block.textFormats().constFirst().length == _block.length() - 1) {
        return _block.textFormats().constFirst().format;
    }

    //
    // В противном случае возвращаем дефолтный формат
    //
    return _block.charFormat();
}

bool TextHelper::isUppercase(const QString& _text)
{
    return _text == TextHelper::smartToUpper(_text);
}

namespace {

//
// Короткий уникальный контекст позволяет найти неизменившийся текст за линейное время.
//
constexpr int kContextLength = 64;
constexpr int kDiffBudgetMs = 20;

int findContext(const QString& _before, const QString& _after, int _start, int _length)
{
    if (_start < 0 || _length < kContextLength || _start + _length > _before.size()) {
        return -1;
    }

    const auto context = _before.mid(_start, _length);
    if (_before.indexOf(context) != _start || _before.indexOf(context, _start + 1) != -1) {
        return -1;
    }
    const auto found = _after.indexOf(context);
    return found >= 0 && _after.indexOf(context, found + 1) == -1 ? found : -1;
}

} // namespace

QVector<int> TextHelper::mapTextPositions(const QString& _before, const QString& _after,
                                          const QVector<int>& _positions)
{
    if (_positions.isEmpty()) {
        return {};
    }

    const int beforeSize = _before.size();
    const int afterSize = _after.size();
    QElapsedTimer elapsed;
    elapsed.start();
    diff_match_patch dmp;
    const auto diff = [&](const QString& _left, const QString& _right) {
        // Общий бюджет для локальных сравнений и резервного сравнения. Нулевой timeout у DMP
        // означает бесконечность, поэтому даже после исчерпания бюджета оставляем минимум.
        dmp.Diff_Timeout = std::max(1, kDiffBudgetMs - int(elapsed.elapsed())) / 1000.0f;
        return dmp.diff_main(_left.isNull() ? QStringLiteral("") : _left,
                             _right.isNull() ? QStringLiteral("") : _right, false);
    };

    const int prefix = dmp.diff_commonPrefix(_before, _after);
    const int suffix = std::min(
        { dmp.diff_commonSuffix(_before, _after), beforeSize - prefix, afterSize - prefix });
    std::map<int, int> mapped;
    for (const int original : _positions) {
        const int position = std::clamp(original, 0, beforeSize);
        if (mapped.count(position) != 0) {
            continue;
        }
        auto& result = mapped[position];
        result = -1;
        if (position == beforeSize) {
            result = afterSize;
        } else if (position < prefix) {
            result = position;
        } else if (suffix > 0 && position >= beforeSize - suffix) {
            result = position + afterSize - beforeSize;
        } else {
            const int start = std::clamp(position - kContextLength / 2, 0,
                                         std::max(0, beforeSize - kContextLength));
            const int found = findContext(_before, _after, start, kContextLength);
            if (found >= 0) {
                result = found + position - start;
                continue;
            }

            // Если менялся сам текст под курсором, ограничим diff ближайшими неизменными
            // контекстами слева и справа. Начало/конец документа тоже служат границами.
            int oldLeft = 0;
            int newLeft = position < kContextLength ? 0 : -1;
            int oldRight = beforeSize;
            int newRight = beforeSize - position < kContextLength ? afterSize : -1;
            for (const int distance : { 0, 64, 256, 1024 }) {
                if (newLeft < 0) {
                    const int left = position - distance - kContextLength;
                    const int match = findContext(_before, _after, left, kContextLength);
                    if (match >= 0) {
                        oldLeft = left + kContextLength;
                        newLeft = match + kContextLength;
                    }
                }
                if (newRight < 0) {
                    const int right = position + distance;
                    const int match = findContext(_before, _after, right, kContextLength);
                    if (match >= 0) {
                        oldRight = right;
                        newRight = match;
                    }
                }
                if (newLeft >= 0 && newRight >= 0) {
                    break;
                }
            }
            if (newLeft >= 0 && newRight >= newLeft) {
                const auto changes = diff(_before.mid(oldLeft, oldRight - oldLeft),
                                          _after.mid(newLeft, newRight - newLeft));
                result = newLeft + dmp.diff_xIndex(changes, position - oldLeft);
            }
        }
    }

    // Переставленные фрагменты могут дать несовместимые ориентиры. В этом случае переносим
    // все позиции одним diff, чтобы не перевернуть направление выделения.
    bool ordered = true;
    int previous = -1;
    bool needsFallback = false;
    for (const auto& entry : mapped) {
        if (entry.second < 0) {
            needsFallback = true;
            continue;
        }
        ordered = ordered && entry.second >= previous;
        previous = entry.second;
    }
    if (needsFallback || !ordered) {
        const auto changes = diff(_before, _after);
        // Один способ отображения для всех позиций сохраняет порядок и согласованность
        // курсора, выделения и привязки прокрутки даже при повторяющемся тексте.
        for (auto& entry : mapped) {
            entry.second = dmp.diff_xIndex(changes, entry.first);
        }
    }

    QVector<int> result;
    result.reserve(_positions.size());
    for (const int position : _positions) {
        result.append(std::clamp(mapped.at(std::clamp(position, 0, beforeSize)), 0, afterSize));
    }
    return result;
}
