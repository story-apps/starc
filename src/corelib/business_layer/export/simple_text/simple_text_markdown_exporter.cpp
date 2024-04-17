#include "simple_text_markdown_exporter.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/simple_text_template.h>

#include <QFile>


namespace BusinessLayer {

namespace {

/**
 * @brief Форматы, которые подерживает markdown
 */
enum MarkdownFormats {
    Bold,
    Italic,
    StrikeOut,
    ChapterHeading,
};

/**
 * @brief Символы форматирования
 */
static const QVector<QPair<MarkdownFormats, QString>> kFormatSymbols{
    { Bold, "**" },
    { Italic, "_" },
    { StrikeOut, "~~" },
    { ChapterHeading, "#" },
};

//
// Формируем закрывающую форматную строку
//
static QString closeFormatString(const QVector<QTextLayout::FormatRange>& formats, int _formatIndex)
{
    //
    // Собираем текущие форматы
    //
    QSet<MarkdownFormats> currentFormats;
    if (formats[_formatIndex].format.font().bold()) {
        currentFormats.insert(MarkdownFormats::Bold);
    }
    if (formats[_formatIndex].format.font().italic()) {
        currentFormats.insert(MarkdownFormats::Italic);
    }
    if (formats[_formatIndex].format.font().strikeOut()) {
        currentFormats.insert(MarkdownFormats::StrikeOut);
    }

    QString formatString;

    //
    // Смотрим в какой последовательности открываются текущие форматы
    // и в соответствии с этим формируем форматную строку
    //
    for (int index = _formatIndex - 1; index >= 0 && !currentFormats.isEmpty(); --index) {
        if (!formats[index].format.font().bold()
            && currentFormats.contains(MarkdownFormats::Bold)) {
            formatString.append(kFormatSymbols[MarkdownFormats::Bold].second);
            currentFormats.remove(MarkdownFormats::Bold);
        }
        if (!formats[index].format.font().italic()
            && currentFormats.contains(MarkdownFormats::Italic)) {
            formatString.append(kFormatSymbols[MarkdownFormats::Italic].second);
            currentFormats.remove(MarkdownFormats::Italic);
        }
        if (!formats[index].format.font().strikeOut()
            && currentFormats.contains(MarkdownFormats::StrikeOut)) {
            formatString.append(kFormatSymbols[MarkdownFormats::StrikeOut].second);
            currentFormats.remove(MarkdownFormats::StrikeOut);
        }
    }

    //
    // ... если первый формат такой же, как и текущий, то в цикле он обработан не был
    //
    if (!currentFormats.isEmpty()) {
        if (currentFormats.contains(MarkdownFormats::Bold)) {
            formatString.append(kFormatSymbols[MarkdownFormats::Bold].second);
        }
        if (currentFormats.contains(MarkdownFormats::Italic)) {
            formatString.append(kFormatSymbols[MarkdownFormats::Italic].second);
        }
        if (currentFormats.contains(MarkdownFormats::StrikeOut)) {
            formatString.append(kFormatSymbols[MarkdownFormats::StrikeOut].second);
        }
    }

    return formatString;
}

//
// Формируем открывающую форматную строку
//
static QString openFormatString(const QVector<QTextLayout::FormatRange>& formats, int _formatIndex)
{
    //
    // Собираем текущие форматы
    //
    QSet<MarkdownFormats> currentFormats;
    if (formats[_formatIndex].format.font().bold()) {
        currentFormats.insert(MarkdownFormats::Bold);
    }
    if (formats[_formatIndex].format.font().italic()) {
        currentFormats.insert(MarkdownFormats::Italic);
    }
    if (formats[_formatIndex].format.font().strikeOut()) {
        currentFormats.insert(MarkdownFormats::StrikeOut);
    }

    QString formatString;

    //
    // Смотрим в какой последовательности закрываются текущие форматы
    // и в соответствии с этим формируем форматную строку
    //
    for (int index = _formatIndex + 1; index < formats.size() && !currentFormats.isEmpty();
         ++index) {
        if (!formats[index].format.font().bold()
            && currentFormats.contains(MarkdownFormats::Bold)) {
            formatString.prepend(kFormatSymbols[MarkdownFormats::Bold].second);
            currentFormats.remove(MarkdownFormats::Bold);
        }
        if (!formats[index].format.font().italic()
            && currentFormats.contains(MarkdownFormats::Italic)) {
            formatString.prepend(kFormatSymbols[MarkdownFormats::Italic].second);
            currentFormats.remove(MarkdownFormats::Italic);
        }
        if (!formats[index].format.font().strikeOut()
            && currentFormats.contains(MarkdownFormats::StrikeOut)) {
            formatString.prepend(kFormatSymbols[MarkdownFormats::StrikeOut].second);
            currentFormats.remove(MarkdownFormats::StrikeOut);
        }
    }

    //
    // ... если последний формат такой же, как и текущий, то в цикле он обработан не был
    //
    if (!currentFormats.isEmpty()) {
        if (currentFormats.contains(MarkdownFormats::Bold)) {
            formatString.prepend(kFormatSymbols[MarkdownFormats::Bold].second);
        }
        if (currentFormats.contains(MarkdownFormats::Italic)) {
            formatString.prepend(kFormatSymbols[MarkdownFormats::Italic].second);
        }
        if (currentFormats.contains(MarkdownFormats::StrikeOut)) {
            formatString.prepend(kFormatSymbols[MarkdownFormats::StrikeOut].second);
        }
    }

    return formatString;
}

/**
 * @brief Преобразовать разницу в двух форматах в строку, открывающую формат
 */
static QString formatsOpenDiffToString(const QTextCharFormat& _current,
                                       const QTextCharFormat& _next)
{
    QTextCharFormat diff;
    diff.setFontWeight(!_current.font().bold() && _next.font().bold() ? QFont::Bold
                                                                      : QFont::Normal);
    diff.setFontItalic(!_current.font().italic() && _next.font().italic());
    diff.setFontStrikeOut(!_current.font().strikeOut() && _next.font().strikeOut());

    QString formatString;
    if (diff.font().bold()) {
        formatString.prepend(kFormatSymbols[MarkdownFormats::Bold].second);
    }
    if (diff.font().italic()) {
        formatString.prepend(kFormatSymbols[MarkdownFormats::Italic].second);
    }
    if (diff.font().strikeOut()) {
        formatString.prepend(kFormatSymbols[MarkdownFormats::StrikeOut].second);
    }
    return formatString;
}

/**
 * @brief Преобразовать разницу в двух форматах в строку, закрывающую формат
 */
static QString formatsCloseDiffToString(const QTextCharFormat& _current,
                                        const QTextCharFormat& _next)
{
    QTextCharFormat diff;
    diff.setFontWeight(_current.font().bold() && !_next.font().bold() ? QFont::Bold
                                                                      : QFont::Normal);
    diff.setFontItalic(_current.font().italic() && !_next.font().italic());
    diff.setFontStrikeOut(_current.font().strikeOut() && !_next.font().strikeOut());

    QString formatString;
    if (diff.font().bold()) {
        formatString.append(kFormatSymbols[MarkdownFormats::Bold].second);
    }
    if (diff.font().italic()) {
        formatString.append(kFormatSymbols[MarkdownFormats::Italic].second);
    }
    if (diff.font().strikeOut()) {
        formatString.append(kFormatSymbols[MarkdownFormats::StrikeOut].second);
    }

    return formatString;
}

/**
 * @brief Заэкранировать специальные символы в заданном диапазоне
 */
static void escapeCharacters(QString& _paragraph, int _begin, int _end)
{
    const QSet<QChar> escapedCharacters
        = { '\\', '`', '*', '_', '{', '}', '[', ']', '(', ')', '#', '+', '-', '.', '!', '|', '~' };
    for (int index = _end; index >= _begin; --index) {
        if (escapedCharacters.contains(_paragraph[index])) {
            _paragraph.insert(index, '\\');
        }
    }
}

/**
 * @brief Получить индекс для вставки открывающего формата
 */
static int openFormatIndex(const QString& _paragraph,
                           const QTextLayout::FormatRange _nextFormatRange)
{
    int index = _nextFormatRange.start;
    //
    // Вставлять открывающую форматную строку будем непосредственно перед непробельными
    // символами следующего формата, иначе Markdown неправильно нас поймет
    //
    while (_paragraph[index].isSpace()) {
        ++index;
    }
    return index;
}

/**
 * @brief Получить индекс для вставки закрывающего формата
 */
static int closeFormatIndex(const QString& _paragraph,
                            const QTextLayout::FormatRange _currentFormatRange)
{
    int index = _currentFormatRange.start + _currentFormatRange.length;
    //
    // Вставлять закрывающую форматную строку будем сразу после непробельных символов текущего
    // формата
    //
    while (_paragraph[index - 1].isSpace()) {
        --index;
    }
    return index;
}

/**
 * @brief Очистить начало параграфа от пробельных символов
 */
static void removeWhitespaceAtBegin(QString& _paragraph)
{
    int spaceIndex = 0;
    while (_paragraph[spaceIndex].isSpace()) {
        ++spaceIndex;
    }
    _paragraph.remove(0, spaceIndex);
}

} // namespace

void SimpleTextMarkdownExporter::exportTo(AbstractModel* _model,
                                          ExportOptions& _exportOptions) const
{
    constexpr int invalidPosition = -1;
    exportTo(_model, invalidPosition, invalidPosition, _exportOptions);
}

void SimpleTextMarkdownExporter::exportTo(AbstractModel* _model, int _fromPosition, int _toPosition,
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
    if (_fromPosition != -1 && _toPosition != -1 && _fromPosition < _toPosition) {
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
            // Далее работаем с форматированием
            //

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
                    = openFormatIndex(paragraphText, formats[nextFormatIndex]);

                //
                // Экранируем специальные символы до индекса (со стороны конца)
                //
                escapeCharacters(paragraphText, nextFormatStartIndex, escapedRange);
                escapedRange = nextFormatStartIndex - 1;

                //
                // Вставляем открывающую форматную строку следующего формата
                //
                paragraphText.insert(nextFormatStartIndex,
                                     formatsOpenDiffToString(formats[formatIndex].format,
                                                             formats[nextFormatIndex].format));

                //
                // Получаем индекс для вставки закрывающей форматной строки текущего формата
                //
                const int formatEndIndex = closeFormatIndex(paragraphText, formats[formatIndex]);

                //
                // Экранируем специальные символы до индекса (со стороны конца)
                //
                escapeCharacters(paragraphText, formatEndIndex, escapedRange);
                escapedRange = formatEndIndex - 1;

                //
                // Вставляем закрывающую форматную строку текущего формата
                //
                paragraphText.insert(formatEndIndex,
                                     formatsCloseDiffToString(formats[formatIndex].format,
                                                              formats[nextFormatIndex].format));
            }
            //
            // ... а если это последний формат блока, то формируем полностью
            //
            else {
                //
                // Получаем индекс для вставки закрывающей форматной строки текущего формата
                //
                const int formatEndIndex = closeFormatIndex(paragraphText, formats[formatIndex]);

                //
                // Экранируем специальные символы до индекса (со стороны конца)
                //
                escapeCharacters(paragraphText, formatEndIndex, escapedRange);
                escapedRange = formatEndIndex - 1;

                //
                // Вставляем закрывающую форматную строку текущего формата
                //
                paragraphText.insert(formatEndIndex, closeFormatString(formats, formatIndex));
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
                    = openFormatIndex(paragraphText, formats[formatIndex]);

                //
                // Экранируем специальные символы до индекса (со стороны конца)
                //
                escapeCharacters(paragraphText, currentFormatStartIndex, escapedRange);
                escapedRange = currentFormatStartIndex - 1;

                //
                // Вставляем открывающую форматную строку текущего формата
                //
                paragraphText.insert(currentFormatStartIndex,
                                     openFormatString(formats, formatIndex));
            }
        }

        //
        // Экранируем специальные символы до конца, если остались непроверенные
        //
        escapeCharacters(paragraphText, 0, escapedRange);

        //
        // Разрывы строк преобразуем в переносы строк
        //
        paragraphText = paragraphText.replace(QChar::LineSeparator, QChar::LineFeed);

        //
        // Очистим пробельные символы в начале абзаца, чтобы не сбивалось форматирование
        //
        removeWhitespaceAtBegin(paragraphText);

        //
        // Форматируем блоки
        //
        auto formatToChapterHeading = [&paragraphText, &block, &document](int _level) {
            const QString prefix = kFormatSymbols[MarkdownFormats::ChapterHeading].second;
            paragraphText.prepend(prefix.repeated(_level) + " ");
            paragraphText.append("\n\n");

            if (block != document->begin()) {
                paragraphText.prepend("\n");
            }
        };

        switch (TextBlockStyle::forBlock(block)) {
        case TextParagraphType::ChapterHeading1: {
            formatToChapterHeading(1);
            break;
        }
        case TextParagraphType::ChapterHeading2: {
            formatToChapterHeading(2);
            break;
        }
        case TextParagraphType::ChapterHeading3: {
            formatToChapterHeading(3);
            break;
        }
        case TextParagraphType::ChapterHeading4: {
            formatToChapterHeading(4);
            break;
        }
        case TextParagraphType::ChapterHeading5: {
            formatToChapterHeading(5);
            break;
        }
        case TextParagraphType::ChapterHeading6: {
            formatToChapterHeading(6);
            break;
        }
        default:
            paragraphText.append("  \n");
            break;
        }

        markdownFile.write(paragraphText.toUtf8());
    }
    markdownFile.close();
}

} // namespace BusinessLayer
