#include "chronometer.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <QTextBlock>
#include <QtMath>


namespace BusinessLayer
{

namespace {

/**
 * @brief Абстрактный класс вычислителя хронометража
 */
class AbstractChronometer {
public:
    virtual ~AbstractChronometer() = default;
    virtual std::chrono::seconds duration(const QTextBlock& _block) const = 0;
};

/**
 * @brief Расчёт хронометража по количеству символов
 */
class CharactersChronometer : public AbstractChronometer
{
public:
    std::chrono::seconds duration(const QTextBlock& _block) const override {
        const auto blockType = ScreenplayBlockStyle::forBlock(_block);
        if (blockType == ScreenplayParagraphType::InlineNote
            || blockType == ScreenplayParagraphType::FolderHeader
            || blockType == ScreenplayParagraphType::FolderFooter) {
            return std::chrono::seconds{0};
        }

        const int characters = 1374;
        const int seconds = 60;
        const bool considerSpaces = true;

        auto text = _block.text();
        if (!considerSpaces) {
            text.remove(' ');
        }

        const auto characterDuration = static_cast<qreal>(seconds) / characters;
        return std::chrono::seconds{qCeil(text.length() * characterDuration)};
    }
};

}


std::chrono::seconds Chronometer::duration(const QTextBlock& _block)
{
    return CharactersChronometer().duration(_block);
}

} // namespace BusinessLayer
