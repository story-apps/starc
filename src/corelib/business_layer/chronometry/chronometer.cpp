#include "chronometer.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/widgets/text_edit/page/page_metrics.h>

#include <utils/helpers/text_helper.h>

#include <QTextBlock>
#include <QtMath>


namespace BusinessLayer
{

namespace {

/**
 * @brief Получить список неэкспортируемых типов
 */
QSet<ScreenplayParagraphType> unexportableTypes() {
    QSet<ScreenplayParagraphType> types = { ScreenplayParagraphType::InlineNote };
    //
    // Папки могут быть настроены таким образом, чтобы не экспортироваться с остальным текстом
    //
    const auto currentTemplate = BusinessLayer::ScreenplayTemplateFacade::getTemplate();
    for (auto type : { ScreenplayParagraphType::FolderHeader, ScreenplayParagraphType::FolderFooter }) {
        if (currentTemplate.blockStyle(type).isExportable()) {
            types.insert(type);
        }
    }
    return types;
}

/**
 * @brief Абстрактный класс вычислителя хронометража
 */
class AbstractChronometer {
public:
    virtual ~AbstractChronometer() = default;
    virtual std::chrono::milliseconds duration(ScreenplayParagraphType _type, const QString& _text) const = 0;
};

/**
 * @brief Расчёт хронометража по количеству страниц
 */
class PageChronometer : public AbstractChronometer
{
public:
    std::chrono::milliseconds duration(ScreenplayParagraphType _type, const QString& _text) const override {
        if (unexportableTypes().contains(_type)) {
            return std::chrono::seconds{0};
        }

        using namespace DataStorageLayer;
        const auto milliseconds = StorageFacade::settingsStorage()->value(
                                      kComponentsScreenplayDurationByPageDurationKey,
                                      SettingsStorage::SettingsPlace::Application)
                                  .toInt() * 1000;

        const auto currentTemplate = BusinessLayer::ScreenplayTemplateFacade::getTemplate();
        const auto mmPageSize = QPageSize(currentTemplate.pageSizeId()).rect(QPageSize::Millimeter).size();
        const bool x = true, y = false;
        const auto pxPageSize = QSizeF(PageMetrics::mmToPx(mmPageSize.width(), x),
                                       PageMetrics::mmToPx(mmPageSize.height(), y));
        const auto mmPageMargins = currentTemplate.pageMargins();
        const auto pxPageMargins = QMarginsF(PageMetrics::mmToPx(mmPageMargins.left(), x),
                                             PageMetrics::mmToPx(mmPageMargins.top(), y),
                                             PageMetrics::mmToPx(mmPageMargins.right(), x),
                                             PageMetrics::mmToPx(mmPageMargins.bottom(), y));
        const auto pageHeight = pxPageSize.height()
                                - pxPageMargins.top() - pxPageMargins.bottom();

        const auto blockStyle = currentTemplate.blockStyle(_type);
        const auto mmBlockMargins = blockStyle.margins();
        const auto pxBlockMargins = QMarginsF(PageMetrics::mmToPx(mmBlockMargins.left(), x),
                                              PageMetrics::mmToPx(mmBlockMargins.top(), y),
                                              PageMetrics::mmToPx(mmBlockMargins.right(), x),
                                              PageMetrics::mmToPx(mmBlockMargins.bottom(), y));
        const auto textWidth = pxPageSize.width()
                               - pxPageMargins.left() - pxPageMargins.right()
                               - pxBlockMargins.left() - pxBlockMargins.right();
        const auto textLineHeight = TextHelper::fineLineSpacing(blockStyle.font());
        const auto textHeight = TextHelper::heightForWidth(_text, blockStyle.font(), textWidth)
                                + pxBlockMargins.top() + pxBlockMargins.bottom()
                                + blockStyle.linesBefore() * textLineHeight + blockStyle.linesAfter() * textLineHeight;

        //
        // Добавляем небольшую дельту, т.к. из-за приблизительности рассчётов не удаётся попадать точно в минуты
        //
        return std::chrono::milliseconds{qCeil(textHeight / pageHeight * milliseconds * 1.01)};
    }
};

/**
 * @brief Расчёт хронометража по количеству символов
 */
class CharactersChronometer : public AbstractChronometer
{
public:
    std::chrono::milliseconds duration(ScreenplayParagraphType _type, const QString& _text) const override {
        if (unexportableTypes().contains(_type)) {
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
        const int milliseconds = StorageFacade::settingsStorage()->value(
                                     kComponentsScreenplayDurationByCharactersDurationKey,
                                     SettingsStorage::SettingsPlace::Application)
                                 .toInt() * 1000;

        auto text = _text;
        if (!considerSpaces) {
            text.remove(' ');
        }

        const auto characterDuration = static_cast<qreal>(milliseconds) / characters;
        return std::chrono::milliseconds{qCeil(text.length() * characterDuration)};
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


std::chrono::milliseconds Chronometer::duration(ScreenplayParagraphType _type, const QString& _text)
{
    const auto chronometerType = DataStorageLayer::StorageFacade::settingsStorage()->value(
                                     DataStorageLayer::kComponentsScreenplayDurationTypeKey,
                                     DataStorageLayer::SettingsStorage::SettingsPlace::Application).toInt();
    switch (static_cast<ChronometerType>(chronometerType)) {
        case ChronometerType::Page: {
            return PageChronometer().duration(_type, _text);
        }

        case ChronometerType::Characters: {
            return CharactersChronometer().duration(_type, _text);
        }

        default: {
            Q_ASSERT(false);
            return {};
        }
    }
}

} // namespace BusinessLayer
