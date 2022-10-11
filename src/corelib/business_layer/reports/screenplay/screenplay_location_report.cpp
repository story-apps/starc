#include "screenplay_location_report.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
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
    /**
     * @brief Модель локаций
     */
    QScopedPointer<QStandardItemModel> locationModel;

    /**
     * @brief Порядок сортировки
     */
    int sortBy = 0;
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
    // Сортируем локации
    //
    QVector<QPair<QString, LocationData>> locationsSorted;
    for (auto iter = locations.begin(); iter != locations.end(); ++iter) {
        locationsSorted.append({ iter.key(), iter.value() });
    }
    switch (d->sortBy) {
    default:
    case 0: {
        break;
    }

    case 1: {
        std::sort(locationsSorted.begin(), locationsSorted.end(),
                  [](const QPair<QString, LocationData>& _lhs,
                     const QPair<QString, LocationData>& _rhs) { return _lhs.first < _rhs.first; });
        break;
    }

    case 2: {
        std::sort(
            locationsSorted.begin(), locationsSorted.end(),
            [](const QPair<QString, LocationData>& _lhs, const QPair<QString, LocationData>& _rhs) {
                return _lhs.second.scenes() > _rhs.second.scenes();
            });
        break;
    }

    case 3: {
        std::sort(
            locationsSorted.begin(), locationsSorted.end(),
            [](const QPair<QString, LocationData>& _lhs, const QPair<QString, LocationData>& _rhs) {
                return _lhs.second.scenes() < _rhs.second.scenes();
            });
        break;
    }

    case 4: {
        std::sort(
            locationsSorted.begin(), locationsSorted.end(),
            [](const QPair<QString, LocationData>& _lhs, const QPair<QString, LocationData>& _rhs) {
                return _lhs.second.duration() > _rhs.second.duration();
            });
        break;
    }

    case 5: {
        std::sort(
            locationsSorted.begin(), locationsSorted.end(),
            [](const QPair<QString, LocationData>& _lhs, const QPair<QString, LocationData>& _rhs) {
                return _lhs.second.duration() < _rhs.second.duration();
            });
        break;
    }
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
    if (d->locationModel.isNull()) {
        d->locationModel.reset(new QStandardItemModel);
    } else {
        d->locationModel->clear();
    }
    const auto titleBackgroundColor = QVariant::fromValue(ColorHelper::transparent(
        Ui::DesignSystem::color().onBackground(), Ui::DesignSystem::elevationEndOpacity()));
    for (const auto& locationData : locationsSorted) {
        //
        // ... локация
        //
        const auto locationName = locationData.first;
        const auto& location = locationData.second;
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

        d->locationModel->appendRow({
            locationItem,
            createModelItem({}, titleBackgroundColor),
            createModelItem({}, titleBackgroundColor),
            createModelItem(QString::number(location.scenes()), titleBackgroundColor),
            createModelItem(TimeHelper::toString(location.duration()), titleBackgroundColor),
        });
    }
    //
    d->locationModel->setHeaderData(
        0, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Location/scene"),
        Qt::DisplayRole);
    d->locationModel->setHeaderData(
        1, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Number"),
        Qt::DisplayRole);
    d->locationModel->setHeaderData(
        2, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Page"),
        Qt::DisplayRole);
    d->locationModel->setHeaderData(
        3, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Scenes"),
        Qt::DisplayRole);
    d->locationModel->setHeaderData(
        4, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayLocationReport", "Duration"),
        Qt::DisplayRole);
}

void ScreenplayLocationReport::saveToFile(const QString& _fileName) const
{
    QXlsx::Document xlsx;
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    QXlsx::Format textHeaderFormat;
    textHeaderFormat.setFillPattern(QXlsx::Format::PatternLightUp);

    constexpr int firstRow = 1;
    constexpr int firstColumn = 1;
    int reportRow = firstRow;
    auto writeHeader = [&xlsx, &headerFormat, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text, headerFormat);
    };
    auto writeTextHeader
        = [&xlsx, &reportRow, &textHeaderFormat](int _column, const QVariant& _text) {
              xlsx.write(reportRow, _column, _text, textHeaderFormat);
          };
    auto writeText = [&xlsx, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text);
    };

    for (int column = firstColumn; column < firstColumn + locationModel()->columnCount();
         ++column) {
        writeHeader(column, locationModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < locationModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + locationModel()->columnCount();
             ++column) {
            writeTextHeader(column, locationModel()->index(row, column - firstColumn).data());
        }

        const auto locationIndex = locationModel()->index(row, 0);
        for (int sceneTimeRow = 0; sceneTimeRow < locationModel()->rowCount(locationIndex);
             ++sceneTimeRow) {
            ++reportRow;
            for (int sceneTimeColumn = 0;
                 sceneTimeColumn < locationModel()->columnCount(locationIndex); ++sceneTimeColumn) {
                writeText(
                    sceneTimeColumn + firstColumn,
                    locationModel()->index(sceneTimeRow, sceneTimeColumn, locationIndex).data());
            }

            const auto sceneTimeIndex = locationModel()->index(sceneTimeRow, 0, locationIndex);
            for (int sceneRow = 0; sceneRow < locationModel()->rowCount(sceneTimeIndex);
                 ++sceneRow) {
                ++reportRow;
                for (int sceneColumn = 0;
                     sceneColumn < locationModel()->columnCount(sceneTimeIndex); ++sceneColumn) {
                    writeText(sceneColumn + firstColumn,
                              locationModel()->index(sceneRow, sceneColumn, sceneTimeIndex).data());
                }
            }
        }
    }

    xlsx.saveAs(_fileName);
}

void ScreenplayLocationReport::setParameters(int _sortBy)
{
    d->sortBy = _sortBy;
}

QAbstractItemModel* ScreenplayLocationReport::locationModel() const
{
    return d->locationModel.data();
}

} // namespace BusinessLayer
