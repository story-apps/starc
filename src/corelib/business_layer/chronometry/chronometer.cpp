#include "chronometer.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

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
    virtual std::chrono::seconds duration(ScreenplayParagraphType _type, const QString& _text) const = 0;
};

/**
 * @brief Расчёт хронометража по количеству символов
 */
class CharactersChronometer : public AbstractChronometer
{
public:
    std::chrono::seconds duration(ScreenplayParagraphType _type, const QString& _text) const override {
        if (_type == ScreenplayParagraphType::InlineNote
            || _type == ScreenplayParagraphType::FolderHeader
            || _type == ScreenplayParagraphType::FolderFooter) {
            return std::chrono::seconds{0};
        }

        using namespace DataStorageLayer;
        const int characters = StorageFacade::settingsStorage()->value(
                                   kComponentsScreenplayDurationByCharactersCharactersKey,
                                   SettingsStorage::SettingsPlace::Application)
                               .toInt();
        const bool considerSpaces = StorageFacade::settingsStorage()->value(
                                        kComponentsScreenplayDurationByCharactersIncludeSpacesKey,
                                        SettingsStorage::SettingsPlace::Application)
                                    .toBool();
        const int seconds = StorageFacade::settingsStorage()->value(
                                kComponentsScreenplayDurationByCharactersDurationKey,
                                SettingsStorage::SettingsPlace::Application)
                            .toInt();

        auto text = _text;
        if (!considerSpaces) {
            text.remove(' ');
        }

        const auto characterDuration = static_cast<qreal>(seconds) / characters;
        return std::chrono::seconds{qCeil(text.length() * characterDuration)};
    }
};

/**
 * @brief Расчёт хронометража а-ля Софокл
 */
//class ConfigurableChronometer : public AbstractChronometer
//{
//public:
//    std::chrono::seconds duration(const QTextBlock& _block) const override {
//        const auto blockType = ScreenplayBlockStyle::forBlock(_block);
//        if (blockType != ScreenplayParagraphType::SceneHeading
//            && blockType != ScreenplayParagraphType::Action
//            && blockType != ScreenplayParagraphType::Dialogue
//            && blockType != ScreenplayParagraphType::Lyrics) {
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
//            secondsForParagraphKey = "chronometry/configurable/seconds-for-paragraph/scene_heading";
//            secondsForEvery50Key = "chronometry/configurable/seconds-for-every-50/scene_heading";
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

}


std::chrono::seconds Chronometer::duration(ScreenplayParagraphType _type, const QString& _text)
{
    return CharactersChronometer().duration(_type, _text);
}

} // namespace BusinessLayer
