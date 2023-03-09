#include "screenplay_breakdown_structure_locations_model.h"

#include "screenplay_breakdown_structure_model_item.h"

#include <business_layer/model/abstract_model_item.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/text_template.h>
#include <utils/tools/debouncer.h>

#include <QMimeData>
#include <QPointer>


namespace BusinessLayer {

namespace {
const char* kMimeType = "application/x-starc/breakdown/location";
}

class ScreenplayBreakdownStructureLocationsModel::Implementation
{
public:
    explicit Implementation(ScreenplayBreakdownStructureLocationsModel* _q);

    /**
     * @brief Получить элемент находящийся в заданном индексе
     */
    ScreenplayBreakdownStructureModelItem* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Получить индекс заданного элемента
     */
    QModelIndex indexForItem(ScreenplayBreakdownStructureModelItem* _item) const;

    /**
     * @brief Очистить данные модели
     */
    void clear();

    /**
     * @brief Собрать модель из исходной
     */
    void buildModel();

    /**
     * @brief Транзакция, для управления операция сброса данных модели
     */
    void beginResetModelTransaction();
    void endResetModelTransaction();


    ScreenplayBreakdownStructureLocationsModel* q = nullptr;

    /**
     * @brief Модель сценария, на базе которой строится данная модель
     */
    QPointer<ScreenplayTextModel> model;

    /**
     * @brief Список персонажей для сохранения
     */
    QSet<QString> locationsToSave;

    /**
     * @brief Корневой элемент модели
     */
    QScopedPointer<ScreenplayBreakdownStructureModelItem> rootItem;

    /**
     * @brief Счётчик транзакций операции сброса модели
     */
    int resetModelTransationsCounter = 0;

    /**
     * @brief Последний подсвеченный элемент
     */
    QModelIndex lastHighlightedSceneIndex;
    QVector<ScreenplayBreakdownStructureModelItem*> lastHighlightedItems;

    /**
     * @brief Последние положенные в майм элементы
     */
    mutable QVector<ScreenplayBreakdownStructureModelItem*> lastMimeItems;

    /**
     * @brief Дебаунсер на перестройку модели локаций при изменении модели текста
     */
    Debouncer modelRebuildDebouncer;
};

ScreenplayBreakdownStructureLocationsModel::Implementation::Implementation(
    ScreenplayBreakdownStructureLocationsModel* _q)
    : q(_q)
    , rootItem(new ScreenplayBreakdownStructureModelItem({}))
    , modelRebuildDebouncer(300)
{
}

ScreenplayBreakdownStructureModelItem* ScreenplayBreakdownStructureLocationsModel::Implementation::
    itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return rootItem.data();
    }

    auto item = static_cast<ScreenplayBreakdownStructureModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return rootItem.data();
    }

    return item;
}

QModelIndex ScreenplayBreakdownStructureLocationsModel::Implementation::indexForItem(
    ScreenplayBreakdownStructureModelItem* _item) const
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
        row = rootItem->rowOfChild(_item);
    }

    return q->index(row, 0, parent);
}

void ScreenplayBreakdownStructureLocationsModel::Implementation::clear()
{
    lastHighlightedItems.clear();

    if (!rootItem->hasChildren()) {
        return;
    }

    beginResetModelTransaction();
    while (rootItem->childCount() > 0) {
        rootItem->removeItem(rootItem->childAt(0));
    }
    endResetModelTransaction();
}

void ScreenplayBreakdownStructureLocationsModel::Implementation::buildModel()
{
    //
    // Считываем данные модели
    //
    QVector<QString> locations;
    QSet<QString> newLocations;
    QHash<QString, QVector<QModelIndex>> locationsScenes;
    //
    // ... берём все локации из модели
    //
    for (int row = 0; row < model->locationsModel()->rowCount(); ++row) {
        const auto location = model->locationsModel()->index(row, 0).data().toString();
        locations.append(location);
    }
    //
    // ... далее читаем текст сценария
    //
    auto saveLocation = [this, &locations, &newLocations,
                         &locationsScenes](const QString& _name, TextModelTextItem* _textItem) {
        if (_name.simplified().isEmpty()) {
            return;
        }

        //
        // Тут имена локаций с разными разделителями приводим к единому виду,
        // чтобы единообразно потом их обрабатывать
        //
        auto nameCorrected = _name.simplified();
        nameCorrected.replace(". ", " - ");

        const auto sceneIndex = model->indexForItem(_textItem).parent();

        const auto locationIndex = locations.indexOf(nameCorrected);
        //
        // Если такой локации ещё нет
        //
        constexpr int invalidIndex = -1;
        if (locationIndex == invalidIndex) {
            //
            // ... и мы ещё не планировали ей сохранять
            //
            if (!locationsToSave.contains(_name)) {
                //
                // ... запланируем сохранение локации
                //
                newLocations.insert(_name);
            }
            //
            // ... а если сохранение было запланировано
            //
            else {
                //
                // ... то сохраним
                //
                model->createLocation(_name);
            }
            locations.append(nameCorrected);
            locationsScenes.insert(nameCorrected, { sceneIndex });
            return;
        }

        auto& scenes = locationsScenes[nameCorrected];
        if (scenes.isEmpty() || scenes.constLast() != sceneIndex) {
            scenes.append(sceneIndex);
        }
    };
    std::function<void(const TextModelItem*)> findLocationsInText;
    findLocationsInText = [&findLocationsInText, &saveLocation](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                findLocationsInText(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                if (textItem->paragraphType() == TextParagraphType::SceneHeading) {
                    saveLocation(ScreenplaySceneHeadingParser::location(textItem->text()),
                                 textItem);
                }

                break;
            }

            default:
                break;
            }
        }
    };
    findLocationsInText(model->itemForIndex({}));
    //
    // ... сортируем список локаций
    //
    {
        QVector<QString> orderedLocations;
        QSet<QString> placedLocations;
        for (const auto& location : locations) {
            const auto locationParts = location.split(" - ");
            //
            // ... если это состовная локация, то обрабатываем каждый уровень
            //
            if (locationParts.size() > 1) {
                QString currentLocation;
                for (const auto& locationPart : locationParts) {
                    const auto parentLocation = currentLocation;
                    if (!currentLocation.isEmpty()) {
                        currentLocation.append(" - ");
                    }
                    currentLocation.append(locationPart);
                    if (!placedLocations.contains(currentLocation)) {
                        placedLocations.insert(currentLocation);
                        //
                        // ... если это верхнеуровневая локация, то просто добавляем в список
                        //
                        if (currentLocation == locationParts.constFirst()) {
                            orderedLocations.append(currentLocation);
                        }
                        //
                        // ... а если второй и последующие уровни, то ищем лучшее расположение
                        //
                        else {
                            for (int index = 0; index < orderedLocations.size(); ++index) {
                                const auto& orderedLocation = orderedLocations.at(index);
                                if (!orderedLocation.startsWith(parentLocation)) {
                                    continue;
                                }
                                //
                                // ... если дошли до конца списка,
                                //     или следующая локация имеет уже другого родителя
                                //
                                if (index == orderedLocations.size() - 1
                                    || !orderedLocations.at(index + 1).startsWith(parentLocation)) {
                                    //
                                    // ... нашли идеальное место
                                    //
                                    orderedLocations.insert(index + 1, currentLocation);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            //
            // ... а если простая, то лишь проверяем была ли она уже сохранена
            //
            else {
                if (!placedLocations.contains(location)) {
                    placedLocations.insert(location);
                    orderedLocations.append(location);
                }
            }
        }
        //
        // ... далее будем использовать отсортированные локации
        //
        locations.swap(orderedLocations);
    }
    //
    // ... удаляем локации, которые были запланированы, но так и не были сохранены
    //
    locationsToSave = newLocations;
    //
    // ... удаляем локации, которые не участвую ни в одной сцене
    //
    for (auto iter = locations.begin(); iter != locations.end();) {
        if (!locationsScenes.contains(*iter)) {
            iter = locations.erase(iter);
        } else {
            ++iter;
        }
    }
    //
    // ... сохраняем собранные данные
    //
    beginResetModelTransaction();
    //
    // ... сперва формируем нормализованную структуру
    //
    std::function<void(ScreenplayBreakdownStructureModelItem*, QVector<QString>&, const QString&)>
        fillLocation;
    fillLocation
        = [&fillLocation, &locationsScenes](ScreenplayBreakdownStructureModelItem* parentItem,
                                            QVector<QString>& locations, const QString& _prefix) {
              while (!locations.empty()) {
                  const auto location = locations.takeFirst();

                  const auto locationParts = location.mid(_prefix.length()).split(" - ");
                  QVector<QString> sublocations;
                  if (location.startsWith(_prefix)) {
                      sublocations.append(location);
                      while (!locations.empty()
                             && locations.constFirst().startsWith(
                                 _prefix + locationParts.constFirst() + " - ")) {
                          const auto sublocation = locations.takeFirst();
                          sublocations.append(sublocation);
                      }
                  }
                  //
                  // Если у локации есть подлокации
                  //
                  if (!sublocations.empty()) {
                      //
                      // ... создаём родительский элемент этой локации и дальше обрабатываем
                      // вложенные
                      //
                      auto locationItem
                          = new ScreenplayBreakdownStructureModelItem(locationParts.constFirst());
                      fillLocation(locationItem, sublocations,
                                   _prefix + locationParts.constFirst() + " - ");

                      parentItem->appendItem(locationItem);
                  }
                  //
                  // А если подлокаций нет
                  //
                  else {
                      //
                      // ... сохраняем локацию в текущем родительском элементе
                      //
                      if (location.startsWith(_prefix)) {
                          auto locationItem = new ScreenplayBreakdownStructureModelItem(location);
                          const auto locationScenes = locationsScenes[location];
                          for (const auto& sceneIndex : locationScenes) {
                              locationItem->appendItem(new ScreenplayBreakdownStructureModelItem(
                                  sceneIndex.data(TextModelGroupItem::GroupNumberRole).toString()
                                      + " " + sceneIndex.data().toString(),
                                  sceneIndex));
                          }

                          parentItem->appendItem(locationItem);
                      }
                      //
                      // ... сохраняем список сцен текущей локации
                      //
                      else {
                          const auto locationScenes = locationsScenes[location];
                          for (const auto& sceneIndex : locationScenes) {
                              parentItem->appendItem(new ScreenplayBreakdownStructureModelItem(
                                  sceneIndex.data(TextModelGroupItem::GroupNumberRole).toString()
                                      + " " + sceneIndex.data().toString(),
                                  sceneIndex));
                          }
                      }
                      //
                      // ... помещаем оставшиеся в списке элементы в текущего родителя
                      //
                      fillLocation(parentItem, locations, _prefix);
                  }
              }
          };
    fillLocation(rootItem.data(), locations, {});
    //
    // ... затем схлапываем её, чтобы не было пустых уровней вложенности
    //
    std::function<void(ScreenplayBreakdownStructureModelItem*)> optimizeItem;
    optimizeItem = [&optimizeItem](ScreenplayBreakdownStructureModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            optimizeItem(_item->childAt(childIndex));
        }

        if (_item->childCount() == 1 && !_item->childAt(0)->isScene()) {
            auto childItem = _item->childAt(0);
            while (childItem->hasChildren()) {
                auto grandChildItem = childItem->childAt(0);
                childItem->takeItem(grandChildItem);
                _item->appendItem(grandChildItem);
            }
            _item->name += " - " + childItem->name;
            _item->removeItem(childItem);
        }
    };
    optimizeItem(rootItem.data());
    //
    endResetModelTransaction();
}

void ScreenplayBreakdownStructureLocationsModel::Implementation::beginResetModelTransaction()
{
    if (resetModelTransationsCounter == 0) {
        q->beginResetModel();
    }

    ++resetModelTransationsCounter;
}

void ScreenplayBreakdownStructureLocationsModel::Implementation::endResetModelTransaction()
{
    --resetModelTransationsCounter;

    if (resetModelTransationsCounter == 0) {
        q->endResetModel();
    }
}


// ****


ScreenplayBreakdownStructureLocationsModel::ScreenplayBreakdownStructureLocationsModel(
    QObject* _parent)
    : QAbstractItemModel(_parent)
    , d(new Implementation(this))
{
    connect(&d->modelRebuildDebouncer, &Debouncer::gotWork, this, [this] {
        d->beginResetModelTransaction();
        d->clear();
        d->buildModel();
        d->endResetModelTransaction();

        setSourceModelCurrentIndex(d->lastHighlightedSceneIndex);
    });
}

ScreenplayBreakdownStructureLocationsModel::~ScreenplayBreakdownStructureLocationsModel()
{
    d->clear();
}

void ScreenplayBreakdownStructureLocationsModel::setSourceModel(ScreenplayTextModel* _model)
{
    if (!d->model.isNull()) {
        disconnect(d->model);
        d->clear();
        d->locationsToSave.clear();
    }

    d->model = _model;

    if (!d->model.isNull()) {
        d->buildModel();

        //
        // Наблюдаем за событиями модели, чтобы обновлять собственные данные
        //
        connect(d->model, &TextModel::modelAboutToBeReset, this, [this] { d->clear(); });
        connect(d->model, &TextModel::modelReset, this, [this] { setSourceModel(d->model); });
        connect(d->model, &TextModel::rowsInserted, &d->modelRebuildDebouncer,
                &Debouncer::orderWork);
        connect(d->model, &TextModel::rowsRemoved, &d->modelRebuildDebouncer,
                &Debouncer::orderWork);
        connect(d->model, &TextModel::rowsMoved, &d->modelRebuildDebouncer, &Debouncer::orderWork);
        connect(d->model, &TextModel::dataChanged, &d->modelRebuildDebouncer,
                &Debouncer::orderWork);
    }
}

void ScreenplayBreakdownStructureLocationsModel::sortBy(ScreenplayBreakdownSortOrder _sortOrder)
{
    if (d->model == nullptr) {
        return;
    }

    d->beginResetModelTransaction();

    //
    // Отсортировать текущую структуру
    //
    const std::function<bool(AbstractModelItem*, AbstractModelItem*)> sorter
        = [_sortOrder](AbstractModelItem* _lhs, AbstractModelItem* _rhs) {
              const auto lhs = static_cast<ScreenplayBreakdownStructureModelItem*>(_lhs);
              const auto rhs = static_cast<ScreenplayBreakdownStructureModelItem*>(_rhs);

              switch (_sortOrder) {
              case ScreenplayBreakdownSortOrder::Undefined: {
                  return false;
              }

              case ScreenplayBreakdownSortOrder::Alphabetically: {
                  return lhs->name < rhs->name;
              }

              case ScreenplayBreakdownSortOrder::ByDuration: {
                  return lhs->duration > rhs->duration;
              }

              default: {
                  Q_ASSERT(false);
                  return false;
              }
              }
          };
    std::function<void(ScreenplayBreakdownStructureModelItem*)> sort;
    sort = [&sort, &sorter](ScreenplayBreakdownStructureModelItem* _item) {
        if (!_item->hasChildren()) {
            return;
        }

        _item->sortChildren(sorter);
        for (int index = 0; index < _item->childCount(); ++index) {
            sort(_item->childAt(index));
        }
    };
    sort(d->rootItem.data());

    //
    // Сформировать плоский список локаций
    //
    QVector<ScreenplayBreakdownStructureModelItem*> locations;
    std::function<void(ScreenplayBreakdownStructureModelItem*)> addLocation;
    addLocation = [&addLocation, &locations](ScreenplayBreakdownStructureModelItem* _item) {
        if (_item->isScene()) {
            return;
        }

        if (!_item->name.isEmpty()) {
            locations.append(_item);
        }

        for (int index = 0; index < _item->childCount(); ++index) {
            addLocation(_item->childAt(index));
        }
    };
    addLocation(d->rootItem.data());

    //
    // Упорядочить элементы в модели локаций
    //
    auto locationsModel = d->model->locationsModel();
    int index = 0;
    for (const auto& location : locations) {
        auto locationName = location->name;
        auto locationParent = location->parent();
        while (locationParent != nullptr) {
            if (!locationParent->name.isEmpty()) {
                locationName.prepend(locationParent->name + " - ");
            }
            locationParent = locationParent->parent();
        }
        auto locationNameDots = locationName;
        locationNameDots.replace(" - ", ". ");
        if (locationsModel->exists(locationName)) {
            locationsModel->moveLocation(locationName, index);
        } else if (locationsModel->exists(locationNameDots)) {
            locationsModel->moveLocation(locationNameDots, index);
        } else {
            continue;
        }

        ++index;
    }

    //
    // Перестроить текущую модель с учётом сортировки
    //
    d->clear();
    d->buildModel();

    d->endResetModelTransaction();
}

void ScreenplayBreakdownStructureLocationsModel::setSourceModelCurrentIndex(
    const QModelIndex& _index)
{
    if (d->lastHighlightedSceneIndex == _index && !d->lastHighlightedItems.isEmpty()) {
        return;
    }

    //
    // Снять выделение с последних выделенных
    //
    for (auto highlightedItem : std::as_const(d->lastHighlightedItems)) {
        highlightedItem->setHighlighted(false);
        auto itemForUpdate = highlightedItem;
        while (itemForUpdate != nullptr) {
            const auto parentIndex = d->indexForItem(itemForUpdate);
            emit dataChanged(parentIndex, parentIndex);
            itemForUpdate = itemForUpdate->parent();
        }
    }
    d->lastHighlightedItems.clear();

    //
    // Запоминаем новый индекс сцены для выделения
    //
    d->lastHighlightedSceneIndex = _index;
    if (!_index.isValid()) {
        return;
    }

    //
    // Выделяем элементы, у которых есть искомая сцена
    //
    std::function<bool(ScreenplayBreakdownStructureModelItem*)> updateHighlighted;
    updateHighlighted
        = [this, _index, &updateHighlighted](ScreenplayBreakdownStructureModelItem* _item) {
              if (_item->screenplayItemIndex == _index) {
                  d->lastHighlightedItems.append(_item);
                  _item->setHighlighted(true);
                  auto itemForUpdate = _item;
                  while (itemForUpdate != nullptr) {
                      const auto parentIndex = d->indexForItem(itemForUpdate);
                      emit dataChanged(parentIndex, parentIndex);
                      itemForUpdate = itemForUpdate->parent();
                  }
                  return true;
              }

              for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
                  const auto isItemUpdated = updateHighlighted(_item->childAt(childIndex));
                  if (isItemUpdated) {
                      return true;
                  }
              }
              return false;
          };
    updateHighlighted(d->rootItem.data());
}

QModelIndex ScreenplayBreakdownStructureLocationsModel::index(int _row, int _column,
                                                              const QModelIndex& _parent) const
{
    if (_row < 0 || _row > rowCount(_parent) || _column < 0 || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))) {
        return {};
    }

    auto parentItem = d->itemForIndex(_parent);
    Q_ASSERT(parentItem);

    auto indexItem = parentItem->childAt(_row);
    if (indexItem == nullptr) {
        return {};
    }

    return createIndex(_row, _column, indexItem);
}

QModelIndex ScreenplayBreakdownStructureLocationsModel::parent(const QModelIndex& _child) const
{
    if (!_child.isValid()) {
        return {};
    }

    auto childItem = d->itemForIndex(_child);
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

int ScreenplayBreakdownStructureLocationsModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int ScreenplayBreakdownStructureLocationsModel::rowCount(const QModelIndex& _parent) const
{
    if (_parent.isValid() && _parent.column() != 0) {
        return 0;
    }

    auto item = d->itemForIndex(_parent);
    if (item == nullptr) {
        return 0;
    }

    return item->childCount();
}

Qt::ItemFlags ScreenplayBreakdownStructureLocationsModel::flags(const QModelIndex& _index) const
{
    //
    // Вставлять можно только в рута
    //
    if (!_index.isValid()) {
        return Qt::ItemIsDropEnabled;
    }

    //
    // Перетаскивать можно только элементы верхнего уровня
    //
    if (!_index.parent().isValid()) {
        return Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ScreenplayBreakdownStructureLocationsModel::data(const QModelIndex& _index,
                                                          int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    auto item = d->itemForIndex(_index);
    if (item == nullptr) {
        return {};
    }

    return item->data(_role);
}

bool ScreenplayBreakdownStructureLocationsModel::canDropMimeData(const QMimeData* _data,
                                                                 Qt::DropAction _action, int _row,
                                                                 int _column,
                                                                 const QModelIndex& _parent) const
{
    Q_UNUSED(_action)
    Q_UNUSED(_row)
    Q_UNUSED(_parent)

    if (!_data->hasFormat(kMimeType)) {
        return false;
    }

    if (_column > 0) {
        return false;
    }

    if (d->lastMimeItems.isEmpty()) {
        return false;
    }

    //
    // Проверяем, что перемещаются данные из модели
    //
    QByteArray encodedData = _data->data(kMimeType);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int row = 0;
    while (!stream.atEnd()) {
        QString itemName;
        stream >> itemName;
        if (itemName != d->lastMimeItems[row]->name) {
            //
            // ... если это какие-то внешние данные, то ничего не делаем
            //
            return false;
        }

        ++row;
    }

    return true;
}

bool ScreenplayBreakdownStructureLocationsModel::dropMimeData(const QMimeData* _data,
                                                              Qt::DropAction _action, int _row,
                                                              int _column,
                                                              const QModelIndex& _parent)
{
    if (!canDropMimeData(_data, _action, _row, _column, _parent)) {
        return false;
    }

    if (_action == Qt::IgnoreAction) {
        return true;
    }

    //
    // Перемещаем все элементы по очереди
    //

    auto parentItem = d->itemForIndex(_parent);

    //
    // Добавляем после всех элементов выбранного
    //
    if (_row == -1 || _row == parentItem->childCount()) {
        //
        // Если вставляем перед корзиной, то добавляем дельту
        //
        const int recycleBinIndexDelta = 0;

        while (!d->lastMimeItems.isEmpty()) {
            auto item = d->lastMimeItems.takeFirst();
            const auto itemIndex = d->indexForItem(item);

            if (item->parent() == parentItem
                && itemIndex.row() == (parentItem->childCount() - 1 + recycleBinIndexDelta)) {
                continue;
            }

            beginMoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row(), _parent,
                          parentItem->childCount() + recycleBinIndexDelta);
            item->parent()->takeItem(item);
            parentItem->insertItem(parentItem->childCount() + recycleBinIndexDelta, item);
            endMoveRows();
        }
    }
    //
    // Вставляем между элементами
    //
    else {
        const QModelIndex insertBeforeItemIndex = index(_row, _column, _parent);
        auto insertBeforeItem = d->itemForIndex(insertBeforeItemIndex);

        if (d->lastMimeItems.contains(insertBeforeItem)) {
            return false;
        }

        while (!d->lastMimeItems.isEmpty()) {
            auto item = d->lastMimeItems.takeFirst();
            auto itemIndex = d->indexForItem(item);

            //
            // Нет смысла перемещать элемент на то же самое место
            //
            if (itemIndex.parent() == insertBeforeItemIndex.parent()
                && (itemIndex.row() == insertBeforeItemIndex.row()
                    || itemIndex.row() == insertBeforeItemIndex.row() - 1)) {
                continue;
            }

            beginMoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row(),
                          insertBeforeItemIndex.parent(), _row);
            item->parent()->takeItem(item);
            parentItem->insertItem(parentItem->rowOfChild(insertBeforeItem), item);
            endMoveRows();
        }
    }

    //
    // После того, как переставили элементы, нужно обновить порядок следования элементов
    // в исходной модели персонажей
    //
    sortBy(ScreenplayBreakdownSortOrder::Undefined);

    return true;
}

QMimeData* ScreenplayBreakdownStructureLocationsModel::mimeData(
    const QModelIndexList& _indexes) const
{
    d->lastMimeItems.clear();

    if (_indexes.isEmpty()) {
        return nullptr;
    }

    //
    // Формируем список элементов для перемещения
    //
    for (const QModelIndex& index : _indexes) {
        if (index.isValid()) {
            d->lastMimeItems << d->itemForIndex(index);
        }
    }
    //
    // ... и упорядочиваем его
    //
    std::sort(d->lastMimeItems.begin(), d->lastMimeItems.end(),
              [](ScreenplayBreakdownStructureModelItem* _lhs,
                 ScreenplayBreakdownStructureModelItem* _rhs) {
                  //
                  // Для элементов находящихся на одном уровне сравниваем их позиции
                  //
                  if (_lhs->parent() == _rhs->parent()) {
                      return _lhs->parent()->rowOfChild(_lhs) < _rhs->parent()->rowOfChild(_rhs);
                  }

                  //
                  // Для разноуровневых элементов определяем путь до верха и сравниваем пути
                  //
                  auto buildPath = [](ScreenplayBreakdownStructureModelItem* _item) {
                      QString path;
                      auto child = _item;
                      auto parent = child->parent();
                      while (parent != nullptr) {
                          path.prepend(QString("0000%1").arg(parent->rowOfChild(child)).right(5));
                          child = parent;
                          parent = child->parent();
                      }
                      return path;
                  };
                  return buildPath(_lhs) < buildPath(_rhs);
              });

    //
    // Помещаем индексы перемещаемых элементов в майм
    //
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    for (const auto& item : std::as_const(d->lastMimeItems)) {
        stream << item->name;
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData(kMimeType, encodedData);
    return mimeData;
}

QStringList ScreenplayBreakdownStructureLocationsModel::mimeTypes() const
{
    return { kMimeType };
}

Qt::DropActions ScreenplayBreakdownStructureLocationsModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions ScreenplayBreakdownStructureLocationsModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

} // namespace BusinessLayer
