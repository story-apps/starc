#include "screenplay_cast_report.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QStandardItemModel>


namespace BusinessLayer {

class ScreenplayCastReport::Implementation
{
public:
    QScopedPointer<QStandardItemModel> castModel;
};


// ****


ScreenplayCastReport::ScreenplayCastReport()
    : d(new Implementation)
{
}

ScreenplayCastReport::~ScreenplayCastReport() = default;

void ScreenplayCastReport::build(QAbstractItemModel* _model)
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
    struct CharacterData {
        int totalDialogues = 0;
        int speakingScenesCount = 0;
        int nonspeakingScenesCount = 0;
    };
    // - персонаж - кол-во реплик
    QHash<QString, CharacterData> charactersData;
    QSet<QString> lastSceneNonspeakingCharacters;
    QSet<QString> lastSceneSpeakingCharacters;

    //
    // Сформируем регулярное выражение для выуживания молчаливых персонажей
    //
    QString rxPattern;
    auto charactersModel = screenplayModel->charactersModel();
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
    // Собираем статистику
    //
    std::function<void(const TextModelItem*)> includeInReport;
    includeInReport = [&includeInReport, &charactersData, &lastSceneNonspeakingCharacters,
                       &lastSceneSpeakingCharacters,
                       &rxCharacterFinder](const TextModelItem* _item) {
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
                    lastSceneNonspeakingCharacters.clear();
                    lastSceneSpeakingCharacters.clear();
                    break;
                }

                case TextParagraphType::SceneCharacters: {
                    const auto sceneCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    for (const auto& character : sceneCharacters) {
                        lastSceneNonspeakingCharacters.insert(character);
                        //
                        // Первое упоминание персонажа - первая молчаливая сцена
                        //
                        if (!charactersData.contains(character)) {
                            charactersData.insert(character, { 0, 0, 1 });
                        }
                        //
                        // Не первое упоминание - плюс одна молчаливая сцена
                        //
                        else {
                            ++charactersData[character].nonspeakingScenesCount;
                        }
                    }
                    break;
                }

                case TextParagraphType::Character: {
                    const auto character = ScreenplayCharacterParser::name(textItem->text());
                    if (!charactersData.contains(character)) {
                        charactersData.insert(character, { 1, 1, 0 });
                        lastSceneSpeakingCharacters.insert(character);
                    } else {
                        auto& characterData = charactersData[character];
                        if (lastSceneNonspeakingCharacters.contains(character)) {
                            lastSceneNonspeakingCharacters.remove(character);
                            lastSceneSpeakingCharacters.insert(character);
                            --characterData.nonspeakingScenesCount;
                            ++characterData.speakingScenesCount;
                        } else if (!lastSceneSpeakingCharacters.contains(character)) {
                            lastSceneSpeakingCharacters.insert(character);
                            ++characterData.speakingScenesCount;
                        }
                        ++characterData.totalDialogues;
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
                        if (!charactersData.contains(character)) {
                            charactersData.insert(character, { 0, 0, 1 });
                        } else {
                            //
                            // Если он ещё не добавлен в текущую сцену
                            //
                            if (!lastSceneNonspeakingCharacters.contains(character)
                                && !lastSceneSpeakingCharacters.contains(character)) {
                                lastSceneNonspeakingCharacters.insert(character);
                                ++charactersData[character].nonspeakingScenesCount;
                            }
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
    // Формируем отчёт
    //
    auto createModelItem = [](const QString& _text) {
        auto item = new QStandardItem(_text);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };
    //
    // Сортируем персонажей
    //
    QVector<QPair<QString, CharacterData>> charactersSorted;
    for (auto iter = charactersData.begin(); iter != charactersData.end(); ++iter) {
        charactersSorted.append({ iter.key(), iter.value() });
    }
    std::sort(
        charactersSorted.begin(), charactersSorted.end(),
        [](const QPair<QString, CharacterData>& _lhs, const QPair<QString, CharacterData>& _rhs) {
            return _lhs.second.totalDialogues == _rhs.second.totalDialogues
                ? _lhs.first < _rhs.first
                : _lhs.second.totalDialogues > _rhs.second.totalDialogues;
        });

    //
    // Формируем таблицу
    //
    if (d->castModel.isNull()) {
        d->castModel.reset(new QStandardItemModel);
    } else {
        d->castModel->clear();
    }
    //
    auto addCharacterItemToReport
        = [this, &createModelItem](const QString& _name, const CharacterData& _count) {
              auto characterItem = createModelItem(_name);
              d->castModel->appendRow({
                  characterItem,
                  createModelItem(QString::number(_count.totalDialogues)),
                  createModelItem(QString::number(_count.speakingScenesCount)),
                  createModelItem(QString::number(_count.nonspeakingScenesCount)),
                  createModelItem(
                      QString::number(_count.speakingScenesCount + _count.nonspeakingScenesCount)),
              });
          };
    for (const auto& character : charactersSorted) {
        addCharacterItemToReport(character.first, character.second);
    }
    //
    d->castModel->setHeaderData(
        0, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Character name"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        1, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Total dialogues"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        2, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Speaking scenes"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        3, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Nonspeaking scenes"),
        Qt::DisplayRole);
    d->castModel->setHeaderData(
        4, Qt::Horizontal,
        QCoreApplication::translate("BusinessLayer::ScreenplayCastReport", "Total scenes"),
        Qt::DisplayRole);
}

QAbstractItemModel* ScreenplayCastReport::castModel() const
{
    return d->castModel.data();
}

} // namespace BusinessLayer
