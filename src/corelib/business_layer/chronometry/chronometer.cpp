#include "chronometer.h"

#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QTextBlock>
#include <QtMath>


namespace BusinessLayer {

namespace {

/**
 * @brief Абстрактный класс вычислителя хронометража
 */
class AbstractChronometer
{
public:
    explicit AbstractChronometer(const ChronometerOptions& _options)
        : m_options(_options)
    {
    }
    virtual ~AbstractChronometer() = default;
    virtual std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                               const TextTemplate& _textTemplate) const = 0;

protected:
    const ChronometerOptions& m_options;
};

/**
 * @brief Расчёт хронометража по количеству страниц
 */
class PageChronometer : public AbstractChronometer
{
public:
    explicit PageChronometer(const ChronometerOptions& _options)
        : AbstractChronometer(_options)
    {
    }
    std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                       const TextTemplate& _textTemplate) const override
    {
        const auto milliseconds = m_options.page.seconds * 1000;

        const auto mmPageSize
            = QPageSize(_textTemplate.pageSizeId()).rect(QPageSize::Millimeter).size();
        const bool x = true, y = false;
        const auto pxPageSize = QSizeF(MeasurementHelper::mmToPx(mmPageSize.width(), x),
                                       MeasurementHelper::mmToPx(mmPageSize.height(), y));
        const auto mmPageMargins = _textTemplate.pageMargins();
        const auto pxPageMargins = QMarginsF(MeasurementHelper::mmToPx(mmPageMargins.left(), x),
                                             MeasurementHelper::mmToPx(mmPageMargins.top(), y),
                                             MeasurementHelper::mmToPx(mmPageMargins.right(), x),
                                             MeasurementHelper::mmToPx(mmPageMargins.bottom(), y));
        const auto pageHeight = pxPageSize.height() - pxPageMargins.top() - pxPageMargins.bottom();

        const auto& blockStyle = _textTemplate.paragraphStyle(_type);
        const auto mmBlockMargins = blockStyle.margins();
        const auto pxBlockMargins
            = QMarginsF(MeasurementHelper::mmToPx(mmBlockMargins.left(), x),
                        MeasurementHelper::mmToPx(mmBlockMargins.top(), y),
                        MeasurementHelper::mmToPx(mmBlockMargins.right(), x),
                        MeasurementHelper::mmToPx(mmBlockMargins.bottom(), y));
        const auto textWidth = pxPageSize.width() - pxPageMargins.left() - pxPageMargins.right()
            - pxBlockMargins.left() - pxBlockMargins.right();
        const auto textLineHeight = TextHelper::fineLineSpacing(blockStyle.font());
        const auto textHeight = TextHelper::heightForWidth(_text, blockStyle.font(), textWidth)
            + pxBlockMargins.top() + pxBlockMargins.bottom()
            + blockStyle.linesBefore() * textLineHeight + blockStyle.linesAfter() * textLineHeight;

        //
        // Добавляем небольшую дельту, т.к. из-за приблизительности рассчётов не удаётся попадать
        // точно в минуты
        //
        return std::chrono::milliseconds{ qCeil(textHeight / pageHeight * milliseconds * 1.01) };
    }
};

/**
 * @brief Расчёт хронометража по количеству символов
 */
class CharactersChronometer : public AbstractChronometer
{
public:
    explicit CharactersChronometer(const ChronometerOptions& _options)
        : AbstractChronometer(_options)
    {
    }
    std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                       const TextTemplate& _textTemplate) const override
    {
        Q_UNUSED(_type)
        Q_UNUSED(_textTemplate)

        auto text = _text;
        if (!m_options.characters.considerSpaces) {
            text.remove(' ');
        }

        const int milliseconds = m_options.characters.seconds * 1000;
        const auto characterDuration
            = static_cast<qreal>(milliseconds) / m_options.characters.characters;
        return std::chrono::milliseconds{ qCeil(text.length() * characterDuration) };
    }
};

/**
 * @brief Расчёт хронометража по количеству слов
 */
class WordsChronometer : public AbstractChronometer
{
public:
    explicit WordsChronometer(const ChronometerOptions& _options)
        : AbstractChronometer(_options)
    {
    }
    std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                       const TextTemplate& _textTemplate) const override
    {
        Q_UNUSED(_type)
        Q_UNUSED(_textTemplate)

        const int milliseconds = m_options.words.seconds * 1000;
        const auto characterDuration = static_cast<qreal>(milliseconds) / m_options.words.words;
        return std::chrono::milliseconds{ qCeil(TextHelper::wordsCount(_text)
                                                * characterDuration) };
    }
};

/**
 * @brief Расчёт хронометража а-ля Софокл
 */
class SophoclesChronometer : public AbstractChronometer
{
public:
    explicit SophoclesChronometer(const ChronometerOptions& _options)
        : AbstractChronometer(_options)
    {
    }
    std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                       const TextTemplate& _textTemplate) const override
    {
        Q_UNUSED(_textTemplate)

        const auto blockType = _type;
        if (blockType != TextParagraphType::SceneHeading && blockType != TextParagraphType::Action
            && blockType != TextParagraphType::Dialogue && blockType != TextParagraphType::Lyrics) {
            return std::chrono::seconds{ 0 };
        }

        //
        // Длительность зависит от блока
        //
        qreal secondsForParagraph = 0.0;
        qreal secondsForEvery50 = 0.0;

        if (blockType == TextParagraphType::Action) {
            secondsForParagraph = m_options.sophocles.secsPerAction;
            secondsForEvery50 = m_options.sophocles.secsPerEvery50Action;
        } else if (blockType == TextParagraphType::Dialogue
                   || blockType == TextParagraphType::Lyrics) {
            secondsForParagraph = m_options.sophocles.secsPerDialogue;
            secondsForEvery50 = m_options.sophocles.secsPerEvery50Dialogue;
        } else {
            secondsForParagraph = m_options.sophocles.secsPerSceneHeading;
            secondsForEvery50 = m_options.sophocles.secsPerEvery50SceneHeading;
        }

        const qreal every50 = 50.0;
        const qreal secondsPerCharacter = secondsForEvery50 / every50;
        const qreal textDuration = secondsForParagraph + _text.length() * secondsPerCharacter;
        return std::chrono::milliseconds{ qCeil(textDuration * 1000) };
    }
};

} // namespace


std::chrono::milliseconds ScreenplayChronometer::duration(TextParagraphType _type,
                                                          const QString& _text,
                                                          const QString& _templateId,
                                                          const ChronometerOptions& _options)
{
    const auto& screenplayTemplate = TemplatesFacade::screenplayTemplate(_templateId);

    switch (_options.type) {
    case ChronometerType::Page: {
        return PageChronometer(_options).duration(_type, _text, screenplayTemplate);
    }

    case ChronometerType::Characters: {
        return CharactersChronometer(_options).duration(_type, _text, screenplayTemplate);
    }

    case ChronometerType::Sophocles: {
        return SophoclesChronometer(_options).duration(_type, _text, screenplayTemplate);
    }

    default: {
        Q_ASSERT(false);
        return {};
    }
    }
}

std::chrono::milliseconds AudioplayChronometer::duration(TextParagraphType _type,
                                                         const QString& _text,
                                                         const QString& _templateId,
                                                         const ChronometerOptions& _options)
{
    const auto& audioplayTemplate = TemplatesFacade::audioplayTemplate(_templateId);
    return WordsChronometer(_options).duration(_type, _text, audioplayTemplate);
}

} // namespace BusinessLayer
