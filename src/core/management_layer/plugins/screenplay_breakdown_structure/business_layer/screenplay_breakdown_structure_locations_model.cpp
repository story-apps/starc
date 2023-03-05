#include "screenplay_breakdown_structure_locations_model.h"

#include "screenplay_breakdown_structure_model_item.h"

#include <business_layer/model/abstract_model_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/text_template.h>
#include <utils/tools/debouncer.h>

#include <QPointer>

#include <set>


namespace BusinessLayer {

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
    QModelIndex lastHighlightedItemIndex;
    ScreenplayBreakdownStructureModelItem* lastHighlightedItem = nullptr;

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
    lastHighlightedItem = nullptr;

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
    std::set<QString> locations;
    QHash<QString, QVector<QModelIndex>> locationsScenes;
    //
    auto saveLocation
        = [this, &locations, &locationsScenes](const QString& _name, TextModelTextItem* _textItem) {
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

              const auto iter = locations.find(nameCorrected);
              if (iter == locations.end()) {
                  locations.insert(nameCorrected);
                  locationsScenes.insert(nameCorrected, { sceneIndex });
                  return;
              }

              auto& scenes = locationsScenes[nameCorrected];
              if (scenes.constLast() != sceneIndex) {
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
    // ... сохраняем собранные данные
    //
    beginResetModelTransaction();
    //
    // ... сперва формируем нормализованную структуру
    //
    std::function<void(ScreenplayBreakdownStructureModelItem*, std::set<QString>&, const QString&)>
        fillLocation;
    fillLocation
        = [&fillLocation, &locationsScenes](ScreenplayBreakdownStructureModelItem* parentItem,
                                            std::set<QString>& locations, const QString& _prefix) {
              while (!locations.empty()) {
                  const auto location = *locations.begin();
                  locations.erase(location);

                  const auto locationParts = location.mid(_prefix.length()).split(" - ");
                  std::set<QString> sublocations;
                  if (location.startsWith(_prefix)) {
                      sublocations.insert(location);
                      while (!locations.empty()
                             && locations.begin()->startsWith(_prefix + locationParts.constFirst()
                                                              + " - ")) {
                          const auto sublocation = *locations.begin();
                          locations.erase(sublocation);
                          sublocations.insert(sublocation);
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

        setSourceModelCurrentIndex(d->lastHighlightedItemIndex);
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
        connect(d->model, &TextModel::rowsAboutToBeRemoved, &d->modelRebuildDebouncer,
                &Debouncer::orderWork);
        connect(d->model, &TextModel::dataChanged, &d->modelRebuildDebouncer,
                &Debouncer::orderWork);
    }
}

void ScreenplayBreakdownStructureLocationsModel::setSourceModelCurrentIndex(
    const QModelIndex& _index)
{
    if (d->lastHighlightedItemIndex == _index && d->lastHighlightedItem != nullptr) {
        return;
    }

    //
    // Снять выделение с последнего выделенного
    //
    if (d->lastHighlightedItem != nullptr) {
        d->lastHighlightedItem->setHighlighted(false);
        auto itemForUpdate = d->lastHighlightedItem;
        while (itemForUpdate != nullptr) {
            const auto parentIndex = d->indexForItem(itemForUpdate);
            emit dataChanged(parentIndex, parentIndex);
            itemForUpdate = itemForUpdate->parent();
        }
        d->lastHighlightedItem = nullptr;
    }

    d->lastHighlightedItemIndex = _index;
    if (!_index.isValid()) {
        return;
    }

    std::function<bool(ScreenplayBreakdownStructureModelItem*)> updateHighlighted;
    updateHighlighted
        = [this, _index, &updateHighlighted](ScreenplayBreakdownStructureModelItem* _item) {
              if (_item->screenplayItemIndex == _index) {
                  d->lastHighlightedItem = _item;
                  d->lastHighlightedItem->setHighlighted(true);
                  auto itemForUpdate = d->lastHighlightedItem;
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
    Q_UNUSED(_index)
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

} // namespace BusinessLayer
