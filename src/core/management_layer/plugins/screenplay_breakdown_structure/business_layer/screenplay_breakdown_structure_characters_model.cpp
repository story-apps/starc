#include "screenplay_breakdown_structure_characters_model.h"

#include <business_layer/model/abstract_model_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/text_template.h>

#include <QPointer>


namespace BusinessLayer {

class CharacterModelItem : public AbstractModelItem
{
public:
    explicit CharacterModelItem(const QString& _name, const QModelIndex& _screenplayItemIndex = {});

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    CharacterModelItem* parent() const override;
    CharacterModelItem* childAt(int _index) const override;

    /**
     * @brief Определяем интерфейс получения данных элемента
     */
    QVariant data(int _role) const override;


    /**
     * @brief Название элемента
     */
    QString name;

    /**
     * @brief Индекс элемента из модели сценария
     */
    QModelIndex screenplayItemIndex;
};

CharacterModelItem::CharacterModelItem(const QString& _name,
                                       const QModelIndex& _screenplayItemIndex)
    : name(_name)
    , screenplayItemIndex(_screenplayItemIndex)
{
}

CharacterModelItem* CharacterModelItem::parent() const
{
    return static_cast<CharacterModelItem*>(AbstractModelItem::parent());
}

CharacterModelItem* CharacterModelItem::childAt(int _index) const
{
    return static_cast<CharacterModelItem*>(AbstractModelItem::childAt(_index));
}

QVariant CharacterModelItem::data(int _role) const
{
    switch (_role) {
    case Qt::DisplayRole: {
        return name;
    }

    case Qt::DecorationRole: {
        return screenplayItemIndex.isValid() ? screenplayItemIndex.data(_role) : u8"\U000f0004";
    }

    default: {
        return {};
    }
    }
}

// ****


class ScreenplayBreakdownStructureCharactersModel::Implementation
{
public:
    explicit Implementation(ScreenplayBreakdownStructureCharactersModel* _q);

    /**
     * @brief Получить элемент находящийся в заданном индексе
     */
    CharacterModelItem* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Очистить данные модели
     */
    void clear();

    void processSourceModelRowsInserted(const QModelIndex& _parent, int _firstRow, int _lastRow);
    void processSourceModelRowsRemoved(const QModelIndex& _parent, int _firstRow, int _lastRow);
    void processSourceModelDataChanged(const QModelIndex& _index);


    ScreenplayBreakdownStructureCharactersModel* q = nullptr;

    /**
     * @brief Модель сценария, на базе которой строится данная модель
     */
    QPointer<ScreenplayTextModel> model;

    /**
     * @brief Корневой элемент модели
     */
    QScopedPointer<CharacterModelItem> rootItem;

    /**
     * @brief Карта персонажей <элемент персонажа, кол-во сцен>
     */
    QHash<CharacterModelItem*, int> characterItems;
};

ScreenplayBreakdownStructureCharactersModel::Implementation::Implementation(
    ScreenplayBreakdownStructureCharactersModel* _q)
    : q(_q)
    , rootItem(new CharacterModelItem({}))
{
}

CharacterModelItem* ScreenplayBreakdownStructureCharactersModel::Implementation::itemForIndex(
    const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return rootItem.data();
    }

    auto item = static_cast<CharacterModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return rootItem.data();
    }

    return item;
}

void ScreenplayBreakdownStructureCharactersModel::Implementation::clear()
{
    if (!rootItem->hasChildren()) {
        return;
    }

    q->beginResetModel();
    while (rootItem->childCount() > 0) {
        rootItem->removeItem(rootItem->childAt(0));
    }
    q->endResetModel();
}

void ScreenplayBreakdownStructureCharactersModel::Implementation::processSourceModelRowsInserted(
    const QModelIndex& _parent, int _firstRow, int _lastRow)
{
}

void ScreenplayBreakdownStructureCharactersModel::Implementation::processSourceModelRowsRemoved(
    const QModelIndex& _parent, int _firstRow, int _lastRow)
{
}

void ScreenplayBreakdownStructureCharactersModel::Implementation::processSourceModelDataChanged(
    const QModelIndex& _index)
{
}


// ****


ScreenplayBreakdownStructureCharactersModel::ScreenplayBreakdownStructureCharactersModel(
    QObject* _parent)
    : QAbstractItemModel(_parent)
    , d(new Implementation(this))
{
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
        //
        // Считываем данные модели
        //
        QSet<QString> characters;
        QHash<QString, QVector<QModelIndex>> charactersScenes;
        auto saveCharacter = [this, &characters, &charactersScenes](const QString& _name,
                                                                    TextModelTextItem* _textItem) {
            if (_name.simplified().isEmpty()) {
                return;
            }

            const auto sceneIndex = d->model->indexForItem(_textItem).parent();

            const auto iter = characters.find(_name);
            if (iter == characters.end()) {
                characters.insert(_name);
                charactersScenes.insert(_name, { sceneIndex });
                return;
            }

            auto& scenes = charactersScenes[_name];
            if (scenes.constLast() != sceneIndex) {
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
        findCharactersInText(d->model->itemForIndex({}));
        //
        // ... сохраняем собранные данные
        //
        beginResetModel();
        for (const auto& character : characters) {
            auto characterItem = new CharacterModelItem(character);
            const auto characterScenes = charactersScenes[character];
            for (const auto& sceneIndex : characterScenes) {
                characterItem->appendItem(new CharacterModelItem(
                    sceneIndex.data(TextModelGroupItem::GroupNumberRole).toString() + " "
                        + sceneIndex.data().toString(),
                    sceneIndex));
            }

            d->rootItem->appendItem(characterItem);
        }
        endResetModel();

        //
        // Наблюдаем за событиями модели, чтобы обновлять собственные данные
        //
        connect(d->model, &TextModel::modelAboutToBeReset, this, [this] { d->clear(); });
        connect(d->model, &TextModel::modelReset, this, [this] { setSourceModel(d->model); });
        connect(d->model, &TextModel::rowsInserted, this,
                [this](const QModelIndex& _parent, int _first, int _last) {
                    d->processSourceModelRowsInserted(_parent, _first, _last);
                });
        connect(d->model, &TextModel::rowsAboutToBeRemoved, this,
                [this](const QModelIndex& _parent, int _first, int _last) {
                    d->processSourceModelRowsRemoved(_parent, _first, _last);
                });
        connect(d->model, &TextModel::dataChanged, this,
                [this](const QModelIndex& _topLeft, const QModelIndex& _bottomRight) {
                    Q_ASSERT(_topLeft == _bottomRight);
                    d->processSourceModelDataChanged(_topLeft);
                });
    }
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
