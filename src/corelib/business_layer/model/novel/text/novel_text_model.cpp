#include "novel_text_model.h"

#include "novel_text_block_parser.h"
#include "novel_text_model_beat_item.h"
#include "novel_text_model_folder_item.h"
#include "novel_text_model_scene_item.h"
#include "novel_text_model_text_item.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/templates/novel_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>

#include <QRegularExpression>
#include <QStringListModel>
#include <QXmlStreamReader>


namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/novel/text/item";
}

class NovelTextModel::Implementation
{
public:
    explicit Implementation(NovelTextModel* _q);

    /**
     * @brief Получить корневой элемент
     */
    TextModelItem* rootItem() const;

    /**
     * @brief Пересчитать количество сцен
     */
    //
    // TODO: Возможно стоит добавить обновление номеров сцен, как в аналогичных моделях
    //
    void updateNumbering();

    /**
     * @brief Пересчитать счетчики элемента и всех детей
     */
    void updateChildrenCounters(const TextModelItem* _item);


    /**
     * @brief Родительский элемент
     */
    NovelTextModel* q = nullptr;

    /**
     * @brief Модель информации о проекте
     */
    NovelInformationModel* informationModel = nullptr;

    /**
     * @brief Модель справочников
     */
    NovelDictionariesModel* dictionariesModel = nullptr;

    /**
     * @brief Количество страниц
     */
    int outlinePageCount = 0;
    int textPageCount = 0;

    /**
     * @brief Количество сцен
     */
    int scenesCount = 0;

    /**
     * @brief Последний сохранённый хэш документа
     */
    QByteArray lastContentHash;

    /**
     * @brief Запланировано ли обновление нумерации
     */
    bool isUpdateNumberingPlanned = false;
};

NovelTextModel::Implementation::Implementation(NovelTextModel* _q)
    : q(_q)
{
}

TextModelItem* NovelTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}

void NovelTextModel::Implementation::updateNumbering()
{
    if (isUpdateNumberingPlanned) {
        return;
    }

    scenesCount = 0;
    std::function<void(const TextModelItem*)> updateChildNumbering;
    updateChildNumbering = [this, &updateChildNumbering](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder: {
                updateChildNumbering(childItem);
                break;
            }

            case TextModelItemType::Group: {
                auto groupItem = static_cast<TextModelGroupItem*>(childItem);
                if (groupItem->groupType() == TextGroupType::Scene) {
                    ++scenesCount;
                }
                break;
            }

            default:
                break;
            }
        }
    };
    updateChildNumbering(rootItem());
}

void NovelTextModel::Implementation::updateChildrenCounters(const TextModelItem* _item)
{
    if (_item == nullptr) {
        return;
    }
    for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
        auto childItem = _item->childAt(childIndex);
        switch (childItem->type()) {
        case TextModelItemType::Folder:
        case TextModelItemType::Group: {
            updateChildrenCounters(childItem);
            break;
        }

        case TextModelItemType::Text: {
            auto textItem = static_cast<NovelTextModelTextItem*>(childItem);
            textItem->updateCounters();
            break;
        }

        default:
            break;
        }
    }
}


// ****


NovelTextModel::NovelTextModel(QObject* _parent)
    : ScriptTextModel(_parent, NovelTextModel::createFolderItem(TextFolderType::Root))
    , d(new Implementation(this))
{
    auto updateCounters = [this](const QModelIndex& _index) {
        if (const auto hash = contentHash(); d->lastContentHash != hash) {
            d->updateNumbering();
            d->lastContentHash = hash;
        }

        d->updateChildrenCounters(itemForIndex(_index));
    };
    //
    // Обновляем счётчики после того, как операции вставки и удаления будут обработаны клиентами
    // модели (главным образом внутри прокси-моделей), т.к. обновление элемента модели может
    // приводить к падению внутри них
    //
    connect(this, &NovelTextModel::afterRowsInserted, this, updateCounters);
    connect(this, &NovelTextModel::afterRowsRemoved, this, updateCounters);
    //
    // Если модель планируем большое изменение, то планируем отложенное обновление нумерации
    //
    connect(this, &NovelTextModel::rowsAboutToBeChanged, this,
            [this] { d->isUpdateNumberingPlanned = true; });
    connect(this, &NovelTextModel::rowsChanged, this, [this] {
        if (d->isUpdateNumberingPlanned) {
            d->isUpdateNumberingPlanned = false;
            d->updateNumbering();
        }
    });
}

NovelTextModel::~NovelTextModel() = default;

QString NovelTextModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Novel"), informationModel()->name());
}

TextModelFolderItem* NovelTextModel::createFolderItem(TextFolderType _type) const
{
    return new NovelTextModelFolderItem(this, _type);
}

TextModelGroupItem* NovelTextModel::createGroupItem(TextGroupType _type) const
{
    switch (_type) {
    case TextGroupType::Scene: {
        return new NovelTextModelSceneItem(this);
    }

    case TextGroupType::Beat: {
        return new NovelTextModelBeatItem(this);
    }

    default: {
        Q_ASSERT(false);
        return nullptr;
    }
    }
}

TextModelTextItem* NovelTextModel::createTextItem() const
{
    return new NovelTextModelTextItem(this);
}

QStringList NovelTextModel::mimeTypes() const
{
    return { kMimeType };
}

void NovelTextModel::setInformationModel(NovelInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    if (d->informationModel) {
        d->informationModel->disconnect(this);
    }

    d->informationModel = _model;
}

NovelInformationModel* NovelTextModel::informationModel() const
{
    return d->informationModel;
}

void NovelTextModel::setDictionariesModel(NovelDictionariesModel* _model)
{
    d->dictionariesModel = _model;
}

NovelDictionariesModel* NovelTextModel::dictionariesModel() const
{
    return d->dictionariesModel;
}

void NovelTextModel::updateCharacterName(const QString& _oldName, const QString& _newName)
{
    Q_UNUSED(_oldName)
    Q_UNUSED(_newName)
}

QVector<QModelIndex> NovelTextModel::characterDialogues(const QString& _name) const
{
    Q_UNUSED(_name)
    return {};
}

QVector<QString> NovelTextModel::findCharactersFromText() const
{
    return {};
}

void NovelTextModel::updateLocationName(const QString& _oldName, const QString& _newName)
{
    Q_UNUSED(_oldName)
    Q_UNUSED(_newName)
}

QVector<QModelIndex> NovelTextModel::locationScenes(const QString& _name) const
{
    Q_UNUSED(_name)
    return {};
}

QVector<QString> NovelTextModel::findLocationsFromText() const
{
    return {};
}

int NovelTextModel::outlinePageCount() const
{
    return d->outlinePageCount;
}

void NovelTextModel::setOutlinePageCount(int _count)
{
    if (d->outlinePageCount == _count) {
        return;
    }

    d->outlinePageCount = _count;

    //
    // Создаём фейковое уведомление, чтобы оповестить клиентов
    //
    emit dataChanged(index(0, 0), index(0, 0));
}

int NovelTextModel::textPageCount() const
{
    return d->textPageCount;
}

void NovelTextModel::setTextPageCount(int _count)
{
    if (d->textPageCount == _count) {
        return;
    }

    d->textPageCount = _count;

    //
    // Создаём фейковое уведомление, чтобы оповестить клиентов
    //
    emit dataChanged(index(0, 0), index(0, 0));
}

int NovelTextModel::scenesCount() const
{
    return d->scenesCount;
}

int NovelTextModel::wordsCount() const
{
    return static_cast<NovelTextModelFolderItem*>(d->rootItem())->wordsCount();
}

QPair<int, int> NovelTextModel::charactersCount() const
{
    return static_cast<NovelTextModelFolderItem*>(d->rootItem())->charactersCount();
}

void NovelTextModel::updateRuntimeDictionaries()
{
}

void NovelTextModel::initEmptyDocument()
{
    auto sceneHeading = new NovelTextModelTextItem(this);
    sceneHeading->setParagraphType(TextParagraphType::SceneHeading);
    auto scene = new NovelTextModelSceneItem(this);
    scene->appendItem(sceneHeading);
    appendItem(scene);
}

void NovelTextModel::finalizeInitialization()
{
    d->updateNumbering();
}

} // namespace BusinessLayer
