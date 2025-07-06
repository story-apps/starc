#include "screenplay_series_location_report.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/series/screenplay_series_episodes_model.h>
#include <business_layer/model/screenplay/series/screenplay_series_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/model_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/helpers/time_helper.h>

#include <QCoreApplication>
#include <QPdfWriter>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardItemModel>


namespace BusinessLayer {

class ScreenplaySeriesLocationReport::Implementation
{
public:
    /**
     * @brief Модель аудиопостановки
     */
    QPointer<ScreenplaySeriesEpisodesModel> episodesModel;

    /**
     * @brief Модель локаций
     */
    QScopedPointer<QStandardItemModel> locationModel;

    /**
     * @brief Нужно ли отображать детали по сценам
     */
    bool extendedView = true;

    /**
     * @brief Порядок сортировки
     */
    int sortBy = 0;
};


// ****


ScreenplaySeriesLocationReport::ScreenplaySeriesLocationReport()
    : d(new Implementation)
{
}

ScreenplaySeriesLocationReport::~ScreenplaySeriesLocationReport() = default;

bool ScreenplaySeriesLocationReport::isValid() const
{
    return d->locationModel->rowCount() > 0;
}

void ScreenplaySeriesLocationReport::build(QAbstractItemModel* _model)
{
    if (_model == nullptr) {
        return;
    }

    d->episodesModel = qobject_cast<ScreenplaySeriesEpisodesModel*>(_model);
    if (d->episodesModel == nullptr) {
        return;
    }

    //
    // Подготовим модель к наполнению
    //
    if (d->locationModel.isNull()) {
        d->locationModel.reset(new QStandardItemModel);
    } else {
        d->locationModel->clear();
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
    // Если в сериях одинаковые шаблоны, то добавляем автоматически номер эпизода
    //
    bool needToAddEpisodeNumber = false;
    QSet<QString> sceneNumbersTemplates;
    for (const auto episode : d->episodesModel->episodes()) {
        if (sceneNumbersTemplates.contains(episode->informationModel()->scenesNumbersTemplate())) {
            needToAddEpisodeNumber = true;
            break;
        }

        sceneNumbersTemplates.insert(episode->informationModel()->scenesNumbersTemplate());
    }

    //
    // Собираем статистику со всех сценариев
    //
    int episodeNumber = 1;
    for (const auto episode : d->episodesModel->episodes()) {
        const auto sceneNumberPrefix
            = needToAddEpisodeNumber ? QString("%1.").arg(episodeNumber++) : "";
        //
        // Подготовим текстовый документ, для определения страниц сцен
        //
        const auto& screenplayTemplate
            = TemplatesFacade::screenplayTemplate(episode->informationModel()->templateId());
        PageTextEdit screenplayTextEdit;
        screenplayTextEdit.setUsePageMode(true);
        screenplayTextEdit.setPageSpacing(0);
        screenplayTextEdit.setPageFormat(screenplayTemplate.pageSizeId());
        screenplayTextEdit.setPageMarginsMm(screenplayTemplate.pageMargins());
        ScreenplayTextDocument screenplayDocument;
        screenplayTextEdit.setDocument(&screenplayDocument);
        const bool kCanChangeModel = false;
        screenplayDocument.setModel(episode, kCanChangeModel);
        QTextCursor screenplayCursor(&screenplayDocument);
        auto textItemPage = [&screenplayTextEdit, &screenplayDocument,
                             &screenplayCursor](TextModelTextItem* _item) {
            screenplayCursor.setPosition(screenplayDocument.itemPosition(
                screenplayDocument.model()->indexForItem(_item), true));
            return screenplayTextEdit.cursorPage(screenplayCursor);
        };

        //
        // Собираем статистику
        //
        std::function<void(const TextModelItem*)> includeInReport;
        includeInReport = [&includeInReport, &locations, &lastLocation, &locationsOrder,
                           textItemPage, sceneNumberPrefix](const TextModelItem* _item) {
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
                        const auto timeName
                            = ScreenplaySceneHeadingParser::sceneTime(textItem->text());
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
                        scene.number = sceneNumberPrefix + sceneItem->number()->text;
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
        includeInReport(episode->itemForIndex({}));
    }
    if (!lastLocation.name.isEmpty()) {
        locations[lastLocation.name] = lastLocation;
    }

    //
    // Прерываем выполнение, если в сценариях нет сцен
    //
    if (locations.isEmpty()
        || std::count_if(locations.begin(), locations.end(),
                         [](const LocationData& _location) { return _location.name.isEmpty(); })
            == locations.size()) {
        return;
    }

    //
    // Сортируем локации
    //
    QVector<QPair<QString, LocationData>> locationsSorted;
    for (const auto& location : locationsOrder) {
        locationsSorted.append({ location, locations[location] });
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
    const auto titleBackgroundColor = d->extendedView
        ? QVariant::fromValue(ColorHelper::transparent(Ui::DesignSystem::color().onBackground(),
                                                       Ui::DesignSystem::elevationEndOpacity()))
        : QVariant();
    for (const auto& locationData : locationsSorted) {
        //
        // ... локация
        //
        const auto locationName = locationData.first;
        const auto& location = locationData.second;
        auto locationItem = createModelItem(location.name, titleBackgroundColor);
        if (d->extendedView) {
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

void ScreenplaySeriesLocationReport::setParameters(bool _extendedView, int _sortBy)
{
    d->extendedView = _extendedView;
    d->sortBy = _sortBy;
}

QAbstractItemModel* ScreenplaySeriesLocationReport::locationModel() const
{
    return d->locationModel.data();
}

void ScreenplaySeriesLocationReport::saveToPdf(const QString& _fileName) const
{
    const auto& exportTemplate
        = TemplatesFacade::screenplayTemplate(d->episodesModel->informationModel()->templateId());

    //
    // Настраиваем документ
    //
    PageTextEdit textEdit;
    textEdit.setUsePageMode(true);
    textEdit.setPageSpacing(0);
    QTextDocument report;
    report.setDefaultFont(exportTemplate.baseFont());
    textEdit.setDocument(&report);
    //
    // ... параметры страницы
    //
    textEdit.setPageFormat(exportTemplate.pageSizeId());
    textEdit.setPageMarginsMm(exportTemplate.pageMargins());
    textEdit.setPageNumbersAlignment(exportTemplate.pageNumbersAlignment());

    //
    // Формируем отчёт
    //
    QTextCursor cursor(&report);
    QTextCharFormat titleFormat;
    auto titleFont = report.defaultFont();
    titleFont.setBold(true);
    titleFormat.setFont(titleFont);
    cursor.setCharFormat(titleFormat);
    cursor.insertText(QString("%1 - %2").arg(
        d->episodesModel->informationModel()->name(),
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Scene report")));
    cursor.insertBlock();
    cursor.insertBlock();
    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    tableFormat.setColumnWidthConstraints({
        QTextLength{ QTextLength::PercentageLength, 60 },
        QTextLength{ QTextLength::PercentageLength, 10 },
        QTextLength{ QTextLength::PercentageLength, 10 },
        QTextLength{ QTextLength::PercentageLength, 10 },
        QTextLength{ QTextLength::PercentageLength, 10 },
    });
    const auto beforeTablePosition = cursor.position();
    cursor.insertTable(ModelHelper::recursiveRowCount(locationModel()) + 1,
                       locationModel()->columnCount(), tableFormat);
    cursor.setPosition(beforeTablePosition);
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.beginEditBlock();
    //
    for (int column = 0; column < locationModel()->columnCount(); ++column) {
        QTextTableCellFormat cellFormat;
        cellFormat.setBottomBorder(1);
        cellFormat.setVerticalAlignment(QTextCharFormat::AlignBottom);
        cellFormat.setBottomBorderStyle(QTextFrameFormat::BorderStyle_Solid);
        cellFormat.setBottomBorderBrush(Qt::black);
        cursor.mergeBlockCharFormat(cellFormat);
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
        cursor.setBlockFormat(blockFormat);
        cursor.insertText(locationModel()->headerData(column, Qt::Horizontal).toString());

        cursor.movePosition(QTextCursor::NextBlock);
    }
    for (int row = 0; row < locationModel()->rowCount(); ++row) {
        const auto sceneIndex = locationModel()->index(row, 0);
        const auto hasChildren = locationModel()->rowCount(sceneIndex) > 0;

        for (int column = 0; column < locationModel()->columnCount(); ++column) {
            QTextBlockFormat blockFormat = cursor.blockFormat();
            blockFormat.setAlignment(column == 0 ? Qt::AlignLeft : Qt::AlignRight);
            cursor.setBlockFormat(blockFormat);
            QTextCharFormat textFormat = cursor.blockCharFormat();
            textFormat.setFontWeight(hasChildren ? QFont::Weight::Bold : QFont::Weight::Normal);
            cursor.insertText(locationModel()->index(row, column).data().toString(), textFormat);
            cursor.movePosition(QTextCursor::NextBlock);
        }

        //
        // Добавляем детали
        //
        if (d->extendedView) {
            for (int childRow = 0; childRow < locationModel()->rowCount(sceneIndex); ++childRow) {
                const auto childIndex = locationModel()->index(childRow, 0, sceneIndex);
                for (int childColumn = 0; childColumn < locationModel()->columnCount();
                     ++childColumn) {
                    QTextBlockFormat blockFormat = cursor.blockFormat();
                    blockFormat.setAlignment(childColumn == 0 ? Qt::AlignLeft : Qt::AlignRight);
                    cursor.setBlockFormat(blockFormat);
                    auto textFormat = cursor.blockCharFormat();
                    textFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                    cursor.insertText(
                        locationModel()->index(childRow, childColumn, sceneIndex).data().toString(),
                        textFormat);
                    cursor.movePosition(QTextCursor::NextBlock);
                }

                for (int grandChildRow = 0; grandChildRow < locationModel()->rowCount(childIndex);
                     ++grandChildRow) {
                    for (int grandChildColumn = 0;
                         grandChildColumn < locationModel()->columnCount(); ++grandChildColumn) {
                        QTextBlockFormat blockFormat = cursor.blockFormat();
                        blockFormat.setAlignment(grandChildColumn == 0 ? Qt::AlignLeft
                                                                       : Qt::AlignRight);
                        cursor.setBlockFormat(blockFormat);
                        const auto grandChildIndex
                            = locationModel()->index(grandChildRow, grandChildColumn, childIndex);
                        auto textFormat = cursor.blockCharFormat();
                        cursor.insertText(grandChildIndex.data().toString(), textFormat);
                        cursor.movePosition(QTextCursor::NextBlock);
                    }
                }
            }
        }

        if (hasChildren) {
            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.insertText({ QChar::LineSeparator });
            cursor.movePosition(QTextCursor::NextBlock);
        }
    }
    cursor.endEditBlock();

    //
    // Печатаем
    //
    QPdfWriter printer(_fileName);
    printer.setPageSize(QPageSize(exportTemplate.pageSizeId()));
    printer.setPageMargins({});
    report.print(&printer);
}

void ScreenplaySeriesLocationReport::saveToXlsx(const QString& _fileName) const
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

} // namespace BusinessLayer
