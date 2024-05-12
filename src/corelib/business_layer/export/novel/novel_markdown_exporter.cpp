#include "novel_markdown_exporter.h"

#include <business_layer/export/novel/novel_export_options.h>
#include <business_layer/templates/novel_template.h>

namespace BusinessLayer {

class NovelMarkdownExporter::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief  Увеличить показатель вложенности глав
     */
    void increaseNesting();

    /**
     * @brief  Уменььшить показатель вложенности глав
     */
    void decreaseNesting();


    /**
     * @brief Глубина вложенности глав
     */
    mutable unsigned m_chapterNesting = 1;
};

NovelMarkdownExporter::Implementation::Implementation() = default;

void NovelMarkdownExporter::Implementation::increaseNesting()
{
    if (m_chapterNesting > 5) {
        m_chapterNesting = 5;
        return;
    } else {
        ++m_chapterNesting;
    }
}

void NovelMarkdownExporter::Implementation::decreaseNesting()
{
    if (m_chapterNesting < 2) {
        m_chapterNesting = 2;
        return;
    } else {
        --m_chapterNesting;
    }
}


// ****


NovelMarkdownExporter::NovelMarkdownExporter()
    : NovelExporter()
    , AbstractMarkdownExporter({ '\\', '`', '*', '_', '#', '~', '-', '(', ')' })
    , d(new Implementation())
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
    case TextParagraphType::ChapterHeading: {
        d->increaseNesting();
        formatToHeading(d->m_chapterNesting);
        return true;
    }
    case TextParagraphType::ChapterFooter: {
        formatToHeading(d->m_chapterNesting);
        d->decreaseNesting();
        return true;
    }
    case TextParagraphType::SceneHeading: {
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
        case TextParagraphType::ChapterHeading: {
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
