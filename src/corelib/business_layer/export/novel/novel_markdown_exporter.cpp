#include "novel_markdown_exporter.h"

#include <business_layer/export/novel/novel_export_options.h>
#include <business_layer/templates/novel_template.h>

namespace BusinessLayer {

NovelMarkdownExporter::NovelMarkdownExporter()
    : NovelExporter()
    , AbstractMarkdownExporter(
          { '\\', '`', '*', '_', '{', '}', '[', ']', '(', ')', '#', '+', '-', '.', '!', '|', '~' })
{
}

NovelMarkdownExporter::~NovelMarkdownExporter() = default;

bool NovelMarkdownExporter::processBlock(QString& _paragraph, const QTextBlock& _block,
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
    case TextParagraphType::PartHeading:
    case TextParagraphType::PartFooter: {
        formatToHeading(1);
        return true;
    }
    case TextParagraphType::ChapterHeading:
    case TextParagraphType::ChapterFooter: {
        formatToHeading(3);
        return true;
    }
    case TextParagraphType::SceneHeading: {
        formatToHeading(5);
        return true;
    }
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
    case TextParagraphType::BeatHeading:
    case TextParagraphType::UnformattedText:
    case TextParagraphType::Text: {
        return true;
    }
    default: {
        return false;
    }
    }
}

QString NovelMarkdownExporter::formatSymbols(TextSelectionTypes _type) const
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

void NovelMarkdownExporter::addIndentationAtBegin(QString& _paragraph,
                                                  TextParagraphType _previosBlockType,
                                                  TextParagraphType _currentBlockType) const
{
    //
    // Таблица количества переносов строк между различными типами блоков
    // столбцы - предыдущий тип, строки - текущий
    //
    const QVector<QVector<int>> countIndentation = {
        // Undefined, PartHeading/ChapterHeadings, SceneHeading, Footers, Text/BeatHeading
        { 0, 0, 0, 0, 0 }, // Undefined
        { 0, 2, 2, 3, 3 }, // PartHeading/ChapterHeadings
        { 0, 2, 2, 3, 2 }, // SceneHeading
        { 0, 2, 2, 2, 2 }, // Footers
        { 0, 2, 2, 3, 2 }, // Text/BeatHeading
    };

    auto positionInTable = [](TextParagraphType _type) {
        switch (_type) {
        case TextParagraphType::PartHeading:
        case TextParagraphType::ChapterHeading:
        case TextParagraphType::ChapterHeading1:
        case TextParagraphType::ChapterHeading2:
        case TextParagraphType::ChapterHeading3:
        case TextParagraphType::ChapterHeading4:
        case TextParagraphType::ChapterHeading5:
        case TextParagraphType::ChapterHeading6: {
            return 1;
        }
        case TextParagraphType::SceneHeading: {
            return 2;
        }
        case TextParagraphType::PartFooter:
        case TextParagraphType::ChapterFooter: {
            return 3;
        }
        case TextParagraphType::UnformattedText:
        case TextParagraphType::Text:
        case TextParagraphType::BeatHeading: {
            return 4;
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
