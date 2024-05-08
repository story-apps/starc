#include "comic_book_fountain_exporter.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>

#include <QRegularExpression>


namespace BusinessLayer {

ComicBookFountainExporter::ComicBookFountainExporter()
    : ComicBookExporter()
    , AbstractMarkdownExporter({ '*', '_' })
{
}

ComicBookFountainExporter::~ComicBookFountainExporter() = default;

bool ComicBookFountainExporter::processBlock(QString& _paragraph, const QTextBlock& _block,
                                             const ExportOptions& _exportOptions) const
{

    //
    // Если нужны редакторские заметки, то вставляем их
    //
    if (_exportOptions.includeReviewMarks) {
        QVector<QTextLayout::FormatRange> reviewMarks;
        for (const QTextLayout::FormatRange& format : _block.textFormats()) {
            if (format.format.boolProperty(TextBlockStyle::PropertyIsReviewMark)) {
                reviewMarks.push_back(format);
            }
        }
        for (int markIndex = reviewMarks.size() - 1; markIndex >= 0; --markIndex) {
            //
            // Извлечем список редакторских заметок для данной области блока
            //
            const QStringList comments = reviewMarks[markIndex]
                                             .format.property(TextBlockStyle::PropertyComments)
                                             .toStringList();
            //
            // Вставлять редакторские заметки нужно с конца, чтобы не сбилась их
            // позиция вставки
            //
            for (int commentIndex = comments.size() - 1; commentIndex >= 0; --commentIndex) {
                if (!comments[commentIndex].simplified().isEmpty()) {
                    _paragraph.insert(reviewMarks[markIndex].start + reviewMarks[markIndex].length,
                                      "[[" + comments[commentIndex] + "]]");
                }
            }
        }
    }

    //
    // Добавить форматные символы перед заголовком
    //
    auto formatToHeading = [&_paragraph](int _level) {
        const QString prefix = "#";
        _paragraph.prepend(prefix.repeated(_level) + " ");
    };

    const auto type = TextBlockStyle::forBlock(_block);
    switch (type) {
    case TextParagraphType::PageHeading: {
        formatToHeading(1);
        return true;
    }

    case TextParagraphType::PanelHeading: {
        _paragraph.prepend('.');
        return true;
    }

    case TextParagraphType::Description: {
        //
        // Корректируем регистр, чтобы не было верхнего регистра
        //
        const auto capitalizeEveryWord = false;
        const auto capitalizeEverySentense = true;
        _paragraph
            = TextHelper::toSentenceCase(_paragraph, capitalizeEveryWord, capitalizeEverySentense);
        return true;
    }

    case TextParagraphType::Character: {
        //
        // Очистим имя персонажа от номера
        //
        const QRegularExpression number("(\\d+. )");
        QRegularExpressionMatch match = number.match(_paragraph);
        if (match.hasMatch()) {
            _paragraph.remove(0, match.capturedLength());
        }

        //
        // Если название персонажа не состоит из заглавных букв,
        // то необходимо добавить @ в начало
        //
        if (!TextHelper::isUppercase(_paragraph)) {
            _paragraph.prepend('@');
        }

        //
        // Если в конце имени персонажа стоит двоеточие, удалим его
        //
        if (_paragraph.endsWith(':')) {
            _paragraph.chop(1);
        }

        return true;
    }

    case TextParagraphType::Dialogue: {
        return true;
    }

    case TextParagraphType::InlineNote: {
        //
        // Обернем в /* и */
        //
        _paragraph = "/* " + _paragraph + " */";
        return true;
    }
    case TextParagraphType::UnformattedText: {
        return true;
    }
    default: {
        //
        // Игнорируем неизвестные блоки
        //
        return false;
    }
    }
}

QString ComicBookFountainExporter::formatSymbols(TextSelectionTypes _type) const
{
    switch (_type) {
    case TextSelectionTypes::Bold: {
        return "**";
    }
    case TextSelectionTypes::Italic: {
        return "*";
    }
    case TextSelectionTypes::Underline: {
        return "_";
    }
    default: {
        return "";
    }
    }
}

void ComicBookFountainExporter::addIndentationAtBegin(QString& _paragraph,
                                                      TextParagraphType _previosBlockType,
                                                      TextParagraphType _currentBlockType) const
{
    //
    // Таблица количества переносов строк между различными типами блоков
    // столбцы - предыдущий тип, строки - текущий
    //
    const QVector<QVector<int>> countIndentation = {
        // Undefined, PageHeading, PanelHeading, Character, Dialogue,
        // Description/UnformattedText/Synopses
        { 0, 0, 0, 0, 0, 0 }, // Undefined
        { 0, 2, 3, 3, 3, 3 }, // PageHeading
        { 0, 2, 2, 3, 3, 3 }, // PanelHeading
        { 0, 2, 2, 2, 2, 2 }, // Character
        { 0, 2, 2, 1, 2, 2 }, // Dialogue
        { 0, 2, 2, 2, 2, 2 }, // Description/UnformattedText/InlineNote
    };

    auto positionInTable = [](TextParagraphType _type) {
        switch (_type) {
        case TextParagraphType::PageHeading: {
            return 1;
        }
        case TextParagraphType::PanelHeading: {
            return 2;
        }
        case TextParagraphType::Character: {
            return 3;
        }
        case TextParagraphType::Dialogue: {
            return 4;
        }
        case TextParagraphType::Description:
        case TextParagraphType::UnformattedText:
        case TextParagraphType::InlineNote: {
            return 5;
        }
        default: {
            return 0;
        }
        }
    };
    int column = positionInTable(_previosBlockType);
    int row = positionInTable(_currentBlockType);
    _paragraph.prepend(QString("\n").repeated(countIndentation[row][column]));
}

} // namespace BusinessLayer
