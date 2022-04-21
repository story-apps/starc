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
 * @brief Преобразовать заданный формат в строку
 */
static QString formatToString(const QTextCharFormat& _format)
{
    QString formatString;
    if (_format.font().bold()) {
        formatString += "**";
    }
    if (_format.font().italic()) {
        formatString += "*";
    }
    if (_format.font().underline()) {
        formatString += "_";
    }
    return formatString;
}

/**
 * @brief Преобразовать разницу в двух форматах в строку
 */
static QString formatsDiffToString(const QTextCharFormat& _current, const QTextCharFormat& _next)
{
    QTextCharFormat diff;
    diff.setFontWeight(_current.font().bold() ^ _next.font().bold() ? QFont::Bold : QFont::Normal);
    diff.setFontItalic(_current.font().italic() ^ _next.font().italic());
    diff.setFontUnderline(_current.font().underline() ^ _next.font().underline());
    return formatToString(diff);
}

} // namespace

void ScreenplayFountainExporter::exportTo(ScreenplayTextModel* _model,
                                          const ScreenplayExportOptions& _exportOptions) const
{
    //
    // Открываем документ на запись
    //
    QFile fountainFile(_exportOptions.filePath);
    if (!fountainFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QScopedPointer<ScreenplayTextDocument> document(prepareDocument(_model, _exportOptions));

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
                    // Смотрим на последующее форматирование
                    //
                    const int nextFormatIndex = formatIndex + 1;
                    //
                    // ... если было, то сравниваем форматы и пишем только необходимое (то,
                    // которое не дублируется с последующим)
                    //
                    if (nextFormatIndex < formats.size()
                        && (formats[formatIndex].start + formats[formatIndex].length
                            == formats[nextFormatIndex].start)) {
                        paragraphText.insert(formats[formatIndex].start
                                                 + formats[formatIndex].length,
                                             formatsDiffToString(formats[formatIndex].format,
                                                                 formats[nextFormatIndex].format));
                    }
                    //
                    // ... если его не было, то формируем полностью
                    //
                    else {
                        paragraphText.insert(formats[formatIndex].start
                                                 + formats[formatIndex].length,
                                             formatToString(formats[formatIndex].format));
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
                    if (prevFormatIndex > 0
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
                        paragraphText.insert(formats[formatIndex].start,
                                             formatToString(formats[formatIndex].format));
                    }
                }
            }

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
                if (_exportOptions.showScenesNumbers) {
                    const auto blockData = static_cast<TextBlockData*>(block.userData());
                    if (blockData != nullptr) {
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
                paragraphText = "\n/*\n" + paragraphText + "\n*/";
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

                //
                // TODO: добавить экспорт описания сцен
                //
                //            case TextParagraphType::SceneDescription: {
                //                //
                //                // Блоки описания сцены предворяются = и расставляются обособлено
                //                //
                //                paragraphText.prepend("\n= ");
                //                break;
                //            }

            case TextParagraphType::Lyrics: {
                //
                // Добавим ~ вначало блока лирики
                //
                paragraphText.prepend("~ ");
                break;
            }

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

            case TextParagraphType::SequenceFooter: {
                --dirNesting;
                skipBlock = true;
                break;
            }

            case TextParagraphType::Dialogue:
            case TextParagraphType::Parenthetical:
                break;
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
                const QTextCharFormat paragraphFormat = formats.first().format;
                const QString paragraphFormatText = formatToString(paragraphFormat);
                //
                // ... начало
                //
                int formatStartIndex = 0;
                const QStringList prefixes = { "\n",     ".",     "@",    "= ",  "~ ", "/*",
                                               "##### ", "#### ", "### ", "## ", "# ", "\n" };
                for (const QString& prefix : prefixes) {
                    if (paragraphText.mid(formatStartIndex, prefix.length()) == prefix) {
                        formatStartIndex += prefix.length();
                    }
                }
                paragraphText.insert(formatStartIndex, paragraphFormatText);
                //
                // ... конец
                //
                int formatEndIndex = paragraphText.length();
                const QStringList postfixes = { "\n", "*/", "\n" };
                for (const QString& postfix : postfixes) {
                    if (paragraphText.mid(formatEndIndex - postfix.length(), postfix.length())
                        == postfix) {
                        formatEndIndex -= postfix.length();
                    }
                }
                paragraphText.insert(formatEndIndex, paragraphFormatText);
                //
                // Заметки
                //
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
