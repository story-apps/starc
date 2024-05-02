#include "simple_text_markdown_exporter.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/simple_text_template.h>

#include <QFile>


namespace BusinessLayer {

SimpleTextMarkdownExporter::SimpleTextMarkdownExporter()
    : SimpleTextExporter()
    , AbstractMarkdownExporter(
          { '\\', '`', '*', '_', '{', '}', '[', ']', '(', ')', '#', '+', '-', '.', '!', '|', '~' })
{
}

SimpleTextMarkdownExporter::~SimpleTextMarkdownExporter() = default;

bool SimpleTextMarkdownExporter::processBlock(QString& _paragraph, const QTextBlock& _block,
                                              const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)

    //
    // Очистим пробельные символы в начале абзаца, чтобы не сбивалось форматирование
    //
    removeWhitespaceAtBegin(_paragraph);

    //
    // Добавить форматные символы перед заголовком
    //
    auto formatToHeading = [&_paragraph](int _level) {
        const QString prefix = "#";
        _paragraph.prepend(prefix.repeated(_level) + " ");
    };

    switch (TextBlockStyle::forBlock(_block)) {
    case TextParagraphType::ChapterHeading1: {
        formatToHeading(1);
        return true;
    }
    case TextParagraphType::ChapterHeading2: {
        formatToHeading(2);
        return true;
    }
    case TextParagraphType::ChapterHeading3: {
        formatToHeading(3);
        return true;
    }
    case TextParagraphType::ChapterHeading4: {
        formatToHeading(4);
        return true;
    }
    case TextParagraphType::ChapterHeading5: {
        formatToHeading(5);
        return true;
    }
    case TextParagraphType::ChapterHeading6: {
        formatToHeading(6);
        return true;
    }
    case TextParagraphType::Text: {
        return true;
    }
    default: {
        return false;
    }
    }
}

QString SimpleTextMarkdownExporter::formatSymbols(TextSelectionTypes _type) const
{
    switch (_type) {
    case TextSelectionTypes::Bold: {
        return "**";
    }
    case TextSelectionTypes::Italic: {
        return "_";
    }
    case TextSelectionTypes::StrikeOut: {
        return "~~";
    }
    default: {
        return "";
    }
    }
}

void SimpleTextMarkdownExporter::addIndentationAtBegin(QString& _paragraph,
                                                       TextParagraphType _previosBlockType,
                                                       TextParagraphType _currentBlockType) const
{
    //
    // Таблица количества переносов строк между различными типами блоков
    // столбцы - предыдущий тип, строки - текущий
    //
    const QVector<QVector<int>> countIndentation = {
        // Undefined, ChapterHeadings, SceneHeading, Text
        { 0, 0, 0 }, // Undefined
        { 0, 2, 3 }, // ChapterHeadings
        { 0, 2, 2 }, // Text
    };

    auto positionInTable = [](TextParagraphType _type) {
        switch (_type) {
        case TextParagraphType::ChapterHeading:
        case TextParagraphType::ChapterHeading1:
        case TextParagraphType::ChapterHeading2:
        case TextParagraphType::ChapterHeading3:
        case TextParagraphType::ChapterHeading4:
        case TextParagraphType::ChapterHeading5:
        case TextParagraphType::ChapterHeading6: {
            return 1;
        }
        case TextParagraphType::Text: {
            return 2;
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
