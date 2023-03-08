#include "screenplay_breakdown_structure_characters_model.h"

#include "screenplay_breakdown_structure_model_item.h"

#include <business_layer/model/abstract_model_item.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/text_template.h>
#include <utils/tools/alphanum_comparer.h>
#include <utils/tools/debouncer.h>

#include <QPointer>

#include <set>


namespace BusinessLayer {

class ScreenplayBreakdownStructureCharactersModel::Implementation
{
public:
    explicit Implementation(ScreenplayBreakdownStructureCharactersModel* _q);

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


    ScreenplayBreakdownStructureCharactersModel* q = nullptr;

    /**
     * @brief Модель сценария, на базе которой строится данная модель
     */
    QPointer<ScreenplayTextModel> model;

    /**
     * @brief Список персонажей для сохранения
     */
    QSet<QString> charactersToSave;

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
     * @brief Дебаунсер на перестройку модели локаций при изменении модели текста
     */
    Debouncer modelRebuildDebouncer;
};

ScreenplayBreakdownStructureCharactersModel::Implementation::Implementation(
    ScreenplayBreakdownStructureCharactersModel* _q)
    : q(_q)
    , rootItem(new ScreenplayBreakdownStructureModelItem({}))
    , modelRebuildDebouncer(300)
{
}

ScreenplayBreakdownStructureModelItem* ScreenplayBreakdownStructureCharactersModel::Implementation::
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

QModelIndex ScreenplayBreakdownStructureCharactersModel::Implementation::indexForItem(
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

void ScreenplayBreakdownStructureCharactersModel::Implementation::clear()
{
    charactersToSave.clear();
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

void ScreenplayBreakdownStructureCharactersModel::Implementation::buildModel()
{
    //
    // Считываем данные модели
    //
    QVector<QString> characters;
    QSet<QString> newCharacters;
    QHash<QString, QVector<QModelIndex>> charactersScenes;
    //
    // ... берём всех персонажей из модели персонажей в том, порядке, в котором они идут в ней
    //
    for (int row = 0; row < model->charactersModel()->rowCount(); ++row) {
        const auto character = model->charactersModel()->index(row, 0).data().toString();
        characters.append(character);
    }
    //
    // ... далее читаем текст сценария
    //
    //
    auto saveCharacter = [this, &characters, &newCharacters,
                          &charactersScenes](const QString& _name, TextModelTextItem* _textItem) {
        if (_name.simplified().isEmpty()) {
            return;
        }

        const auto sceneIndex = model->indexForItem(_textItem).parent();

        const auto characterIndex = characters.indexOf(_name);
        constexpr int invalidIndex = -1;
        //
        // Если такого персонажа ещё нет
        //
        if (characterIndex == invalidIndex) {
            //
            // ... и мы ещё не планировали его сохранять
            //
            if (!charactersToSave.contains(_name)) {
                //
                // ... запланируем сохранение персонажа
                //
                newCharacters.insert(_name);
            }
            //
            // ... а если сохранение было запланировано
            //
            else {
                //
                // ... то сохраним
                //
                model->createCharacter(_name);
            }

            characters.append(_name);
            charactersScenes.insert(_name, { sceneIndex });
            return;
        }

        auto& scenes = charactersScenes[_name];
        if (scenes.isEmpty() || scenes.constLast() != sceneIndex) {
            scenes.append(sceneIndex);
        }
    };
    std::function<void(const TextModelItem*)> findCharactersInText;
    findCharactersInText = [&findCharactersInText, &saveCharacter](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Folder:
            case TextModelItemType::Group: {
                findCharactersInText(childItem);
                break;
            }

            case TextModelItemType::Text: {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);

                switch (textItem->paragraphType()) {
                case TextParagraphType::SceneCharacters: {
                    const auto sceneCharacters
                        = ScreenplaySceneCharactersParser::characters(textItem->text());
                    for (const auto& character : sceneCharacters) {
                        saveCharacter(character, textItem);
                    }
                    break;
                }

                case TextParagraphType::Character: {
                    saveCharacter(ScreenplayCharacterParser::name(textItem->text()), textItem);
                    break;
                }

                default: {
                    break;
                }
                }

                break;
            }

            default:
                break;
            }
        }
    };
    findCharactersInText(model->itemForIndex({}));
    //
    // ... удаляем персонажей, которые были запланированы, но так и не были сохранены
    //
    charactersToSave = newCharacters;
    //
    // ... удаляем персонажей, которые не участвую ни в одной сцене
    //
    for (int index = 0; index < characters.size(); ++index) {
        const auto& character = characters.at(index);
        if (!charactersScenes.contains(character)) {
            characters.removeAt(index);
            --index;
        }
    }
    //
    // ... сохраняем собранные данные
    //
    beginResetModelTransaction();
    //
    // ... сперва формируем нормализованную структуру
    //
    for (const auto& character : characters) {
        auto characterItem = new ScreenplayBreakdownStructureModelItem(character);
        const auto characterScenes = charactersScenes[character];
        for (const auto& sceneIndex : characterScenes) {
            characterItem->appendItem(new ScreenplayBreakdownStructureModelItem(
                sceneIndex.data(TextModelGroupItem::GroupNumberRole).toString() + " "
                    + sceneIndex.data().toString(),
                sceneIndex));
        }

        rootItem->appendItem(characterItem);
    }
    //
    endResetModelTransaction();
}

void ScreenplayBreakdownStructureCharactersModel::Implementation::beginResetModelTransaction()
{
    if (resetModelTransationsCounter == 0) {
        q->beginResetModel();
    }

    ++resetModelTransationsCounter;
}

void ScreenplayBreakdownStructureCharactersModel::Implementation::endResetModelTransaction()
{
    --resetModelTransationsCounter;

    if (resetModelTransationsCounter == 0) {
        q->endResetModel();
    }
}


// ****


ScreenplayBreakdownStructureCharactersModel::ScreenplayBreakdownStructureCharactersModel(
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

ScreenplayBreakdownStructureCharactersModel::~ScreenplayBreakdownStructureCharactersModel()
{
    d->clear();
}

void ScreenplayBreakdownStructureCharactersModel::setSourceModel(ScreenplayTextModel* _model)
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
        connect(d->model, &TextModel::rowsRemoved, &d->modelRebuildDebouncer,
                &Debouncer::orderWork);
        connect(d->model, &TextModel::rowsMoved, &d->modelRebuildDebouncer, &Debouncer::orderWork);
        connect(d->model, &TextModel::dataChanged, &d->modelRebuildDebouncer,
                &Debouncer::orderWork);
    }
}

void ScreenplayBreakdownStructureCharactersModel::sortBy(ScreenplayBreakdownSortOrder _sortOrder)
{
    if (d->model == nullptr) {
        return;
    }

    //
    // Сформировать сортированный список имён
    //
    QVector<ScreenplayBreakdownStructureModelItem*> characters;
    for (int index = 0; index < d->rootItem->childCount(); ++index) {
        characters.append(d->rootItem->childAt(index));
    }
    //
    // ... если понадобится порядок следования в сценарии, определим его
    //
    std::sort(characters.begin(), characters.end(),
              [_sortOrder](ScreenplayBreakdownStructureModelItem* _lhs,
                           ScreenplayBreakdownStructureModelItem* _rhs) {
                  switch (_sortOrder) {
                  case ScreenplayBreakdownSortOrder::Alphabetically: {
                      return _lhs->name < _rhs->name;
                  }

                  case ScreenplayBreakdownSortOrder::ByScriptOrder: {
                      return AlphanumComparer().lessThan(_lhs->childAt(0)->name,
                                                         _rhs->childAt(0)->name);
                  }

                  case ScreenplayBreakdownSortOrder::ByDuration: {
                      return _lhs->duration > _rhs->duration;
                  }

                  default: {
                      Q_ASSERT(false);
                      return false;
                  }
                  }
              });

    //
    // Упорядочить элементы в модели персонажей
    //
    auto charactersModel = d->model->charactersModel();
    for (int index = 0; index < characters.size(); ++index) {
        charactersModel->moveCharacter(characters.at(index)->name, index);
    }

    //
    // Перестроить текущую модель с учётом сортировки
    //
    d->clear();
    d->buildModel();
}

void ScreenplayBreakdownStructureCharactersModel::setSourceModelCurrentIndex(
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
                      break;
                  }
              }
              return false;
          };
    updateHighlighted(d->rootItem.data());
}

QModelIndex ScreenplayBreakdownStructureCharactersModel::index(int _row, int _column,
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

QModelIndex ScreenplayBreakdownStructureCharactersModel::parent(const QModelIndex& _child) const
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

int ScreenplayBreakdownStructureCharactersModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int ScreenplayBreakdownStructureCharactersModel::rowCount(const QModelIndex& _parent) const
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

Qt::ItemFlags ScreenplayBreakdownStructureCharactersModel::flags(const QModelIndex& _index) const
{
    Q_UNUSED(_index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ScreenplayBreakdownStructureCharactersModel::data(const QModelIndex& _index,
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
