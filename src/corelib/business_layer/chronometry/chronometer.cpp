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
    virtual ~AbstractChronometer() = default;
    virtual std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                               const TextTemplate& _textTemplate) const = 0;
};

/**
 * @brief Расчёт хронометража по количеству страниц
 */
class PageChronometer : public AbstractChronometer
{
public:
    explicit PageChronometer(int _secondsPerPage)
        : m_secondsPerPage(_secondsPerPage)
    {
    }

    std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                       const TextTemplate& _textTemplate) const override
    {
        const auto milliseconds = m_secondsPerPage * 1000;

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

        const auto blockStyle = _textTemplate.paragraphStyle(_type);
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

private:
    /**
     * @brief Секунд на страницу
     */
    const int m_secondsPerPage = 60;
};

/**
 * @brief Расчёт хронометража по количеству символов
 */
class CharactersChronometer : public AbstractChronometer
{
public:
    CharactersChronometer(int _characters, bool _considerSpaces, int _seconds)
        : m_characters(_characters)
        , m_considerSpaces(_considerSpaces)
        , m_seconds(_seconds)
    {
    }

    std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                       const TextTemplate& _textTemplate) const override
    {
        Q_UNUSED(_type)
        Q_UNUSED(_textTemplate)

        auto text = _text;
        if (!m_considerSpaces) {
            text.remove(' ');
        }

        const int milliseconds = m_seconds * 1000;
        const auto characterDuration = static_cast<qreal>(milliseconds) / m_characters;
        return std::chrono::milliseconds{ qCeil(text.length() * characterDuration) };
    }

private:
    /**
     * @brief Сколько символов
     */
    const int m_characters = 1000;

    /**
     * @brief Включая пробелы
     */
    const bool m_considerSpaces = true;

    /**
     * @brief Имеют заданную длительность
     */
    const int m_seconds = 60;
};

/**
 * @brief Расчёт хронометража по количеству слов
 */
class WordsChronometer : public AbstractChronometer
{
public:
    WordsChronometer(int _words, int _seconds)
        : m_words(_words)
        , m_seconds(_seconds)
    {
    }

    std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                       const TextTemplate& _textTemplate) const override
    {
        Q_UNUSED(_type)
        Q_UNUSED(_textTemplate)

        const int milliseconds = m_seconds * 1000;
        const auto characterDuration = static_cast<qreal>(milliseconds) / m_words;
        return std::chrono::milliseconds{ qCeil(TextHelper::wordsCount(_text)
                                                * characterDuration) };
    }

private:
    /**
     * @brief Сколько слов
     */
    const int m_words = 200;

    /**
     * @brief Имеют заданную длительность
     */
    const int m_seconds = 60;
};

/**
 * @brief Расчёт хронометража а-ля Софокл
 */
// class ConfigurableChronometer : public AbstractChronometer
//{
// public:
//    std::chrono::seconds duration(const QTextBlock& _block) const override {
//        const auto blockType = TextBlockStyle::forBlock(_block);
//        if (blockType != TextParagraphType::SceneHeading
//            && blockType != TextParagraphType::Action
//            && blockType != TextParagraphType::Dialogue
//            && blockType != TextParagraphType::Lyrics) {
//            return std::chrono::seconds{0};
//        }

//        //
//        // Длительность зависит от блока
//        //
//        qreal secondsForParagraph = 0;
//        qreal secondsForEvery50 = 0;
//        QString secondsForParagraphKey;
//        QString secondsForEvery50Key;

//        if (blockType == ScenarioBlockStyle::Action) {
//            secondsForParagraphKey = "chronometry/configurable/seconds-for-paragraph/action";
//            secondsForEvery50Key = "chronometry/configurable/seconds-for-every-50/action";
//        } else if (blockType == ScenarioBlockStyle::Dialogue
//                   || blockType == ScenarioBlockStyle::Lyrics) {
//            secondsForParagraphKey = "chronometry/configurable/seconds-for-paragraph/dialog";
//            secondsForEvery50Key = "chronometry/configurable/seconds-for-every-50/dialog";
//        } else {
//            secondsForParagraphKey =
//            "chronometry/configurable/seconds-for-paragraph/scene_heading"; secondsForEvery50Key =
//            "chronometry/configurable/seconds-for-every-50/scene_heading";
//        }

//        //
//        // Получим значения длительности
//        //
//        secondsForParagraph =
//                StorageFacade::settingsStorage()->value(
//                    secondsForParagraphKey,
//                    SettingsStorage::ApplicationSettings)
//                .toDouble();

//        secondsForEvery50 =
//                StorageFacade::settingsStorage()->value(
//                    secondsForEvery50Key,
//                    SettingsStorage::ApplicationSettings)
//                .toDouble();

//        const int every50 = 50;
//        const qreal secondsPerCharacter = secondsForEvery50 / every50;
//        const qreal textChron = secondsForParagraph + _length * secondsPerCharacter;
//        return textChron;
//    }
//};

} // namespace


std::chrono::milliseconds ScreenplayChronometer::duration(TextParagraphType _type,
                                                          const QString& _text,
                                                          const QString& _templateId)
{
    using namespace DataStorageLayer;

    const auto chronometerType = settingsValue(kComponentsScreenplayDurationTypeKey).toInt();
    const auto& screenplayTemplate = TemplatesFacade::screenplayTemplate(_templateId);

    switch (static_cast<ChronometerType>(chronometerType)) {
    case ChronometerType::Page: {
        const auto secondsPerPage
            = settingsValue(kComponentsScreenplayDurationByPageDurationKey).toInt();
        return PageChronometer(secondsPerPage).duration(_type, _text, screenplayTemplate);
    }

    case ChronometerType::Characters: {
        const int characters
            = settingsValue(kComponentsScreenplayDurationByCharactersCharactersKey).toInt();
        const bool considerSpaces
            = settingsValue(kComponentsScreenplayDurationByCharactersIncludeSpacesKey).toBool();
        const int seconds
            = settingsValue(kComponentsScreenplayDurationByCharactersDurationKey).toInt();
        return CharactersChronometer(characters, considerSpaces, seconds)
            .duration(_type, _text, screenplayTemplate);
    }

    default: {
        Q_ASSERT(false);
        return {};
    }
    }
}

std::chrono::milliseconds AudioplayChronometer::duration(TextParagraphType _type,
                                                         const QString& _text,
                                                         const QString& _templateId)
{
    using namespace DataStorageLayer;

    const int words = settingsValue(kComponentsAudioplayDurationByWordsCharactersKey).toInt();
    const int seconds = settingsValue(kComponentsAudioplayDurationByWordsDurationKey).toInt();
    const auto& audioplayTemplate = TemplatesFacade::audioplayTemplate(_templateId);
    return WordsChronometer(words, seconds).duration(_type, _text, audioplayTemplate);
}

} // namespace BusinessLayer
