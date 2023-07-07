#include "screenplay_gender_report.h"

#include <3rd_party/qtxlsxwriter/xlsxdocument.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QStandardItemModel>

#include <set>

namespace BusinessLayer {

class ScreenplayGenderReport::Implementation
{
public:
    int bechdelTest = 0;
    int reverseBeckdelTest = 0;

    QScopedPointer<QStandardItemModel> scenesInfoModel;
    QScopedPointer<QStandardItemModel> dialoguesInfoModel;
    QScopedPointer<QStandardItemModel> charactersInfoModel;
};


// ****


ScreenplayGenderReport::ScreenplayGenderReport()
    : d(new Implementation)
{
}

ScreenplayGenderReport::~ScreenplayGenderReport() = default;

void ScreenplayGenderReport::build(QAbstractItemModel* _model)
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
    int bechdelTest = 0;
    int reverseBechdelTest = 0;
    QSet<QString> male, female, other, undefined;
    struct GenderCounter {
        int male = 0;
        int female = 0;
        int other = 0;
        int undefined = 0;
        bool hasDialogues = false;
    };
    GenderCounter scenes;
    GenderCounter lastScene;
    int totalScenes = 0;
    GenderCounter dialogues;
    QSet<QString> lastSceneCharacters;

    //
    // Сформируем регулярное выражение для выуживания молчаливых персонажей
    //
    QString rxPattern;
    auto charactersModel = screenplayModel->charactersList();
    for (int index = 0; index < charactersModel->rowCount(); ++index) {
        auto characterName = charactersModel->index(index, 0).data().toString();
        if (!rxPattern.isEmpty()) {
            rxPattern.append("|");
        }
        rxPattern.append(characterName);
    }
    if (!rxPattern.isEmpty()) {
        rxPattern.prepend("(^|\\W)(");
        rxPattern.append(")($|\\W)");
    }
    const QRegularExpression rxCharacterFinder(
        rxPattern,
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);

    //
    // Соберём список персонажей
    //
    for (int index = 0; index < screenplayModel->charactersList()->rowCount(); ++index) {
        const auto characterName
            = screenplayModel->charactersList()->index(index, 0).data().toString();
        if (const auto character = screenplayModel->character(characterName)) {
            switch (character->gender()) {
            case 0: {
                male.insert(character->name());
                break;
            }
            case 1: {
                female.insert(character->name());
                break;
            }
            case 2: {
                other.insert(character->name());
                break;
            }
            case 3: {
                undefined.insert(character->name());
                break;
            }
            }
        } else {
            undefined.insert(characterName);
        }
    }

    //
    // Собираем статистику
    //
    std::function<void(const TextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &bechdelTest, &reverseBechdelTest, &male, &female, &other,
                       &undefined, &scenes, &lastScene, &totalScenes, &dialogues,
                       &lastSceneCharacters, &rxCharacterFinder](const TextModelItem* _item) {
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
                    ++totalScenes;
                    //
                    scenes.male += std::min(1, lastScene.male);
                    scenes.female += std::min(1, lastScene.female);
                    scenes.other += std::min(1, lastScene.other);
                    scenes.undefined += std::min(1, lastScene.undefined);
                    //
                    if (lastScene.male > 1 && lastScene.female == 0 && lastScene.other == 0
                        && lastScene.undefined == 0 && lastScene.hasDialogues) {
                        ++reverseBechdelTest;
                    } else if (lastScene.male == 0 && lastScene.female > 1 && lastScene.other == 0
                               && lastScene.undefined == 0 && lastScene.hasDialogues) {
                        ++bechdelTest;
                    }

                    lastScene = GenderCounter();
                    lastSceneCharacters.clear();
                    break;
                }

                case TextParagraphType::SceneCharacters: {
                    const auto sceneCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    for (const auto& character : sceneCharacters) {
                        if (male.contains(character)) {
                            ++lastScene.male;
                        } else if (female.contains(character)) {
                            ++lastScene.female;
                        } else if (other.contains(character)) {
                            ++lastScene.other;
                        } else {
                            undefined.insert(character);
                            ++lastScene.undefined;
                        }
                    }
                    break;
                }

                case TextParagraphType::Character: {
                    if (!textItem->isCorrection()) {
                        const auto character = ScreenplayCharacterParser::name(textItem->text());
                        if (male.contains(character)) {
                            ++dialogues.male;
                        } else if (female.contains(character)) {
                            ++dialogues.female;
                        } else if (other.contains(character)) {
                            ++dialogues.other;
                        } else {
                            undefined.insert(character);
                            ++dialogues.undefined;
                        }
                        if (!lastSceneCharacters.contains(character)) {
                            if (male.contains(character)) {
                                ++lastScene.male;
                            } else if (female.contains(character)) {
                                ++lastScene.female;
                            } else if (other.contains(character)) {
                                ++lastScene.other;
                            } else {
                                ++lastScene.undefined;
                            }
                            lastSceneCharacters.insert(character);
                        }
                        lastScene.hasDialogues = true;
                    }
                    break;
                }

                case TextParagraphType::Action: {
                    if (rxCharacterFinder.pattern().isEmpty()) {
                        break;
                    }

                    auto match = rxCharacterFinder.match(textItem->text());
                    while (match.hasMatch()) {
                        const QString character = TextHelper::smartToUpper(match.captured(2));
                        if (!lastSceneCharacters.contains(character)) {
                            if (male.contains(character)) {
                                ++lastScene.male;
                            } else if (female.contains(character)) {
                                ++lastScene.female;
                            } else if (other.contains(character)) {
                                ++lastScene.other;
                            } else {
                                ++lastScene.undefined;
                            }
                            lastSceneCharacters.insert(character);
                        }

                        //
                        // Ищем дальше
                        //
                        match = rxCharacterFinder.match(textItem->text(), match.capturedEnd());
                    }
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
    //
    // ... и последняя сцена
    //
    scenes.male += std::min(1, lastScene.male);
    scenes.female += std::min(1, lastScene.female);
    scenes.other += std::min(1, lastScene.other);
    scenes.undefined += std::min(1, lastScene.undefined);
    //
    if (lastScene.male > 1 && lastScene.female == 0 && lastScene.other == 0
        && lastScene.undefined == 0 && lastScene.hasDialogues) {
        ++reverseBechdelTest;
    } else if (lastScene.male == 0 && lastScene.female > 1 && lastScene.other == 0
               && lastScene.undefined == 0 && lastScene.hasDialogues) {
        ++bechdelTest;
    }

    //
    // Формируем отчёт
    //
    auto createModelItem = [](const QString& _text) {
        auto item = new QStandardItem(_text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };
    auto createPercentModelItem = [&createModelItem](qreal _percent) {
        auto item = createModelItem(QString::number(_percent, 'g', 3) + "%");
        item->setData(_percent);
        return item;
    };
    auto makeRow = [createModelItem, createPercentModelItem](int _index, int _count, int _total,
                                                             QStandardItemModel* _model) {
        const QHash<int, QString> genders = {
            { 0, QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Male") },
            { 1, QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Female") },
            { 2, QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Other") },
            { 3,
              QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Undefined") },
        };
        auto genderName = createModelItem(genders.value(_index));
        genderName->setData(u8"\U000F0766", Qt::DecorationRole);
        genderName->setData(ColorHelper::forNumber(_index++), Qt::DecorationPropertyRole);

        _model->appendRow({ genderName, createModelItem(QString::number(_count)),
                            createPercentModelItem(_count * 100.0 / _total) });
    };
    auto makeHeader = [](QStandardItemModel* _model) {
        _model->setHeaderData(
            0, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Gender"),
            Qt::DisplayRole);
        _model->setHeaderData(
            1, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Occurrences"),
            Qt::DisplayRole);
        _model->setHeaderData(
            2, Qt::Horizontal,
            QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Percents"),
            Qt::DisplayRole);
    };
    //
    // ... сводка
    //
    {
        d->bechdelTest = bechdelTest;
        d->reverseBeckdelTest = reverseBechdelTest;
    }
    //
    // ... сцены
    //
    {
        //
        // Формируем таблицу
        //
        if (d->scenesInfoModel.isNull()) {
            d->scenesInfoModel.reset(new QStandardItemModel);
        } else {
            d->scenesInfoModel->clear();
        }
        makeRow(0, scenes.male, totalScenes, d->scenesInfoModel.data());
        makeRow(1, scenes.female, totalScenes, d->scenesInfoModel.data());
        makeRow(2, scenes.other, totalScenes, d->scenesInfoModel.data());
        makeRow(3, scenes.undefined, totalScenes, d->scenesInfoModel.data());
        makeHeader(d->scenesInfoModel.data());
    }

    //
    // ... диалоги
    //
    {
        //
        // Формируем таблицу
        //
        if (d->dialoguesInfoModel.isNull()) {
            d->dialoguesInfoModel.reset(new QStandardItemModel);
        } else {
            d->dialoguesInfoModel->clear();
        }
        //
        const auto totalDialogues
            = dialogues.male + dialogues.female + dialogues.other + dialogues.undefined;
        makeRow(0, dialogues.male, totalDialogues, d->dialoguesInfoModel.data());
        makeRow(1, dialogues.female, totalDialogues, d->dialoguesInfoModel.data());
        makeRow(2, dialogues.other, totalDialogues, d->dialoguesInfoModel.data());
        makeRow(3, dialogues.undefined, totalDialogues, d->dialoguesInfoModel.data());
        makeHeader(d->dialoguesInfoModel.data());
    }
    //
    // ... персонажи
    //
    {
        //
        // Формируем таблицу
        //
        if (d->charactersInfoModel.isNull()) {
            d->charactersInfoModel.reset(new QStandardItemModel);
        } else {
            d->charactersInfoModel->clear();
        }
        //
        const auto totalCharacters = male.size() + female.size() + other.size() + undefined.size();
        makeRow(0, male.size(), totalCharacters, d->charactersInfoModel.data());
        makeRow(1, female.size(), totalCharacters, d->charactersInfoModel.data());
        makeRow(2, other.size(), totalCharacters, d->charactersInfoModel.data());
        makeRow(3, undefined.size(), totalCharacters, d->charactersInfoModel.data());
        makeHeader(d->charactersInfoModel.data());
    }
}

void ScreenplayGenderReport::saveToXlsx(const QString& _fileName) const
{
    QXlsx::Document xlsx;
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);

    constexpr int firstRow = 1;
    constexpr int firstColumn = 1;
    int reportRow = firstRow;
    auto writeTitle = [&xlsx, &headerFormat, &reportRow](const QString& _text) {
        xlsx.write(reportRow++, 1, _text, headerFormat);
        ++reportRow;
    };
    auto writeHeader = [&xlsx, &headerFormat, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text, headerFormat);
    };
    auto writeText = [&xlsx, &reportRow](int _column, const QVariant& _text) {
        xlsx.write(reportRow, _column, _text);
    };

    //
    // Сводка
    //
    writeTitle(
        QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Gender analysis"));
    int reportColumn = firstColumn;
    auto testResult = [](int _passedTimes) {
        return _passedTimes == 0
            ? QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Does not pass")
            : QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport",
                                          "Passed %n time(s)", "", _passedTimes);
    };
    writeHeader(
        reportColumn++,
        QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Bechdel test"));
    writeText(reportColumn, testResult(bechdelTest()));
    ++reportRow;
    reportColumn = firstColumn;
    writeHeader(reportColumn++,
                QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport",
                                            "Reverse Bechdel test"));
    writeText(reportColumn, testResult(reverseBechdelTest()));

    //
    // Статистика по персонажам
    //
    reportRow += 2;
    writeTitle(QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport",
                                           "Characters statistics"));
    for (int column = firstColumn; column < firstColumn + charactersInfoModel()->columnCount();
         ++column) {
        writeHeader(column,
                    charactersInfoModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < charactersInfoModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + 4; ++column) {
            writeText(column, charactersInfoModel()->index(row, column - firstColumn).data());
        }
    }

    //
    // Статистика по репликам
    //
    reportRow += 2;
    writeTitle(QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport",
                                           "Dialogues statistics"));
    for (int column = firstColumn; column < firstColumn + dialoguesInfoModel()->columnCount();
         ++column) {
        writeHeader(column, dialoguesInfoModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < dialoguesInfoModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + 3; ++column) {
            writeText(column, dialoguesInfoModel()->index(row, column - firstColumn).data());
        }
    }

    //
    // Статистика по сценам
    //
    reportRow += 2;
    writeTitle(
        QCoreApplication::translate("BusinessLayer::ScreenplayGenderReport", "Scenes statistics"));
    for (int column = firstColumn; column < firstColumn + scenesInfoModel()->columnCount();
         ++column) {
        writeHeader(column, scenesInfoModel()->headerData(column - firstColumn, Qt::Horizontal));
    }
    for (int row = 0; row < scenesInfoModel()->rowCount(); ++row) {
        ++reportRow;
        for (int column = firstColumn; column < firstColumn + 3; ++column) {
            writeText(column, scenesInfoModel()->index(row, column - firstColumn).data());
        }
    }

    xlsx.saveAs(_fileName);
}

int ScreenplayGenderReport::bechdelTest() const
{
    return d->bechdelTest;
}

int ScreenplayGenderReport::reverseBechdelTest() const
{
    return d->reverseBeckdelTest;
}

QAbstractItemModel* ScreenplayGenderReport::scenesInfoModel() const
{
    return d->scenesInfoModel.data();
}

QAbstractItemModel* ScreenplayGenderReport::dialoguesInfoModel() const
{
    return d->dialoguesInfoModel.data();
}

QAbstractItemModel* ScreenplayGenderReport::charactersInfoModel() const
{
    return d->charactersInfoModel.data();
}

} // namespace BusinessLayer
