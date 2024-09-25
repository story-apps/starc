
#include "abstract_markdown_exporter.h"

#include <business_layer/document/novel/text/novel_text_document.h>
#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/templates/novel_template.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>

#include <QGuiApplication>
#include <QVector>

namespace BusinessLayer {

namespace {
constexpr int kInvalidPosition = -1;
}

class AbstractMarkdownExporter::Implementation
{
public:
    explicit Implementation(AbstractMarkdownExporter* _q, const QSet<QChar> _escapedCharacters);

    /**
     * @brief Получить индекс для вставки закрывающего формата
     */
    int closeFormatIndex(const QString& _paragraph,
                         const QTextLayout::FormatRange _currentFormatRange) const;

    /**
     * @brief Сформировать закрывающую форматную строку
     */
    QString closeFormatString(const QVector<QTextLayout::FormatRange>& formats,
                              int _formatIndex) const;

    /**
     * @brief Заэкранировать специальные символы в заданном диапазоне
     */
    void escapeCharacters(QString& _paragraph, int _begin, int _end) const;

    /**
     * @brief Преобразовать разницу в двух форматах в строку, закрывающую формат
     */
    QString formatsCloseDiffToString(const QTextCharFormat& _current,
                                     const QTextCharFormat& _next) const;

    /**
     * @brief Преобразовать разницу в двух форматах в строку, открывающую формат
     */
    QString formatsOpenDiffToString(const QTextCharFormat& _current,
                                    const QTextCharFormat& _next) const;

    /**
     * @brief Получить индекс для вставки открывающего формата
     */
    int openFormatIndex(const QString& _paragraph,
                        const QTextLayout::FormatRange _nextFormatRange) const;

    /**
     * @brief Сформировать открывающую форматную строку
     */
    QString openFormatString(const QVector<QTextLayout::FormatRange>& formats,
                             int _formatIndex) const;


    AbstractMarkdownExporter* q = nullptr;

    /**
     * @brief Экранируемые символы
     */
    const QSet<QChar> escapedCharacters;
};

AbstractMarkdownExporter::Implementation::Implementation(AbstractMarkdownExporter* _q,
                                                         const QSet<QChar> _escapedCharacters)
    : q(_q)
    , escapedCharacters(_escapedCharacters)
{
}

int AbstractMarkdownExporter::Implementation::closeFormatIndex(
    const QString& _paragraph, const QTextLayout::FormatRange _currentFormatRange) const
{
    int index = _currentFormatRange.start + _currentFormatRange.length;
    //
    // Вставлять закрывающую форматную строку будем сразу после непробельных символов текущего
    // формата
    //
    while (index > 0 && _paragraph[index - 1].isSpace()) {
        --index;
    }
    return index;
}

QString AbstractMarkdownExporter::Implementation::closeFormatString(
    const QVector<QTextLayout::FormatRange>& formats, int _formatIndex) const
{
    //
    // Собираем текущие форматы
    //
    QSet<TextSelectionTypes> currentFormats;
    if (formats[_formatIndex].format.font().bold()) {
        currentFormats.insert(TextSelectionTypes::Bold);
    }
    if (formats[_formatIndex].format.font().italic()) {
        currentFormats.insert(TextSelectionTypes::Italic);
    }
    if (formats[_formatIndex].format.font().strikeOut()) {
        currentFormats.insert(TextSelectionTypes::StrikeOut);
    }
    if (formats[_formatIndex].format.font().underline()) {
        currentFormats.insert(TextSelectionTypes::Underline);
    }

    QString formatString;

    //
    // Смотрим в какой последовательности открываются текущие форматы
    // и в соответствии с этим формируем форматную строку
    //
    for (int index = _formatIndex - 1; index >= 0 && !currentFormats.isEmpty(); --index) {
        if (!formats[index].format.font().bold()
            && currentFormats.contains(TextSelectionTypes::Bold)) {
            formatString.append(q->formatSymbols(TextSelectionTypes::Bold));
            currentFormats.remove(TextSelectionTypes::Bold);
        }
        if (!formats[index].format.font().italic()
            && currentFormats.contains(TextSelectionTypes::Italic)) {
            formatString.append(q->formatSymbols(TextSelectionTypes::Italic));
            currentFormats.remove(TextSelectionTypes::Italic);
        }
        if (!formats[index].format.font().strikeOut()
            && currentFormats.contains(TextSelectionTypes::StrikeOut)) {
            formatString.append(q->formatSymbols(TextSelectionTypes::StrikeOut));
            currentFormats.remove(TextSelectionTypes::StrikeOut);
        }
        if (!formats[index].format.font().underline()
            && currentFormats.contains(TextSelectionTypes::Underline)) {
            formatString.append(q->formatSymbols(TextSelectionTypes::Underline));
            currentFormats.remove(TextSelectionTypes::Underline);
        }
    }

    //
    // ... если первый формат такой же, как и текущий, то в цикле он обработан не был
    //
    if (!currentFormats.isEmpty()) {
        if (currentFormats.contains(TextSelectionTypes::Bold)) {
            formatString.append(q->formatSymbols(TextSelectionTypes::Bold));
        }
        if (currentFormats.contains(TextSelectionTypes::Italic)) {
            formatString.append(q->formatSymbols(TextSelectionTypes::Italic));
        }
        if (currentFormats.contains(TextSelectionTypes::StrikeOut)) {
            formatString.append(q->formatSymbols(TextSelectionTypes::StrikeOut));
        }
        if (currentFormats.contains(TextSelectionTypes::Underline)) {
            formatString.append(q->formatSymbols(TextSelectionTypes::Underline));
        }
    }

    return formatString;
}

void AbstractMarkdownExporter::Implementation::escapeCharacters(QString& _paragraph, int _begin,
                                                                int _end) const
{
    for (int index = _end; index >= _begin; --index) {
        if (escapedCharacters.contains(_paragraph[index])) {
            _paragraph.insert(index, '\\');
        }
    }
}

QString AbstractMarkdownExporter::Implementation::formatsCloseDiffToString(
    const QTextCharFormat& _current, const QTextCharFormat& _next) const
{
    QTextCharFormat diff;
    diff.setFontWeight(_current.font().bold() && !_next.font().bold() ? QFont::Bold
                                                                      : QFont::Normal);
    diff.setFontItalic(_current.font().italic() && !_next.font().italic());
    diff.setFontStrikeOut(_current.font().strikeOut() && !_next.font().strikeOut());
    diff.setFontUnderline(_current.font().underline() && !_next.font().underline());

    QString formatString;
    if (diff.font().bold()) {
        formatString.append(q->formatSymbols(TextSelectionTypes::Bold));
    }
    if (diff.font().italic()) {
        formatString.append(q->formatSymbols(TextSelectionTypes::Italic));
    }
    if (diff.font().strikeOut()) {
        formatString.append(q->formatSymbols(TextSelectionTypes::StrikeOut));
    }
    if (diff.font().underline()) {
        formatString.append(q->formatSymbols(TextSelectionTypes::Underline));
    }

    return formatString;
}

QString AbstractMarkdownExporter::Implementation::formatsOpenDiffToString(
    const QTextCharFormat& _current, const QTextCharFormat& _next) const
{
    QTextCharFormat diff;
    diff.setFontWeight(!_current.font().bold() && _next.font().bold() ? QFont::Bold
                                                                      : QFont::Normal);
    diff.setFontItalic(!_current.font().italic() && _next.font().italic());
    diff.setFontStrikeOut(!_current.font().strikeOut() && _next.font().strikeOut());
    diff.setFontUnderline(!_current.font().underline() && _next.font().underline());

    QString formatString;
    if (diff.font().bold()) {
        formatString.prepend(q->formatSymbols(TextSelectionTypes::Bold));
    }
    if (diff.font().italic()) {
        formatString.prepend(q->formatSymbols(TextSelectionTypes::Italic));
    }
    if (diff.font().strikeOut()) {
        formatString.prepend(q->formatSymbols(TextSelectionTypes::StrikeOut));
    }
    if (diff.font().underline()) {
        formatString.prepend(q->formatSymbols(TextSelectionTypes::Underline));
    }
    return formatString;
}

int AbstractMarkdownExporter::Implementation::openFormatIndex(
    const QString& _paragraph, const QTextLayout::FormatRange _nextFormatRange) const
{
    int index = _nextFormatRange.start;
    //
    // Вставлять открывающую форматную строку будем непосредственно перед непробельными
    // символами следующего формата, иначе Markdown неправильно нас поймет
    //
    while (index < _paragraph.length() && _paragraph[index].isSpace()) {
        ++index;
    }
    return index;
}

QString AbstractMarkdownExporter::Implementation::openFormatString(
    const QVector<QTextLayout::FormatRange>& formats, int _formatIndex) const
{
    //
    // Собираем текущие форматы
    //
    QSet<TextSelectionTypes> currentFormats;
    if (formats[_formatIndex].format.font().bold()) {
        currentFormats.insert(TextSelectionTypes::Bold);
    }
    if (formats[_formatIndex].format.font().italic()) {
        currentFormats.insert(TextSelectionTypes::Italic);
    }
    if (formats[_formatIndex].format.font().strikeOut()) {
        currentFormats.insert(TextSelectionTypes::StrikeOut);
    }
    if (formats[_formatIndex].format.font().underline()) {
        currentFormats.insert(TextSelectionTypes::Underline);
    }

    QString formatString;

    //
    // Смотрим в какой последовательности закрываются текущие форматы
    // и в соответствии с этим формируем форматную строку
    //
    for (int index = _formatIndex + 1; index < formats.size() && !currentFormats.isEmpty();
         ++index) {
        if (!formats[index].format.font().bold()
            && currentFormats.contains(TextSelectionTypes::Bold)) {
            formatString.prepend(q->formatSymbols(TextSelectionTypes::Bold));
            currentFormats.remove(TextSelectionTypes::Bold);
        }
        if (!formats[index].format.font().italic()
            && currentFormats.contains(TextSelectionTypes::Italic)) {
            formatString.prepend(q->formatSymbols(TextSelectionTypes::Italic));
            currentFormats.remove(TextSelectionTypes::Italic);
        }
        if (!formats[index].format.font().strikeOut()
            && currentFormats.contains(TextSelectionTypes::StrikeOut)) {
            formatString.prepend(q->formatSymbols(TextSelectionTypes::StrikeOut));
            currentFormats.remove(TextSelectionTypes::StrikeOut);
        }
        if (!formats[index].format.font().underline()
            && currentFormats.contains(TextSelectionTypes::Underline)) {
            formatString.prepend(q->formatSymbols(TextSelectionTypes::Underline));
            currentFormats.remove(TextSelectionTypes::Underline);
        }
    }

    //
    // ... если последний формат такой же, как и текущий, то в цикле он обработан не был
    //
    if (!currentFormats.isEmpty()) {
        if (currentFormats.contains(TextSelectionTypes::Bold)) {
            formatString.prepend(q->formatSymbols(TextSelectionTypes::Bold));
        }
        if (currentFormats.contains(TextSelectionTypes::Italic)) {
            formatString.prepend(q->formatSymbols(TextSelectionTypes::Italic));
        }
        if (currentFormats.contains(TextSelectionTypes::StrikeOut)) {
            formatString.prepend(q->formatSymbols(TextSelectionTypes::StrikeOut));
        }
        if (currentFormats.contains(TextSelectionTypes::Underline)) {
            formatString.prepend(q->formatSymbols(TextSelectionTypes::Underline));
        }
    }

    return formatString;
}


// ****


AbstractMarkdownExporter::AbstractMarkdownExporter(const QSet<QChar> _escapedCharacters)
    : d(new Implementation(this, _escapedCharacters))
{
}

AbstractMarkdownExporter::~AbstractMarkdownExporter() = default;

void AbstractMarkdownExporter::exportTo(AbstractModel* _model, ExportOptions& _exportOptions) const
{
    exportTo(_model, kInvalidPosition, kInvalidPosition, _exportOptions);
}

void AbstractMarkdownExporter::exportTo(AbstractModel* _model, int _fromPosition, int _toPosition,
                                        ExportOptions& _exportOptions) const
{
    //
    // Открываем документ на запись
    //
    QFile markdownFile(_exportOptions.filePath);
    if (!markdownFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QScopedPointer<TextDocument> document(prepareDocument(_model, _exportOptions));

    //
    // Если задан интервал для экспорта, корректируем документ в соответствии с ним
    //
    if (_fromPosition != kInvalidPosition && _toPosition != kInvalidPosition
        && _fromPosition < _toPosition) {
        TextCursor cursor(document.data());
        cursor.beginEditBlock();
        cursor.setPosition(_toPosition);
        cursor.movePosition(TextCursor::End, TextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setPosition(0);
        cursor.setPosition(_fromPosition, TextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.endEditBlock();
    }

    //
    // Будем запоминать тип предыдущего записанного блока для формирования нужного количества
    // отступов
    //
    TextParagraphType previousBlockType = TextParagraphType::Undefined;

    for (auto block = document->begin(); block.isValid(); block = block.next()) {
        if (block.text().isEmpty()) {
            continue;
        }

        //
        // Извлечем список форматов и редакторских заметок
        //
        QVector<QTextLayout::FormatRange> formats;

        //
        // Не знаю, какая это магия, но если вместо этого цикла использовать remove_copy_if
        // или copy_if, то получаем сегфолт
        //
        for (const QTextLayout::FormatRange& format : block.textFormats()) {
            if (format.format != block.charFormat()) {
                formats.push_back(format);
            }
        }

        QString paragraphText = block.text();

        //
        // При вставке форматных строк будем экранировать специальные символы (начиная с конца),
        // чтобы они не сбивали форматирование.
        // Для этого заведем переменную, отслеживающую положение обработанной части
        //
        int escapedRange = paragraphText.size() - 1;

        //
        // Обрабатывать форматирование надо с конца, чтобы не сбилась их позиция вставки
        //
        for (int formatIndex = formats.size() - 1; formatIndex >= 0; --formatIndex) {
            //
            // Пишем закрывающий формат
            //
            // Смотрим на следующий формат
            //
            const int nextFormatIndex = formatIndex + 1;
            //
            // ... если он есть, то сравниваем форматы и пишем только необходимое (то,
            // которое не дублируется с последующим)
            //
            if (nextFormatIndex < formats.size()
                && (formats[formatIndex].start + formats[formatIndex].length
                    == formats[nextFormatIndex].start)) {

                //
                // Получаем индекс для вставки открывающей форматной строки следующего формата
                //
                const int nextFormatStartIndex
                    = d->openFormatIndex(paragraphText, formats[nextFormatIndex]);

                //
                // Экранируем специальные символы до индекса (со стороны конца)
                //
                d->escapeCharacters(paragraphText, nextFormatStartIndex, escapedRange);
                escapedRange = nextFormatStartIndex - 1;

                //
                // Вставляем открывающую форматную строку следующего формата
                //
                paragraphText.insert(nextFormatStartIndex,
                                     d->formatsOpenDiffToString(formats[formatIndex].format,
                                                                formats[nextFormatIndex].format));

                //
                // Получаем индекс для вставки закрывающей форматной строки текущего формата
                //
                const int formatEndIndex = d->closeFormatIndex(paragraphText, formats[formatIndex]);

                //
                // Экранируем специальные символы до индекса (со стороны конца)
                //
                d->escapeCharacters(paragraphText, formatEndIndex, escapedRange);
                escapedRange = formatEndIndex - 1;

                //
                // Вставляем закрывающую форматную строку текущего формата
                //
                paragraphText.insert(formatEndIndex,
                                     d->formatsCloseDiffToString(formats[formatIndex].format,
                                                                 formats[nextFormatIndex].format));
            }
            //
            // ... а если это последний формат блока, то формируем полностью
            //
            else {
                //
                // Получаем индекс для вставки закрывающей форматной строки текущего формата
                //
                const int formatEndIndex = d->closeFormatIndex(paragraphText, formats[formatIndex]);

                //
                // Экранируем специальные символы до индекса (со стороны конца)
                //
                d->escapeCharacters(paragraphText, formatEndIndex, escapedRange);
                escapedRange = formatEndIndex - 1;

                //
                // Вставляем закрывающую форматную строку текущего формата
                //
                paragraphText.insert(formatEndIndex, d->closeFormatString(formats, formatIndex));
            }

            //
            // Смотрим на предыдущее форматирование
            //
            const int prevFormatIndex = formatIndex - 1;
            //
            // ... если есть, то ничего не пишем
            //
            if (prevFormatIndex >= 0
                && (formats[prevFormatIndex].start + formats[prevFormatIndex].length
                    == formats[formatIndex].start)) {
                //
                // Формат будет записан, при записи закрывающей части
                // предыдущего форматирования в следующем проходе
                //
            }
            //
            // ... если нет, то пишем открывающий формат
            //
            else {
                //
                // Получаем индекс для вставки открывающей форматной строки текущего формата
                //
                const int currentFormatStartIndex
                    = d->openFormatIndex(paragraphText, formats[formatIndex]);

                //
                // Экранируем специальные символы до индекса (со стороны конца)
                //
                d->escapeCharacters(paragraphText, currentFormatStartIndex, escapedRange);
                escapedRange = currentFormatStartIndex - 1;

                //
                // Вставляем открывающую форматную строку текущего формата
                //
                paragraphText.insert(currentFormatStartIndex,
                                     d->openFormatString(formats, formatIndex));
            }
        }

        //
        // Экранируем специальные символы до конца, если остались непроверенные
        //
        d->escapeCharacters(paragraphText, 0, escapedRange);

        //
        // Разрывы строк преобразуем в переносы строк
        //
        paragraphText = paragraphText.replace(QChar::LineSeparator, QChar::LineFeed);

        //
        // Обрабатываем блок
        //
        const bool isProcessed = processBlock(paragraphText, block, _exportOptions);

        //
        // Записываем только обработанные блоки
        //
        if (isProcessed) {
            addIndentationAtBegin(paragraphText, previousBlockType,
                                  TextBlockStyle::forBlock(block));
            markdownFile.write(paragraphText.toUtf8());
            previousBlockType = TextBlockStyle::forBlock(block);
        }
    }
    markdownFile.write("\n");
    markdownFile.close();
}

void AbstractMarkdownExporter::removeWhitespaceAtBegin(QString& _paragraph) const
{
    int spaceIndex = 0;
    while (spaceIndex < _paragraph.size() && _paragraph[spaceIndex].isSpace()) {
        ++spaceIndex;
    }
    _paragraph.remove(0, spaceIndex);
}

} // namespace BusinessLayer
