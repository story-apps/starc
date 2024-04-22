#include "screenplay_fountain_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QFile>


namespace BusinessLayer {

namespace {

/**
 * @brief Список мест, в которые умеет фонтан
 */
const QStringList sceneHeadingStart = {
    QCoreApplication::translate("BusinessLayer::FountainExporter", "INT"),
    QCoreApplication::translate("BusinessLayer::FountainExporter", "EXT"),
    QCoreApplication::translate("BusinessLayer::FountainExporter", "EST"),
    QCoreApplication::translate("BusinessLayer::FountainExporter", "INT./EXT"),
    QCoreApplication::translate("BusinessLayer::FountainExporter", "INT/EXT"),
    QCoreApplication::translate("BusinessLayer::FountainExporter", "EXT./INT"),
    QCoreApplication::translate("BusinessLayer::FountainExporter", "EXT/INT"),
    QCoreApplication::translate("BusinessLayer::FountainExporter", "I/E"),
};

/**
 * @brief Форматы, которые подерживает fountain
 */
enum TextSelectionTypes {
    Bold,
    Italic,
    Underline,
};

/**
 * @brief Символы форматирования
 */
static const QVector<QPair<TextSelectionTypes, QString>> kFormatSymbols{
    { Bold, "**" },
    { Italic, "*" },
    { Underline, "_" },
};

//
// Формируем закрывающую форматную строку
//
static QString closeFormatString(const QVector<QTextLayout::FormatRange>& formats, int _formatIndex)
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
    if (formats[_formatIndex].format.font().underline()) {
        currentFormats.insert(TextSelectionTypes::Underline);
    }

    QString formatString;

    //
    // Смотрим в какой последовательности начинаются текущие форматы
    // и в соответствии с этим формируем форматную строку
    //
    for (int index = _formatIndex - 1; index >= 0 && !currentFormats.isEmpty(); --index) {
        if (!formats[index].format.font().bold()
            && currentFormats.contains(TextSelectionTypes::Bold)) {
            formatString.append(kFormatSymbols[TextSelectionTypes::Bold].second);
            currentFormats.remove(TextSelectionTypes::Bold);
        }
        if (!formats[index].format.font().italic()
            && currentFormats.contains(TextSelectionTypes::Italic)) {
            formatString.append(kFormatSymbols[TextSelectionTypes::Italic].second);
            currentFormats.remove(TextSelectionTypes::Italic);
        }
        if (!formats[index].format.font().underline()
            && currentFormats.contains(TextSelectionTypes::Underline)) {
            formatString.append(kFormatSymbols[TextSelectionTypes::Underline].second);
            currentFormats.remove(TextSelectionTypes::Underline);
        }
    }

    //
    // ... если первый формат такой же, как и текущий, то в цикле он обработан не был
    //
    if (!currentFormats.isEmpty()) {
        if (currentFormats.contains(TextSelectionTypes::Bold)) {
            formatString.append(kFormatSymbols[TextSelectionTypes::Bold].second);
        }
        if (currentFormats.contains(TextSelectionTypes::Italic)) {
            formatString.append(kFormatSymbols[TextSelectionTypes::Italic].second);
        }
        if (currentFormats.contains(TextSelectionTypes::Underline)) {
            formatString.append(kFormatSymbols[TextSelectionTypes::Underline].second);
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
    QSet<TextSelectionTypes> currentFormats;
    if (formats[_formatIndex].format.font().bold()) {
        currentFormats.insert(TextSelectionTypes::Bold);
    }
    if (formats[_formatIndex].format.font().italic()) {
        currentFormats.insert(TextSelectionTypes::Italic);
    }
    if (formats[_formatIndex].format.font().underline()) {
        currentFormats.insert(TextSelectionTypes::Underline);
    }

    QString formatString;

    //
    // Смотрим в какой последовательности заканчиваются текущие форматы
    // и в соответствии с этим формируем форматную строку
    //
    for (int index = _formatIndex + 1; index < formats.size() && !currentFormats.isEmpty();
         ++index) {
        if (!formats[index].format.font().bold()
            && currentFormats.contains(TextSelectionTypes::Bold)) {
            formatString.prepend(kFormatSymbols[TextSelectionTypes::Bold].second);
            currentFormats.remove(TextSelectionTypes::Bold);
        }
        if (!formats[index].format.font().italic()
            && currentFormats.contains(TextSelectionTypes::Italic)) {
            formatString.prepend(kFormatSymbols[TextSelectionTypes::Italic].second);
            currentFormats.remove(TextSelectionTypes::Italic);
        }
        if (!formats[index].format.font().underline()
            && currentFormats.contains(TextSelectionTypes::Underline)) {
            formatString.prepend(kFormatSymbols[TextSelectionTypes::Underline].second);
            currentFormats.remove(TextSelectionTypes::Underline);
        }
    }

    //
    // ... если последний формат такой же, как и текущий, то в цикле он обработан не был
    //
    if (!currentFormats.isEmpty()) {
        if (currentFormats.contains(TextSelectionTypes::Bold)) {
            formatString.prepend(kFormatSymbols[TextSelectionTypes::Bold].second);
        }
        if (currentFormats.contains(TextSelectionTypes::Italic)) {
            formatString.prepend(kFormatSymbols[TextSelectionTypes::Italic].second);
        }
        if (currentFormats.contains(TextSelectionTypes::Underline)) {
            formatString.prepend(kFormatSymbols[TextSelectionTypes::Underline].second);
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
    diff.setFontUnderline(!_current.font().underline() && _next.font().underline());

    QString formatString;
    if (diff.font().bold()) {
        formatString.prepend(kFormatSymbols[TextSelectionTypes::Bold].second);
    }
    if (diff.font().italic()) {
        formatString.prepend(kFormatSymbols[TextSelectionTypes::Italic].second);
    }
    if (diff.font().underline()) {
        formatString.prepend(kFormatSymbols[TextSelectionTypes::Underline].second);
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
    diff.setFontUnderline(_current.font().underline() && !_next.font().underline());

    QString formatString;
    if (diff.font().bold()) {
        formatString.append(kFormatSymbols[TextSelectionTypes::Bold].second);
    }
    if (diff.font().italic()) {
        formatString.append(kFormatSymbols[TextSelectionTypes::Italic].second);
    }
    if (diff.font().underline()) {
        formatString.append(kFormatSymbols[TextSelectionTypes::Underline].second);
    }

    return formatString;
}

/**
 * @brief Заэкранировать специальные символы в заданном диапазоне
 * @return Количество заэкранированных символов
 */
static int escapeCharacters(QString& _paragraph, int _begin, int _end)
{
    int count = 0;
    const QSet<QChar> escapedCharacters = { '*', '_' };
    for (int index = _end; index >= _begin; --index) {
        if (escapedCharacters.contains(_paragraph[index])) {
            _paragraph.insert(index, '\\');
            ++count;
        }
    }
    return count;
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

} // namespace

void ScreenplayFountainExporter::exportTo(AbstractModel* _model,
                                          ExportOptions& _exportOptions) const
{
    constexpr int invalidPosition = -1;
    exportTo(_model, invalidPosition, invalidPosition, _exportOptions);
}

void ScreenplayFountainExporter::exportTo(AbstractModel* _model, int _fromPosition, int _toPosition,
                                          ExportOptions& _exportOptions) const
{
    //
    // Открываем документ на запись
    //
    QFile fountainFile(_exportOptions.filePath);
    if (!fountainFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
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

    const auto& exportOptions = static_cast<const ScreenplayExportOptions&>(_exportOptions);

    //
    // TODO: Реализовать экспорт титульной страницы
    //
    //    //
    //    // При необходимости пишем титульную страницу
    //    //
    //    if (_exportParameters.printTilte) {
    //        auto writeLine = [&fountainFile](const QString& _key, const QString& _value) {
    //            if (!_key.isEmpty() && !_value.isEmpty()) {
    //                fountainFile.write(QString("%1: %2\n").arg(_key).arg(_value).toUtf8());
    //            }
    //        };
    //        auto writeLines = [&fountainFile](const QString& _key, const QString& _value) {
    //            if (!_key.isEmpty() && !_value.isEmpty()) {
    //                fountainFile.write(QString("%1:\n").arg(_key).toUtf8());
    //                for (const QString& line : _value.split("\n")) {
    //                    fountainFile.write(QString("   %1\n").arg(line).toUtf8());
    //                }
    //            }
    //        };
    //        writeLine("Title", _exportParameters.scriptName);
    //        writeLine("Credit", _exportParameters.scriptGenre);
    //        writeLine("Author", _exportParameters.scriptAuthor);
    //        writeLine("Source", _exportParameters.scriptAdditionalInfo);
    //        writeLine("Draft date", _exportParameters.scriptYear);
    //        writeLines("Contact", _exportParameters.scriptContacts);
    //        //
    //        // Пустая строка в конце
    //        //
    //        fountainFile.write("\n");
    //    }

    //
    // Является ли текущий блок первым
    //
    bool isFirst = true;

    //
    // Текущая глубина вложенности директорий
    //
    unsigned dirNesting = 0;

    //
    // Тип предыдущего блока
    //
    auto block = document->begin();
    while (block.isValid()) {
        QString paragraphText;
        if (!block.text().isEmpty()) {
            paragraphText = block.text();
            QVector<QTextLayout::FormatRange> formats;

            //
            // Извлечем список форматов и редакторских заметок
            //

            //
            // Не знаю, какая это магия, но если вместо этого цикла использовать remove_copy_if
            // или copy_if, то получаем сегфолт
            //
            for (const QTextLayout::FormatRange& format : block.textFormats()) {
                if (format.format != block.charFormat()) {
                    formats.push_back(format);
                }
            }

            //
            // Если всего один формат или редакторская заметка на весь текст, то расположим
            // ее после блока на отдельной строке (делается не здесь, а в конце цикла),
            // а иначе просто вставим в блок заметки
            //
            bool fullBlockComment = true;
            if (formats.size() != 1 || formats.front().length != paragraphText.size()) {
                fullBlockComment = false;

                //
                // При вставке форматных строк будем экранировать специальные символы (начиная с
                // конца), чтобы они не сбивали форматирование. Для этого заведем переменную,
                // отслеживающую положение обработанной части
                //
                int escapedRange = paragraphText.size() - 1;

                //
                // Обрабатывать форматирование надо с конца, чтобы не сбилась их позиция вставки
                //
                for (int formatIndex = formats.size() - 1; formatIndex >= 0; --formatIndex) {
                    //
                    // Если это редактораская заметка, обработаем комментарии
                    //
                    if (formats[formatIndex].format.boolProperty(
                            TextBlockStyle::PropertyIsReviewMark)) {
                        //
                        // Извлечем список редакторских заметок для данной области блока
                        //
                        const QStringList comments
                            = formats[formatIndex]
                                  .format.property(TextBlockStyle::PropertyComments)
                                  .toStringList();
                        //
                        // Вставлять редакторские заметки нужно с конца, чтобы не сбилась их
                        // позиция вставки
                        //
                        for (int commentIndex = comments.size() - 1; commentIndex >= 0;
                             --commentIndex) {
                            if (!comments[commentIndex].simplified().isEmpty()) {
                                paragraphText.insert(formats[formatIndex].start
                                                         + formats[formatIndex].length,
                                                     "[[" + comments[commentIndex] + "]]");
                            }
                        }
                    }

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
                        // Получаем индекс для вставки открывающей форматной строки следующего
                        // формата
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
                        paragraphText.insert(
                            nextFormatStartIndex,
                            formatsOpenDiffToString(formats[formatIndex].format,
                                                    formats[nextFormatIndex].format));

                        //
                        // Получаем индекс для вставки закрывающей форматной строки текущего формата
                        //
                        const int formatEndIndex
                            = closeFormatIndex(paragraphText, formats[formatIndex]);

                        //
                        // Экранируем специальные символы до индекса (со стороны конца)
                        //
                        escapeCharacters(paragraphText, formatEndIndex, escapedRange);
                        escapedRange = formatEndIndex - 1;

                        //
                        // Вставляем закрывающую форматную строку текущего формата
                        //
                        paragraphText.insert(
                            formatEndIndex,
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
                        const int formatEndIndex
                            = closeFormatIndex(paragraphText, formats[formatIndex]);

                        //
                        // Экранируем специальные символы до индекса (со стороны конца)
                        //
                        escapeCharacters(paragraphText, formatEndIndex, escapedRange);
                        escapedRange = formatEndIndex - 1;

                        //
                        // Вставляем закрывающую форматную строку текущего формата
                        //
                        paragraphText.insert(formatEndIndex,
                                             closeFormatString(formats, formatIndex));
                    }

                    //
                    // Пишем закрывающий формат, если это самый первый из форматов
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
            }

            //
            // Разрывы строк преобразуем в переносы строк
            //
            paragraphText = paragraphText.replace(QChar::LineSeparator, QChar::LineFeed);

            //
            // Пропустить запись текущего блока
            //
            bool skipBlock = false;

            switch (TextBlockStyle::forBlock(block)) {
            case TextParagraphType::SceneHeading: {
                //
                // Если заголовок сцены начинается с одного из ключевых слов, то все хорошо
                //
                bool startsWithHeading = false;
                for (const QString& heading : sceneHeadingStart) {
                    if (paragraphText.startsWith(heading)) {
                        startsWithHeading = true;
                        break;
                    }
                }

                //
                // Иначе, нужно сказать, что это заголовок сцены добавлением точки в начало
                //
                if (!startsWithHeading) {
                    paragraphText.prepend('.');
                }

                //
                // А если печатаем номера сцен, то добавим в конец этот номер, окруженный #
                //
                if (exportOptions.showScenesNumbers) {
                    const auto blockData = static_cast<TextBlockData*>(block.userData());
                    if (blockData != nullptr && blockData->item()->parent() != nullptr
                        && blockData->item()->parent()->type() == TextModelItemType::Group
                        && static_cast<TextGroupType>(blockData->item()->parent()->subtype())
                            == TextGroupType::Scene) {
                        const auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(
                            blockData->item()->parent());
                        paragraphText += QString(" #%1#").arg(sceneItem->number()->text);
                    }
                }

                if (!isFirst) {
                    paragraphText.prepend('\n');
                }
                break;
            }

            case TextParagraphType::Character: {
                if (paragraphText != TextHelper::smartToUpper(paragraphText)) {
                    //
                    // Если название персонажа не состоит из заглавных букв,
                    // то необходимо добавить @ в начало
                    //
                    paragraphText.prepend('@');
                }
                paragraphText.prepend('\n');
                break;
            }

            case TextParagraphType::Transition: {
                //
                // Если переход задан заглавными буквами и в конце есть TO:
                //
                if (TextHelper::smartToUpper(paragraphText) == paragraphText
                    && paragraphText.endsWith("TO:")) {
                    //
                    // Ничего делать не надо, всё распознается нормально
                    //
                }
                //
                // А если переход задан как то иначе
                //
                else {
                    //
                    // То надо добавить в начало >
                    //
                    paragraphText.prepend("> ");
                }
                paragraphText.prepend('\n');
                break;
            }

            case TextParagraphType::InlineNote: {
                //
                // Обернем в /* и */
                //
                paragraphText = "\n/* " + paragraphText + " */";
                break;
            }

            case TextParagraphType::Action: {
                //
                // Если не первое действие, то отделим его пустой строкой от предыдущего
                //
                if (!isFirst) {
                    paragraphText.prepend('\n');
                }
                break;
            }

            case TextParagraphType::BeatHeading: {
                //
                // Блоки описания сцены предворяются = и расставляются обособлено
                //
                paragraphText.prepend("\n= ");
                break;
            }

            case TextParagraphType::Lyrics: {
                //
                // Добавим ~ вначало блока лирики
                //
                paragraphText.prepend("~ ");
                break;
            }

            case TextParagraphType::ActHeading:
            case TextParagraphType::SequenceHeading: {
                //
                // Напечатаем в начале столько #, насколько глубоко мы в директории
                //
                ++dirNesting;
                paragraphText = " " + paragraphText;
                for (unsigned i = 0; i != dirNesting; ++i) {
                    paragraphText = '#' + paragraphText;
                }
                paragraphText.prepend('\n');
                break;
            }

            case TextParagraphType::ActFooter:
            case TextParagraphType::SequenceFooter: {
                --dirNesting;
                skipBlock = true;
                break;
            }

            case TextParagraphType::Parenthetical: {
                paragraphText = "(" + paragraphText + ")";
                break;
            }

            case TextParagraphType::Dialogue: {
                break;
            }

            default: {
                //
                // Игнорируем неизвестные блоки
                //
                skipBlock = true;
            }
            }

            paragraphText += '\n';

            //
            // А это как раз случай одной большой редакторской заметки или формата
            //
            if (fullBlockComment) {
                //
                // Формат
                //

                //
                // ... определяем начало
                //
                int formatStartIndex = 0;
                const QStringList prefixes = { "\n",     ".",     "@",    "= ",  "~ ", "/*",
                                               "##### ", "#### ", "### ", "## ", "# ", "\n" };
                for (const QString& prefix : prefixes) {
                    if (paragraphText.mid(formatStartIndex, prefix.length()) == prefix) {
                        formatStartIndex += prefix.length();
                    }
                }

                //
                // ... определяем конец
                //
                int formatEndIndex = paragraphText.length();
                const QStringList postfixes = { "\n", "*/", "\n" };
                for (const QString& postfix : postfixes) {
                    if (paragraphText.mid(formatEndIndex - postfix.length(), postfix.length())
                        == postfix) {
                        formatEndIndex -= postfix.length();
                    }
                }

                //
                // ... экранируем специальные символы
                //
                const int escapedCount
                    = escapeCharacters(paragraphText, formatStartIndex, formatEndIndex);

                //
                // ... вставляем форматные строки
                //
                paragraphText.insert(formatEndIndex + escapedCount, closeFormatString(formats, 0));
                paragraphText.insert(formatStartIndex, openFormatString(formats, 0));

                //
                // Заметки
                //
                const QTextCharFormat paragraphFormat = formats.first().format;
                const QStringList comments
                    = paragraphFormat.property(TextBlockStyle::PropertyComments).toStringList();
                for (const QString& comment : comments) {
                    if (!comment.isEmpty()) {
                        paragraphText += "\n[[" + comment + "]]\n";
                    }
                }
            }

            //
            // Запишем получившуюся строку
            //
            if (!skipBlock) {
                isFirst = false;
                fountainFile.write(paragraphText.toUtf8());
            }
        }

        block = block.next();
    }

    fountainFile.close();
}

} // namespace BusinessLayer
