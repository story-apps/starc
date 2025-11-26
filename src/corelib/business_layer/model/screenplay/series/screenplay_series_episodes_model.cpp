#include "screenplay_series_episodes_model.h"

#include "../screenplay_information_model.h"
#include "../text/screenplay_text_model.h"
#include "screenplay_series_episodes_model_episode_item.h"
#include "screenplay_series_episodes_model_story_line_item.h"
#include "screenplay_series_information_model.h"

#include <business_layer/chronometry/chronometer.h>
#include <business_layer/model/abstract_model_xml.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kStoryLinesKey("story_lines");
const QLatin1String kStoryLineKey("story_line");
const QLatin1String kColorKey("color");
const QLatin1String kNameKey("name");
} // namespace

class ScreenplaySeriesEpisodesModel::Implementation
{
public:
    explicit Implementation(ScreenplaySeriesEpisodesModel* _q);

    /**
     * @brief Обновить параметры эпизодов
     */
    void updateEpisodesSettings();

    /**
     * @brief Пересобрать модель
     */
    void rebuildModel();


    ScreenplaySeriesEpisodesModel* q = nullptr;

    /**
     * @brief Закончена ли инициилизация документа
     */
    bool initializationFinished = false;

    /**
     * @brief Модель информации о проекте
     */
    ScreenplaySeriesInformationModel* informationModel = nullptr;

    /**
     * @brief Список серий
     */
    QVector<ScreenplayTextModel*> episodes;

    /**
     * @brief Сюжетные линии
     */
    QVector<StoryLine> storyLines;

    /**
     * @brief Корневой элемент дерева
     */
    QScopedPointer<ScreenplaySeriesEpisodesModelItem> rootItem;
};

ScreenplaySeriesEpisodesModel::Implementation::Implementation(ScreenplaySeriesEpisodesModel* _q)
    : q(_q)
    , rootItem(new ScreenplaySeriesEpisodesModelItem)
{
}

void ScreenplaySeriesEpisodesModel::Implementation::updateEpisodesSettings()
{
    Q_ASSERT(informationModel);

    for (const auto& episode : std::as_const(episodes)) {
        auto episodeInformationModel = episode->informationModel();
        episodeInformationModel->setCanCommonSettingsBeOverridden(false);
        episodeInformationModel->setOverrideCommonSettings(
            informationModel->overrideCommonSettings());
        episodeInformationModel->setTemplateId(informationModel->templateId());
        episodeInformationModel->setShowSceneNumbers(informationModel->showSceneNumbers());
        episodeInformationModel->setShowSceneNumbersOnLeft(
            informationModel->showSceneNumbersOnLeft());
        episodeInformationModel->setShowSceneNumbersOnRight(
            informationModel->showSceneNumbersOnRight());
        episodeInformationModel->setShowDialoguesNumbers(informationModel->showDialoguesNumbers());
        episodeInformationModel->setChronometerOptions(informationModel->chronometerOptions());

        if (initializationFinished) {
            //
            // Нормализуем количество сюжетных линий в эпизоде
            //
            auto episodeStoryLines = episodeInformationModel->storyLines();
            while (episodeStoryLines.size() > storyLines.size()) {
                episodeStoryLines.removeLast();
            }
            while (episodeStoryLines.size() < storyLines.size()) {
                episodeStoryLines.append(QString());
            }
            episodeInformationModel->setStoryLines(episodeStoryLines);
        }
    }
}

void ScreenplaySeriesEpisodesModel::Implementation::rebuildModel()
{
    q->beginResetModel();

    rootItem.reset(new ScreenplaySeriesEpisodesModelItem);
    //
    // ... сперва добавляем пустой эпизод и заголовки сюжетных линий
    //
    auto fakeEpisodeItem = new ScreenplaySeriesEpisodesModelEpisodeItem;
    fakeEpisodeItem->setStoryLinesContainer(true);
    for (const auto& storyLine : std::as_const(storyLines)) {
        auto storyLineItem = new ScreenplaySeriesEpisodesModelStoryLineItem;
        storyLineItem->setStoryLineTitle(true);
        storyLineItem->setColor(storyLine.color);
        storyLineItem->setName(storyLine.name);
        fakeEpisodeItem->appendItem(storyLineItem);
    }
    rootItem->appendItem(fakeEpisodeItem);
    //
    // ... далее добавляем уонкртеные эпизоды и их описания сюжетных линий
    //
    for (const auto episode : std::as_const(episodes)) {
        const auto episodeInfo = episode->informationModel();
        auto episodeItem = new ScreenplaySeriesEpisodesModelEpisodeItem;
        episodeItem->setName(episodeInfo->name());
        for (int index = 0; index < episodeInfo->storyLines().size(); ++index) {
            auto storyLineItem = new ScreenplaySeriesEpisodesModelStoryLineItem;
            storyLineItem->setColor(storyLines.value(index).color);
            storyLineItem->setName(episodeInfo->storyLines().value(index));
            episodeItem->appendItem(storyLineItem);
        }
        rootItem->appendItem(episodeItem);
    }

    q->endResetModel();
}


// ****


bool ScreenplaySeriesEpisodesModel::StoryLine::operator==(const StoryLine& _other) const
{
    return color == _other.color && name == _other.name;
}

ScreenplaySeriesEpisodesModel::ScreenplaySeriesEpisodesModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kStoryLinesKey,
            kStoryLineKey,
            kColorKey,
            kNameKey,
        },
        _parent)
    , d(new Implementation(this))
{
    connect(this, &ScreenplaySeriesEpisodesModel::storyLinesChanged, this,
            &ScreenplaySeriesEpisodesModel::updateDocumentContent);
}

ScreenplaySeriesEpisodesModel::~ScreenplaySeriesEpisodesModel() = default;

void ScreenplaySeriesEpisodesModel::setInformationModel(ScreenplaySeriesInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    if (d->informationModel) {
        d->informationModel->disconnect(this);
    }

    d->informationModel = _model;

    if (d->informationModel) {
        connect(d->informationModel,
                &ScreenplaySeriesInformationModel::overrideCommonSettingsChanged, this,
                [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel, &ScreenplaySeriesInformationModel::templateIdChanged, this,
                [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel, &ScreenplaySeriesInformationModel::showSceneNumbersChanged,
                this, [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel,
                &ScreenplaySeriesInformationModel::showSceneNumbersOnLeftChanged, this,
                [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel,
                &ScreenplaySeriesInformationModel::showSceneNumbersOnRightChanged, this,
                [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel, &ScreenplaySeriesInformationModel::showDialoguesNumbersChanged,
                this, [this] { d->updateEpisodesSettings(); });
        connect(d->informationModel, &ScreenplaySeriesInformationModel::chronometerOptionsChanged,
                this, [this] { d->updateEpisodesSettings(); });
    }
}

ScreenplaySeriesInformationModel* ScreenplaySeriesEpisodesModel::informationModel() const
{
    return d->informationModel;
}

QVector<ScreenplayTextModel*> ScreenplaySeriesEpisodesModel::episodes() const
{
    return d->episodes;
}

void ScreenplaySeriesEpisodesModel::setEpisodes(const QVector<ScreenplayTextModel*>& _episodes)
{
    if (d->episodes == _episodes) {
        return;
    }

    //
    // Снимаем запрет на переопределение параметров для сценариев, которые больше не в сериале
    //
    for (const auto& episode : std::as_const(d->episodes)) {
        if (!_episodes.contains(episode)) {
            episode->informationModel()->setCanCommonSettingsBeOverridden(true);
        }
    }

    //
    // Для сценариев, которые были добавлены в сериал включаем запрет на переопределение и
    // синхронизируем значения параметров с параметрами сериала
    //
    d->episodes = _episodes;
    d->updateEpisodesSettings();

    emit episodesChanged(_episodes);

    //
    // Обновляем структуру модели
    //
    d->rebuildModel();
}

ScreenplaySeriesEpisodesModelItem* ScreenplaySeriesEpisodesModel::appendStoryLine()
{
    //
    // Создаём заголовок сюжетной линии
    //
    auto fakeEpisodeItem = static_cast<ScreenplaySeriesEpisodesModelItem*>(d->rootItem->childAt(0));
    beginInsertRows(indexForItem(fakeEpisodeItem), fakeEpisodeItem->childCount(),
                    fakeEpisodeItem->childCount());
    d->storyLines.append({ QColor(), QString() });
    auto storyLineItem = new ScreenplaySeriesEpisodesModelStoryLineItem;
    storyLineItem->setStoryLineTitle(true);
    fakeEpisodeItem->appendItem(storyLineItem);
    endInsertRows();

    //
    // И добавляем заготовки под сюжетную линию во все эпизоды
    //
    for (int index = 1; index < d->rootItem->childCount(); ++index) {
        auto& episode
            = d->episodes[index - 1]; // отнимаем единицу, т.к. в списке нет фейкового эпизода
        auto episodeStoryLines = episode->informationModel()->storyLines();
        episodeStoryLines.append(QString());
        // Q_ASSERT(episodeStoryLines.size() == d->storyLines.size());
        episode->informationModel()->setStoryLines(episodeStoryLines);

        auto episodeItem
            = static_cast<ScreenplaySeriesEpisodesModelItem*>(d->rootItem->childAt(index));
        beginInsertRows(indexForItem(episodeItem), episodeItem->childCount(),
                        episodeItem->childCount());
        episodeItem->appendItem(new ScreenplaySeriesEpisodesModelStoryLineItem);
        endInsertRows();
    }

    emit storyLinesChanged(d->storyLines);

    return storyLineItem;
}

QVector<ScreenplaySeriesEpisodesModel::StoryLine> ScreenplaySeriesEpisodesModel::storyLines() const
{
    return d->storyLines;
}

void ScreenplaySeriesEpisodesModel::setStoryLines(const QVector<StoryLine>& _storyLines)
{
    if (d->storyLines == _storyLines) {
        return;
    }

    d->storyLines = _storyLines;
    for (auto episode : std::as_const(d->episodes)) {
        auto episodeStoryLines = episode->informationModel()->storyLines();
        if (episodeStoryLines.size() > d->storyLines.size()) {
            while (episodeStoryLines.size() > d->storyLines.size()) {
                episodeStoryLines.removeLast();
            }
        } else if (episodeStoryLines.size() < d->storyLines.size()) {
            while (episodeStoryLines.size() < d->storyLines.size()) {
                episodeStoryLines.append(QString());
            }
        }
        episode->informationModel()->setStoryLines(episodeStoryLines);
    }

    d->rebuildModel();

    emit storyLinesChanged(d->storyLines);
}

void ScreenplaySeriesEpisodesModel::updateItem(ScreenplaySeriesEpisodesModelItem* _item)
{
    if (_item == nullptr || !_item->isChanged()) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);

    //
    // Обновим внутренние данные модели
    //
    if (_item->type() == ScreenplaySeriesEpisodesModelItemType::StoryLine) {
        auto storyLineItem = static_cast<ScreenplaySeriesEpisodesModelStoryLineItem*>(_item);
        //
        // Если обновлися заголовок сюжетных линий, то обновить цвет сюжетной линии всех эпизодов
        //
        if (storyLineItem->isStoryLineTitle()) {
            d->storyLines[indexForUpdate.row()] = { storyLineItem->color(), storyLineItem->name() };

            for (int index = 1; index < d->rootItem->childCount(); ++index) {
                auto child = d->rootItem->childAt(index)->childAt(indexForUpdate.row());
                auto childStoryLineItem
                    = static_cast<ScreenplaySeriesEpisodesModelStoryLineItem*>(child);
                childStoryLineItem->setColor(storyLineItem->color());
                updateItem(childStoryLineItem);
            }
        }
        //
        // Если обновилась сюжетная линия эпизода, то нужно обновить модель информации о нём
        //
        else {
            auto episode = d->episodes[indexForUpdate.parent().row() - 1];
            auto episodeStoryLines = episode->informationModel()->storyLines();
            episodeStoryLines[indexForUpdate.row()] = storyLineItem->name();
            episode->informationModel()->setStoryLines(episodeStoryLines);
        }
    }
    //
    // Если обновился эпизод, корректируем его параметры
    //
    else if (_item->type() == ScreenplaySeriesEpisodesModelItemType::Episode) {
        auto episodeItem = static_cast<ScreenplaySeriesEpisodesModelEpisodeItem*>(_item);
        auto episode = d->episodes[indexForUpdate.row() - 1];
        episode->informationModel()->setName(episodeItem->name());
    }

    emit dataChanged(indexForUpdate, indexForUpdate);
    _item->setChanged(false);
}

void ScreenplaySeriesEpisodesModel::removeItem(ScreenplaySeriesEpisodesModelItem* _item)
{
    if (_item == nullptr || _item->parent() == nullptr) {
        return;
    }

    //
    // Удалять эпизоды нельзя
    //
    if (_item->type() != ScreenplaySeriesEpisodesModelItemType::StoryLine) {
        return;
    }

    const auto storyLineItem = static_cast<ScreenplaySeriesEpisodesModelStoryLineItem*>(_item);
    const auto storyLineIndex = indexForItem(storyLineItem).row();

    //
    // Удалить заголовок сюжетной линии
    //
    d->storyLines.removeAt(storyLineIndex);

    //
    // Удалить сюжетные линии из эпизодов
    //
    for (int index = 0; index < d->rootItem->childCount(); ++index) {
        auto episodeItem
            = static_cast<ScreenplaySeriesEpisodesModelItem*>(d->rootItem->childAt(index));
        beginRemoveRows(indexForItem(episodeItem), storyLineIndex, storyLineIndex);
        episodeItem->removeItem(episodeItem->childAt(storyLineIndex));
        endRemoveRows();
    }
}

QModelIndex ScreenplaySeriesEpisodesModel::index(int _row, int _column,
                                                 const QModelIndex& _parent) const
{
    if (_row < 0 || _row > rowCount(_parent) || _column < 0 || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))) {
        return {};
    }

    auto parentItem = itemForIndex(_parent);
    Q_ASSERT(parentItem);

    auto indexItem = parentItem->childAt(_row);
    if (indexItem == nullptr) {
        return {};
    }

    return createIndex(_row, _column, indexItem);
}

QModelIndex ScreenplaySeriesEpisodesModel::parent(const QModelIndex& _child) const
{
    if (!_child.isValid()) {
        return {};
    }

    auto childItem = itemForIndex(_child);
    auto parentItem = childItem->parent();
    if (parentItem == nullptr || parentItem == d->rootItem.data()) {
        return {};
    }

    auto grandParentItem = parentItem->parent();
    if (grandParentItem == nullptr) {
        return {};
    }

    const auto row = grandParentItem->rowOfChild(parentItem);
    return createIndex(row, 0, parentItem);
}

int ScreenplaySeriesEpisodesModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int ScreenplaySeriesEpisodesModel::rowCount(const QModelIndex& _parent) const
{
    if (_parent.isValid() && _parent.column() != 0) {
        return 0;
    }

    auto item = itemForIndex(_parent);
    if (item == nullptr) {
        return 0;
    }

    return item->childCount();
}

QVariant ScreenplaySeriesEpisodesModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    auto item = itemForIndex(_index);
    if (item == nullptr) {
        return {};
    }

    return item->data(_role);
}

ScreenplaySeriesEpisodesModelItem* ScreenplaySeriesEpisodesModel::itemForIndex(
    const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem.data();
    }

    auto item = static_cast<ScreenplaySeriesEpisodesModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem.data();
    }

    return item;
}

QModelIndex ScreenplaySeriesEpisodesModel::indexForItem(
    ScreenplaySeriesEpisodesModelItem* _item) const
{
    if (_item == nullptr) {
        return {};
    }

    int row = 0;
    QModelIndex parent;
    if (_item->hasParent() && _item->parent()->hasParent()) {
        row = _item->parent()->rowOfChild(_item);
        parent = indexForItem(_item->parent());
    } else {
        row = d->rootItem->rowOfChild(_item);
    }

    return index(row, 0, parent);
}

void ScreenplaySeriesEpisodesModel::initDocument()
{
    if (d->initializationFinished || document() == nullptr) {
        return;
    }

    //
    // TODO: выпилить переприменение контента в версии 0.9
    //
    if (document()->content().isNull()) {
        reassignContent();
    }

    QDomDocument domDocument;
    const auto isContentValid = domDocument.setContent(document()->content());
    if (!isContentValid) {
        return;
    }

    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    d->storyLines.clear();
    if (const auto storyLinesNode = documentNode.firstChildElement(kStoryLinesKey);
        !storyLinesNode.isNull()) {
        for (int index = 0; index < storyLinesNode.childNodes().size(); ++index) {
            const auto child = storyLinesNode.childNodes().at(index).toElement();
            d->storyLines.append(
                { child.attribute(kColorKey), TextHelper::fromHtmlEscaped(child.text()) });
        }
    }
    d->initializationFinished = true;

    d->updateEpisodesSettings();
    d->rebuildModel();
}

void ScreenplaySeriesEpisodesModel::clearDocument()
{
    d->storyLines.clear();
    d->initializationFinished = false;
}

QByteArray ScreenplaySeriesEpisodesModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    xml += QString("<%1>\n").arg(kStoryLinesKey).toUtf8();
    for (const auto& storyLine : std::as_const(d->storyLines)) {
        xml += QString("<%1 %2=\"%3\"><![CDATA[%4]]></%1>\n")
                   .arg(kStoryLineKey, kColorKey,
                        storyLine.color.isValid() ? storyLine.color.name() : "",
                        TextHelper::toHtmlEscaped(storyLine.name))
                   .toUtf8();
    }
    xml += QString("</%1>\n").arg(kStoryLinesKey).toUtf8();
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor ScreenplaySeriesEpisodesModel::applyPatch(const QByteArray& _patch)
{
    //
    // Определить область изменения в xml
    //
    auto changes = dmpController().changedXml(toXml(), _patch);
    if (changes.first.xml.isEmpty() && changes.second.xml.isEmpty()) {
        Log::warning("Screenplay information model patch don't lead to any changes");
        return {};
    }

    changes.second.xml = xml::prepareXml(changes.second.xml);

    QDomDocument domDocument;
    domDocument.setContent(changes.second.xml);
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto setStoryLinesVector
        = [&documentNode](const QString& _key,
                          std::function<void(const QVector<StoryLine>&)> _setter) {
              const auto storyLinesNode = documentNode.firstChildElement(_key);
              QVector<StoryLine> value;
              if (!storyLinesNode.isNull()) {
                  for (int index = 0; index < storyLinesNode.childNodes().size(); ++index) {
                      const auto child = storyLinesNode.childNodes().at(index).toElement();
                      value.append({ child.attribute(kColorKey),
                                     TextHelper::fromHtmlEscaped(child.text()) });
                  }
              }
              _setter(value);
          };
    using M = ScreenplaySeriesEpisodesModel;
    const auto _1 = std::placeholders::_1;
    setStoryLinesVector(kStoryLinesKey, std::bind(&M::setStoryLines, this, _1));

    return {};
}

} // namespace BusinessLayer
