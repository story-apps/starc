#include "screenplay_fountain_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>


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
} // namespace


class ScreenplayFountainExporter::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Глубина вложенности дирректории
     */
    mutable unsigned m_dirNesting = 0;
};

ScreenplayFountainExporter::Implementation::Implementation() = default;


// ****


ScreenplayFountainExporter::ScreenplayFountainExporter()
    : ScreenplayExporter()
    , AbstractMarkdownExporter({ '*', '_' })
    , d(new Implementation())
{
}

ScreenplayFountainExporter::~ScreenplayFountainExporter() = default;

bool ScreenplayFountainExporter::processBlock(QString& _paragraph, const QTextBlock& _block,
                                              const ExportOptions& _exportOptions) const
{
    const auto& exportOptions = static_cast<const ScreenplayExportOptions&>(_exportOptions);

    //
    // Если нужны редакторские заметки, то вставляем их
    //
    if (exportOptions.includeReviewMarks) {
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

    switch (TextBlockStyle::forBlock(_block)) {
    case TextParagraphType::SceneHeading: {
        //
        // Если заголовок сцены начинается с одного из ключевых слов, то все хорошо
        //
        bool startsWithHeading = false;
        for (const QString& heading : sceneHeadingStart) {
            if (_paragraph.startsWith(heading)) {
                startsWithHeading = true;
                return true;
            }
        }

        //
        // Иначе, нужно сказать, что это заголовок сцены добавлением точки в начало
        //
        if (!startsWithHeading) {
            _paragraph.prepend('.');
        }

        //
        // А если печатаем номера сцен, то добавим в конец этот номер, окруженный #
        //
        if (exportOptions.showScenesNumbers) {
            const auto blockData = static_cast<TextBlockData*>(_block.userData());
            if (blockData != nullptr && blockData->item()->parent() != nullptr
                && blockData->item()->parent()->type() == TextModelItemType::Group
                && static_cast<TextGroupType>(blockData->item()->parent()->subtype())
                    == TextGroupType::Scene) {
                const auto sceneItem
                    = static_cast<ScreenplayTextModelSceneItem*>(blockData->item()->parent());
                _paragraph += QString(" #%1#").arg(sceneItem->number()->text);
            }
        }
        return true;
    }

    case TextParagraphType::Character: {
        if (!TextHelper::isUppercase(_paragraph)) {
            //
            // Если название персонажа не состоит из заглавных букв,
            // то необходимо добавить @ в начало
            //
            _paragraph.prepend('@');
        }

        //
        // Если двойной диалог, то к имени второго персонажа допишем "^"
        //
        const auto blockData = static_cast<TextBlockData*>(_block.userData());
        if (blockData != nullptr) {
            const auto textItem = static_cast<TextModelTextItem*>(blockData->item());
            if (textItem->isInFirstColumn().has_value() && textItem->isInFirstColumn() == false) {
                _paragraph.append(" ^");
            }
        }
        return true;
    }

    case TextParagraphType::Transition: {
        //
        // Если переход задан заглавными буквами и в конце есть TO:
        //
        if (TextHelper::isUppercase(_paragraph) && _paragraph.endsWith("TO:")) {
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
            _paragraph.prepend("> ");
        }
        return true;
    }

    case TextParagraphType::InlineNote: {
        //
        // Обернем в /* и */
        //
        _paragraph = "/* " + _paragraph + " */";
        return true;
    }

    case TextParagraphType::Action: {
        //
        // Если действие в верхнем регистре, поставим перед ним "!"
        //
        if (TextHelper::isUppercase(_paragraph)) {
            _paragraph.prepend("!");
        }
        return true;
    }

    case TextParagraphType::BeatHeading: {
        //
        // Блоки описания сцены предворяются = и расставляются обособлено
        //
        _paragraph.prepend("= ");
        return true;
    }

    case TextParagraphType::Lyrics: {
        //
        // Добавим ~ вначало блока лирики
        //
        _paragraph.prepend("~ ");
        return true;
    }

    case TextParagraphType::ActHeading:
    case TextParagraphType::SequenceHeading: {
        //
        // Напечатаем в начале столько #, насколько глубоко мы в директории
        //
        ++d->m_dirNesting;
        formatToHeading(d->m_dirNesting);
        return true;
    }

    case TextParagraphType::ActFooter:
    case TextParagraphType::SequenceFooter: {
        --d->m_dirNesting;
        return false;
    }

    case TextParagraphType::Parenthetical: {
        _paragraph = "(" + _paragraph + ")";
        return true;
    }

    case TextParagraphType::Dialogue: {
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

QString ScreenplayFountainExporter::formatSymbols(TextSelectionTypes _type) const
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

void ScreenplayFountainExporter::addIndentationAtBegin(QString& _paragraph,
                                                       TextParagraphType _previosBlockType,
                                                       TextParagraphType _currentBlockType) const
{
    //
    // Таблица количества переносов строк между различными типами блоков
    // столбцы - предыдущий тип, строки - текущий
    //
    const QVector<QVector<int>> countIndentation = {
        // Undefined, SceneCharacters, Character, Parenthetical, Dialogue/Lyrics, Footers,
        // ActHeading/SequenceHeading/SceneHeading, other
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // Undefined
        { 0, 1, 2, 2, 2, 3, 2, 2 }, // SceneCharacters
        { 0, 2, 2, 2, 2, 3, 2, 2 }, // Character
        { 0, 2, 1, 1, 1, 3, 2, 2 }, // Parenthetical
        { 0, 2, 1, 1, 1, 3, 2, 2 }, // Dialogue/Lyrics
        { 0, 2, 2, 2, 2, 2, 2, 2 }, // Footers
        { 0, 3, 3, 3, 3, 3, 2, 3 }, // ActHeading/SequenceHeading/SceneHeading
        { 0, 2, 2, 2, 2, 3, 2, 2 }, // other
        // other:
        // Action/Shot/Transition/BeatHeading/UnformattedText/InlineNote
    };

    auto positionInTable = [](TextParagraphType _type) {
        switch (_type) {
        case TextParagraphType::SceneCharacters: {
            return 1;
        }
        case TextParagraphType::Character: {
            return 2;
        }
        case TextParagraphType::Parenthetical: {
            return 3;
        }
        case TextParagraphType::Dialogue:
        case TextParagraphType::Lyrics: {
            return 4;
        }
        case TextParagraphType::ActFooter:
        case TextParagraphType::SequenceFooter: {
            return 5;
        }
        case TextParagraphType::ActHeading:
        case TextParagraphType::SequenceHeading:
        case TextParagraphType::SceneHeading: {
            return 6;
        }
        case TextParagraphType::Action:
        case TextParagraphType::Shot:
        case TextParagraphType::Transition:
        case TextParagraphType::BeatHeading:
        case TextParagraphType::UnformattedText:
        case TextParagraphType::InlineNote: {
            return 7;
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
