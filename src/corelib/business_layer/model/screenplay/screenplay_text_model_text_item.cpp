#include "screenplay_text_model_text_item.h"

#include <business_layer/templates/screenplay_template.h>

#include <QColor>


namespace BusinessLayer
{

namespace {
    struct TextPart {
        int from = 0;
        int length = 0;
    };
}

class ScreenplayTextModelTextItem::Implementation
{
public:
    /**
     * @brief Тип параграфа
     */
    ScreenplayParagraphType type = ScreenplayParagraphType::UnformattedText;

    /**
     * @brief Закладка в параграфе
     */
    struct Bookmark {
        QColor color;
        QString text;
    };
    std::optional<Bookmark> bookmark;

    /**
     * @brief Текст блока
     */
    QString text;

    /**
     * @brief Редакторские заметки в параграфе
     */
    struct ReviewComment {
        QString author;
        QString date;
        QString text;
    };
    struct ReviewMark : TextPart {
        QColor textColor;
        QColor backgroundColor;
        bool isHighlight = false;
        bool isDone = false;
        QVector<ReviewComment> comments;
    };
    QVector<ReviewMark> reviewMarks;

    /**
     * @brief Форматирование текста в параграфе
     */
    struct TextFormat : TextPart {
        bool isBold = false;
        bool isItalic = false;
        bool isUnderline = false;
        Qt::Alignment alignment = Qt::AlignLeft;
    };
    QVector<TextFormat> formats;

    /**
     * @brief Ревизии в блоке
     */
    struct Revision : TextPart {
        QColor color;
    };
    QVector<Revision> revisions;
};

ScreenplayTextModelTextItem::ScreenplayTextModelTextItem()
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Text),
      d(new Implementation)
{
}

ScreenplayTextModelTextItem::~ScreenplayTextModelTextItem() = default;

} // namespace BusinessLayer
