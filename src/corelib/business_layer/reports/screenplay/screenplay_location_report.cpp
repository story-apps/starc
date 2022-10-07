#include "screenplay_location_report.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QStandardItemModel>

#include <set>

namespace BusinessLayer {

class ScreenplayLocationReport::Implementation
{
public:
    QScopedPointer<QStandardItemModel> sceneModel;
};


// ****


ScreenplayLocationReport::ScreenplayLocationReport()
    : d(new Implementation)
{
}

ScreenplayLocationReport::~ScreenplayLocationReport() = default;

void ScreenplayLocationReport::build(QAbstractItemModel* _model)
{
    if (_model == nullptr) {
        return;
    }

    auto screenplayModel = qobject_cast<ScreenplayTextModel*>(_model);
    if (screenplayModel == nullptr) {
        return;
    }

    //
    // Подготовим необходимые структуры для сбора статистики
    //
    struct SceneData {
        QString name;
        QString number;
        int page = 0;
        std::chrono::milliseconds duration;
    };
    struct SceneTimeData {
        QString name;
        QVector<SceneData> scenes;

        std::chrono::milliseconds duration() const
        {
            auto duration = std::chrono::milliseconds{ 0 };
            for (const auto& scene : scenes) {
                duration += scene.duration;
            }
            return duration;
        }
    };
    struct LocationData {
        QString name;
        QHash<QString, SceneTimeData> sceneTimes;

        int scenes() const
        {
            int scenes = 0;
            for (const auto& sceneTime : sceneTimes) {
                scenes += sceneTime.scenes.size();
            }
            return scenes;
        }
        std::chrono::milliseconds duration() const
        {
            auto duration = std::chrono::milliseconds{ 0 };
            for (const auto& sceneTime : sceneTimes) {
                duration += sceneTime.duration();
            }
            return duration;
        }
    };
    QHash<QString, LocationData> locations;
    LocationData lastLocation;
    QVector<QString> locationsOrder;

    //
    // Подготовим текстовый документ, для определения страниц сцен
    //
    const auto& screenplayTemplate
        = TemplatesFacade::screenplayTemplate(screenplayModel->informationModel()->templateId());
    PageTextEdit screenplayTextEdit;
    screenplayTextEdit.setUsePageMode(true);
    screenplayTextEdit.setPageSpacing(0);
    screenplayTextEdit.setPageFormat(screenplayTemplate.pageSizeId());
    screenplayTextEdit.setPageMarginsMm(screenplayTemplate.pageMargins());
    ScreenplayTextDocument screenplayDocument;
    screenplayTextEdit.setDocument(&screenplayDocument);
    const bool kCanChangeModel = false;
    screenplayDocument.setModel(screenplayModel, kCanChangeModel);
    QTextCursor screenplayCursor(&screenplayDocument);
    auto textItemPage = [&screenplayTextEdit, &screenplayDocument,
                         &screenplayCursor](TextModelTextItem* _item) {
        screenplayCursor.setPosition(
            screenplayDocument.itemPosition(screenplayDocument.model()->indexForItem(_item), true));
        return screenplayTextEdit.cursorPage(screenplayCursor);
    };

    //
    // Собираем статистику
    //
    std::function<void(const TextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &locations, &lastLocation, &locationsOrder,
                       textItemPage](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                includeInReport(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                //
                // ... стата по объектам
                //
                switch (textItem->paragraphType()) {
                case TextParagraphType::SceneHeading: {
                    //
                    // Началась новая сцена
                    //
                    if (!lastLocation.name.isEmpty()) {
                        locations[lastLocation.name] = lastLocation;
                    }
                    //
                    const auto locationName
                        = ScreenplaySceneHeadingParser::location(textItem->text());
                    if (!locations.contains(locationName)) {
                        locationsOrder.append(locationName);
                    }
                    lastLocation = locations[locationName];
                    lastLocation.name = locationName;
                    //
                    const auto timeName = ScreenplaySceneHeadingParser::sceneTime(textItem->text());
                    if (lastLocation.sceneTimes.isEmpty()
                        || !lastLocation.sceneTimes.contains(timeName)) {
                        SceneTimeData sceneTime;
                        sceneTime.name = timeName;
                        lastLocation.sceneTimes.insert(timeName, sceneTime);
                    }
                    //
                    SceneData scene;
                    const auto sceneItem
                        = static_cast<ScreenplayTextModelSceneItem*>(textItem->parent());
                    scene.name = sceneItem->heading();
                    scene.number = sceneItem->number()->text;
                    scene.duration = sceneItem->duration();
                    scene.page = textItemPage(textItem);
                    lastLocation.sceneTimes[timeName].scenes.append(scene);

                    break;
                }

                default:
                    break;
                }

                break;
            }

            default:
                break;
            }
        }
    };
    includeInReport(screenplayModel->itemForIndex({}));
    if (!lastLocation.name.isEmpty()) {
        locations[lastLocation.name] = lastLocation;
    }

    //
    // Формируем отчёт
    //
    auto createModelItem = [](const QString& _text, const QVariant _backgroundColor = {}) {
        auto item = new QStandardItem(_text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        if (_backgroundColor.isValid()) {
            item->setData(_backgroundColor, Qt::BackgroundRole);
        }
        return item;
    };
    //
    // ... наполняем таблицу
    //
    if (d->sceneModel.isNull()) {
        d->sceneModel.reset(new QStandardItemModel);
    } else {
        d->sceneModel->clear();
    }
    const auto titleBackgroundColor = QVariant::fromValue(ColorHelper::transparent(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::elevationEndOpacity()));
    for (const auto& locationName : locationsOrder) {
        //
        // ... локация
        //
        const auto& location = locations[locationName];
        auto locationItem = createModelItem(location.name, titleBackgroundColor);
        //
        // ... время
        //
        auto sceneTimeNames = location.sceneTimes.keys();
        std::sort(sceneTimeNames.begin(), sceneTimeNames.end());
        for (const auto& sceneTimeName : sceneTimeNames) {
            const auto sceneTime = location.sceneTimes[sceneTimeName];
            auto sceneTimeItem
                = createModelItem(sceneTime.name.isEmpty() ? QCoreApplication::translate(
                                      "BusinessLayer::ScreenplayLocationReport", "NOT SET")
                                                           : sceneTime.name);

            for (const auto& scene : sceneTime.scenes) {
                sceneTimeItem->appendRow({
                    createModelItem(scene.name),
                    createModelItem(scene.number),
                    createModelItem(QString::number(scene.page)),
                    createModelItem({}),
                    createModelItem(TimeHelper::toString(scene.duration)),
                });
            }

            locationItem->appendRow({
                sceneTimeItem,
                createModelItem({}),
                createModelItem({}),
                createModelItem(QString::number(sceneTime.scenes.size())),
                createModelItem(TimeHelper::toString(sceneTime.duration())),
            });
        }

        d->sceneModel->appendRow({
            locationItem,
            createModelItem({}, titleBackgroundColor),
            createModelItem({}, titleBackgroundColor),
            createModelItem(QString::number(location.scenes()), titleBackgroundColor),
            createModelItem(TimeHelper::toString(location.duration()), titleBackgroundColor),
        });
    }
    //
    d->sceneModel->setHeaderData(
        0, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Location/scene"),
        Qt::DisplayRole);
    d->sceneModel->setHeaderData(
        1, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Number"),
        Qt::DisplayRole);
    d->sceneModel->setHeaderData(
        2, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Page"),
        Qt::DisplayRole);
    d->sceneModel->setHeaderData(
        3, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Scenes"),
        Qt::DisplayRole);
    d->sceneModel->setHeaderData(
        4, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Duration"),
        Qt::DisplayRole);
}

QAbstractItemModel* ScreenplayLocationReport::sceneModel() const
{
    return d->sceneModel.data();
}

} // namespace BusinessLayer
