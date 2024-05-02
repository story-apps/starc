#include "cards_graphics_view.h"

#include "abstract_card_item.h"
#include "cards_graphics_scene.h"

#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <utils/shugar.h>
#include <utils/tools/once.h>
#include <utils/tools/run_once.h>

#include <QKeyEvent>
#include <QModelIndex>
#include <QPointer>
#include <QScopedValueRollback>
#include <QScreen>
#include <QSet>
#include <QStack>
#include <QTimer>
#include <QVariantAnimation>

#include <cmath>


namespace Ui {

namespace {
const QPoint kInvalidPosition(-1, -1);
}

class CardsGraphicsView::Implementation
{
public:
    Implementation(CardsGraphicsView* _parent, CardsGraphicsScene* _scene);

    /**
     * @brief Обновить параметры карточке
     */
    void updateCardsOptions();

    /**
     * @brief Определение плоского индекса элемента в древовидной модели
     */
    int flatCardIndex(const QModelIndex& _index) const;

    /**
     * @brief Вставить карточку и детей заданного элемента
     */
    AbstractCardItem* insertCard(const QModelIndex& _index, bool _isVisible);

    /**
     * @brief Удалить элемент со сцены и очистить память
     */
    void removeItem(AbstractCardItem* _item);

    /**
     * @brief Уведомить клиентов, если нужно изменить видимость списка проектов
     */
    void notifyVisibleChange();

    /**
     * @brief Определить новую позицию для заданной карточки
     */
    void reorderCard(const QModelIndex& _index);
    void reorderCardInRows(const QModelIndex& _index);
    void reorderCardInColumns(const QModelIndex& _index);
    void reorderCardInHorizontalLines(const QModelIndex& _index);
    void reorderCardInVerticalLines(const QModelIndex& _index);

    /**
     * @brief Упорядочить карточки
     */
    void reorderCards();
    void reorderCardsImpl();
    void reorderCardsInRows();
    void reorderCardsInColumns();
    void reorderCardsInHorizontalLines();
    void reorderCardsInVerticalLines();


    CardsGraphicsView* q = nullptr;

    /**
     * @brief Сцена, на которой отображаются карточки
     */
    CardsGraphicsScene* scene = nullptr;

    /**
     * @brief Параметры отображения карточек
     */
    struct {
        CardsGraphicsViewType type = CardsGraphicsViewType::Rows;
        //
        int sizeScale = 0;
        int ratio = 0;
        int spacingScale = 0;
        int cardsInRow = -1;
        //
        QSizeF size;
        qreal spacing = 0.0;
    } cardsOptions;

    QTimer reorderCardsDebounceTimer;
    QByteArray pendingState;

    QPointer<QAbstractItemModel> model;
    QVector<AbstractCardItem*> cardsItems;
    QHash<void*, AbstractCardItem*> modelItemsToCards;

    struct {
        int row = -1;
        QModelIndex parent;

        bool isValid() const
        {
            return row >= 0;
        }
    } moveTarget;
    QSet<AbstractCardItem*> movedCards;

    struct {
        qreal minimum = 0.0;
        qreal maximum = 0.0;

        bool isValid()
        {
            return qFuzzyCompare(minimum, 0.0) != true || qFuzzyCompare(maximum, 0.0) != true;
        }
    } cardsPositionsInterval;

    bool cardsAnimationsAvailable = true;
    QHash<QGraphicsRectItem*, QPointer<QVariantAnimation>> cardsAnimations;
};

CardsGraphicsView::Implementation::Implementation(CardsGraphicsView* _parent,
                                                  CardsGraphicsScene* _scene)
    : q(_parent)
    , scene(_scene)
{
    reorderCardsDebounceTimer.setSingleShot(true);
    reorderCardsDebounceTimer.setInterval(0);
    QObject::connect(&reorderCardsDebounceTimer, &QTimer::timeout, q, [this] {
        //
        // Принудительно завершаем таймер, чтобы обновилось его значение активности
        //
        reorderCardsDebounceTimer.stop();

        reorderCardsImpl();
    });

    updateCardsOptions();
}

void CardsGraphicsView::Implementation::updateCardsOptions()
{
    //
    // Настроим размер карточки
    //
    const QHash<int, qreal> widthRatios = {
        { 7, 0.9 }, { 8, 0.8 }, { 9, 0.7 }, { 10, 0.6 }, { 11, 0.5 }, { 12, 0.4 },
    };
    const auto cardWidthRatio = widthRatios.value(cardsOptions.ratio, 1.0);
    const QHash<int, qreal> heightRatios = {
        { 0, 0.4 }, { 1, 0.5 }, { 2, 0.6 }, { 3, 0.7 }, { 4, 0.8 }, { 5, 0.9 },
    };
    const auto cardHeightRatio = heightRatios.value(cardsOptions.ratio, 1.0);
    const auto cardWidth = Ui::DesignSystem::layout().px(250) * cardWidthRatio;
    const auto cardHeight = Ui::DesignSystem::layout().px(250) * cardHeightRatio;
    const auto sizeScale = (cardsOptions.sizeScale + 100) / 100.0;
    cardsOptions.size = QSizeF(cardWidth, cardHeight) * sizeScale;

    //
    // Настроим расстояние между карточками
    //
    cardsOptions.spacing = Ui::DesignSystem::layout().px(cardsOptions.spacingScale)
        - Ui::DesignSystem::layout().px24();
}

int CardsGraphicsView::Implementation::flatCardIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return -1;
    }

    //
    // За рутом всегда идёт единичка, так что индекс по-факту будет считаться с нуля
    //
    int flatIndex = -1;
    std::function<bool(const QModelIndex&)> findFlatIndex;
    findFlatIndex = [this, _index, &flatIndex, &findFlatIndex](const QModelIndex& _itemIndex) {
        //
        // Считаем все элементы, которые не исключены
        //
        if (!q->excludeFromFlatIndex(_itemIndex)) {
            ++flatIndex;
        }

        for (int childRow = 0; childRow < model->rowCount(_itemIndex); ++childRow) {
            const auto child = model->index(childRow, 0, _itemIndex);
            if (child == _index) {
                return true;
            }

            if (findFlatIndex(child)) {
                return true;
            }
        }
        return false;
    };
    findFlatIndex({});
    return flatIndex;
}

AbstractCardItem* CardsGraphicsView::Implementation::insertCard(const QModelIndex& _index,
                                                                bool _isVisible)
{
    AbstractCardItem* card = nullptr;

    //
    // Добавляем карточку самого элемента
    //
    const auto cardIter = modelItemsToCards.find(_index.internalPointer());
    //
    // Создаём новую, если такой ещё не было
    //
    if (cardIter == modelItemsToCards.end()) {
        card = q->createCardFor(_index);
        //
        // Если карточку не удалось создать
        //
        if (card == nullptr) {
            //
            // ... но у неё есть дети
            //
            if (model->rowCount(_index) > 0) {
                //
                // ... то пробуем добавить детей
                //
                bool isChildsVisible = true;
                for (int childRow = 0; childRow < model->rowCount(_index); ++childRow) {
                    const auto childItemIndex = model->index(childRow, 0, _index);
                    insertCard(childItemIndex, isChildsVisible);
                }
            }
            return nullptr;
        }

        //
        // Настроим исходное положение
        //
        card->setContainer(modelItemsToCards.value(_index.parent().internalPointer()));
        card->setPos(kInvalidPosition);
        card->setRect({ card->scenePos(), QSizeF{ 1, 1 } });
        scene->addItem(card);
        modelItemsToCards.insert(_index.internalPointer(), card);
    }
    //
    // А если была, то будем лишь корректировать её положение в списке карточек
    //
    else {
        //
        // Определим индекс карточки в списке элементов
        //
        const auto cardItemIndex = cardsItems.indexOf(cardIter.value());
        //
        // Если индекс найден, значит она ещё в списке и нужно её извлечь
        //
        if (cardItemIndex != -1) {
            card = cardsItems.takeAt(cardItemIndex);
            //
            // Извлечём также и всех детей
            //
            std::function<void(AbstractCardItem*)> removeChildren;
            removeChildren = [this, &removeChildren](AbstractCardItem* _card) {
                const auto children = _card->embeddedCards();
                for (auto child : children) {
                    cardsItems.removeAll(child);
                    removeChildren(child);
                }
            };
            removeChildren(card);
        }
        //
        // Если индекс не найден, то значит она уже была удалена из списка и получим её из итератора
        //
        else {
            card = cardIter.value();
        }
    }

    card->setModelItemIndex(_index);

    if (!movedCards.contains(card)) {
        card->setVisible(_isVisible);
    }

    const auto positionToInsert = flatCardIndex(_index);
    cardsItems.insert(positionToInsert, card);

    //
    // Добавляем карточки детей
    //
    bool isChildsVisible = true;
    if (card->isContainer()) {
        isChildsVisible = card->isOpened();
    }
    for (int childRow = 0; childRow < model->rowCount(_index); ++childRow) {
        const auto childItemIndex = model->index(childRow, 0, _index);
        auto child = insertCard(childItemIndex, isChildsVisible);
        if (child != nullptr) {
            child->setContainer(card);
        }
    }

    //
    // Смещаем индексы модели идущих за вставляемой карточкой элементов того же уровня
    //
    for (int row = _index.row(); row < model->rowCount(_index.parent()); ++row) {
        const auto index = model->index(row, 0, _index.parent());
        if (!modelItemsToCards.contains(index.internalPointer())) {
            continue;
        }

        modelItemsToCards[index.internalPointer()]->setModelItemIndex(index);
    }

    return card;
}

void CardsGraphicsView::Implementation::removeItem(AbstractCardItem* _item)
{
    if (_item == nullptr) {
        return;
    }

    if (movedCards.contains(_item)) {
        return;
    }

    if (_item->isContainer()) {
        auto modelItemIndex = _item->modelItemIndex();
        for (int childRow = 0; childRow < model->rowCount(modelItemIndex); ++childRow) {
            auto childItemIndex = model->index(childRow, 0, modelItemIndex);
            removeItem(modelItemsToCards.value(childItemIndex.internalPointer(), nullptr));
        }
    }

    _item->setContainer(nullptr);

    scene->removeItem(_item);
    modelItemsToCards.remove(_item->modelItemIndex().internalPointer());
    cardsItems.removeAll(_item);
    cardsAnimations.remove(_item);
    delete _item;
    _item = nullptr;
}

void CardsGraphicsView::Implementation::notifyVisibleChange()
{
    if (model->rowCount() == 0 && q->isVisible()) {
        emit q->hideRequested();
    } else if (model->rowCount() > 0 && q->isHidden()) {
        emit q->showRequested();
    }
}

void CardsGraphicsView::Implementation::reorderCard(const QModelIndex& _index)
{
    if (scene->isReadOnly()) {
        return;
    }

    if (!_index.isValid()) {
        return;
    }

    //
    // Если сигнал пришёл от карточки, которую не таскают в данный момент, то просто упорядочим
    // карточки на экране - это событие отпускания последней двигаемой карточки
    //
    if (modelItemsToCards.value(_index.internalPointer()) != scene->mouseGrabberItem()) {
        reorderCards();
        return;
    }

    //
    // Собственно определим новое положение
    //
    switch (cardsOptions.type) {
    case CardsGraphicsViewType::Rows: {
        reorderCardInRows(_index);
        break;
    }

    case CardsGraphicsViewType::Columns: {
        reorderCardInColumns(_index);
        break;
    }

    case CardsGraphicsViewType::HorizontalLines: {
        reorderCardInHorizontalLines(_index);
        break;
    }

    case CardsGraphicsViewType::VerticalLines: {
        reorderCardInVerticalLines(_index);
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }

    //
    // И упорядочим остальные карточки
    //
    reorderCards();
}

void CardsGraphicsView::Implementation::reorderCardInRows(const QModelIndex& _index)
{
    auto movedCard = modelItemsToCards.value(_index.internalPointer());
    if (movedCard == nullptr) {
        return;
    }

    //
    // Вспомогательный метод, чтобы определить область карточки, относительно сцены
    //
    auto niceCardRect = [this](AbstractCardItem* _card) {
        if (_card == nullptr) {
            return QRectF();
        }

        const auto cardAnimation = cardsAnimations[_card];
        const QPointF cardPosition
            = cardAnimation.isNull() ? _card->pos() : cardAnimation->endValue().toPointF();
        return QRectF(cardPosition, _card->boundingRect().size())
            .marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    };

    //
    // Определим место, куда перемещена карточка
    //
    const QRectF movedCardRect = niceCardRect(movedCard);
    const qreal movedCardLeft = movedCardRect.x();
    const qreal movedCardRight = movedCardRect.right();
    const qreal movedCardTop = movedCardRect.y();
    const qreal movedCardCenterY = movedCardRect.center().y();

    //
    // Ищем элемент, который будет последним перед карточкой в её новом месте
    //
    AbstractCardItem* containerCard = nullptr;
    AbstractCardItem* previousCard = nullptr;
    bool skipPreviousCardChildren = false;
    bool needToClearInsertionState = false;
    for (auto targetCard : std::as_const(cardsItems)) {
        //
        // Пропускаем саму двигаемую карточку, невидимые и вложенные карточки
        //
        if (!targetCard->isVisible() || targetCard == movedCard
            || movedCard->isContainerOf(targetCard)) {
            continue;
        }

        //
        // Снимаем признак вложения с карточек, т.к. текущая карточка двигается
        //
        targetCard->setInsertionState(AbstractCardItem::InsertionState::Empty);

        //
        // Если осталось только снять статус вставки, то дальше не идём
        //
        if (needToClearInsertionState) {
            continue;
        }

        //
        // Если нужно пропускать детей вложенных в previousCard, то пропутим детей, или снимем флаг
        //
        if (skipPreviousCardChildren) {
            if (previousCard->isContainerOf(targetCard)) {
                continue;
            } else {
                skipPreviousCardChildren = false;
            }
        }

        //
        // Определим параметры проверяемой карточки, чтобы сравнить с двигаемой
        //
        const QRectF cardRect = niceCardRect(targetCard);
        const qreal cardLeft = cardRect.x();
        const qreal cardRight = cardRect.right();
        const qreal cardTop = cardRect.y();
        const qreal cardBottom = cardRect.bottom();

        if ( // на разных линиях
            movedCardTop > cardBottom
            // для письменности слева-направо
            || (QLocale().textDirection() == Qt::LeftToRight
                && (
                    // на одной линии, но левее
                    (movedCardTop < cardBottom && movedCardTop > cardTop
                     && movedCardLeft > cardLeft)
                    || (movedCardCenterY < cardBottom && movedCardCenterY > cardTop
                        && movedCardLeft > cardLeft)
                    // за пределами
                    || (movedCardTop > cardTop && movedCardLeft > cardRight)
                    // ... в рамках контейнера
                    || (targetCard->isContainer() && movedCardLeft > cardLeft
                        && movedCardLeft < cardRight
                        && ((movedCardTop > cardTop && movedCardTop < cardBottom)
                            || (movedCardCenterY > cardTop && movedCardCenterY < cardBottom)))))
            // для письменности справа-налево
            || (QLocale().textDirection() == Qt::RightToLeft
                && (
                    // на одной линии, но правее
                    (movedCardCenterY < cardBottom && movedCardCenterY > cardTop
                     && movedCardRight < cardRight)
                    // за пределами
                    || (movedCardTop > cardTop && movedCardRight < cardLeft)
                    // ... в рамках контейнера
                    || (targetCard->isContainer() && movedCardRight < cardRight
                        && movedCardRight > cardLeft
                        && ((movedCardTop > cardTop && movedCardTop < cardBottom)
                            || (movedCardCenterY > cardTop && movedCardCenterY < cardBottom)))))) {
            //
            // Если перемещаемая карточка находится за пределами контейнера, а текущая проверяемая
            // карточка внутри контейнера, то пропускаем её, оставив предыдущей карточкой контейнер
            //
            if (previousCard != nullptr && previousCard->isContainer() && containerCard == nullptr
                && niceCardRect(previousCard).contains(cardRect)) {
                continue;
            }

            //
            // Карточка вкладывается в последний контейнер, область которого она пересекает
            //
            if (targetCard->isContainer() && movedCard->canBeEmbedded(targetCard)
                && movedCardRect.intersects(cardRect)) {
                containerCard = targetCard;
            }
            previousCard = targetCard;

            //
            // Если контейнер целиком располагается над перемещаемой карточкой,
            // то не нужно проверять его детей, а просто пропускаем их
            //
            if (previousCard->isContainer() && movedCardTop > cardBottom) {
                skipPreviousCardChildren = true;
            }
        }
        //
        // Если не после данного элемента
        //
        else {
            //
            // ... если текущая проверяемая карточка не стоит перед перемещаемой, но она вложена в
            // контейнер, в который может быть вложена перемещаемая, идём дальше, пока не дойдём до
            // конца контейнера
            //
            if (containerCard != nullptr && containerCard->isContainerOf(targetCard)) {
                continue;
            }
            //
            // ... либо прерываем поиск и только снимаем статус вставки с оставшихся карточек
            //
            needToClearInsertionState = true;
            continue;
        }
    }

    //
    // Проверяем, а можно ли вообще карточку в эту позицию расположить
    //
    if (
        // нельзя вложить в контейнер
        (containerCard != nullptr && !movedCard->canBeEmbedded(containerCard))
        // нельзя поместить в начало
        || (containerCard == nullptr && previousCard == nullptr
            && !movedCard->canBePlacedAfter(previousCard))
        // нельзя поместить в корень после предыдущей карточки
        || (containerCard == nullptr && !movedCard->canBePlacedAfter(previousCard))) {
        return;
    }

    //
    // Определим позицию вставки
    //
    // ... поместить в самое начало
    //
    if (containerCard == nullptr && previousCard == nullptr) {
        moveTarget = { 0, {} };
    }
    //
    // ... поместить после previousCard внутрь того же родителя
    //
    else if (containerCard == nullptr) {
        moveTarget
            = { previousCard->modelItemIndex().row() + 1, previousCard->modelItemIndex().parent() };
    }
    //
    // ... поместить внутрь containerCard
    //
    else {
        //
        // ... в начало
        //
        if ((previousCard == containerCard && containerCard->isOpened())) {
            moveTarget = { 1, containerCard->modelItemIndex() };
        }
        //
        // ... после previousCard
        //
        else if (previousCard->modelItemIndex().parent() == containerCard->modelItemIndex()) {
            moveTarget
                = { previousCard->modelItemIndex().row() + 1, containerCard->modelItemIndex() };
        }
        //
        // ... в конец (но перед закрывающим папку блоком)
        //
        else {
            moveTarget = { model->rowCount(containerCard->modelItemIndex()) - 2,
                           containerCard->modelItemIndex() };
        }
    }

    //
    // Подсветим контейнер, в который будет вставлена карточка
    //
    if (containerCard != nullptr) {
        containerCard->setInsertionState(AbstractCardItem::InsertionState::InsertInside);
    }

    //
    // Если карточка остаётся на месте, ничего не делаем
    //
    if (moveTarget.row == _index.row() && moveTarget.parent == _index.parent()) {
        moveTarget = {};
        return;
    }

    //
    // А если карточка перемещается в другое место, то установим статусы вставки для соседних
    //
    if (previousCard != nullptr && previousCard != containerCard) {
        previousCard->setInsertionState(AbstractCardItem::InsertionState::InsertAfter);
        previousCard->updateEmbeddedCardsPositions();
    }
    //
    // .. и найдём карточку, следующую за местом вставки
    //
    AbstractCardItem* nextCard = nullptr;
    if (previousCard == nullptr) {
        for (auto card : std::as_const(cardsItems)) {
            if (card != movedCard) {
                nextCard = card;
                break;
            }
        }
    } else {
        const auto previousCardIndex = previousCard->modelItemIndex();
        const auto searchParent
            = previousCard == containerCard ? previousCardIndex : previousCardIndex.parent();
        const auto searchStartFromRow
            = previousCard == containerCard ? 1 : previousCardIndex.row() + 1;
        for (int row = searchStartFromRow; row < model->rowCount(searchParent); ++row) {
            const auto nextCardIndex = model->index(row, 0, searchParent);
            if (nextCardIndex == _index) {
                continue;
            }

            nextCard = modelItemsToCards.value(nextCardIndex.internalPointer());
            if (nextCard == nullptr) {
                continue;
            }

            break;
        }
    }
    //
    if (nextCard != nullptr) {
        nextCard->setInsertionState(AbstractCardItem::InsertionState::InsertBefore);
        nextCard->updateEmbeddedCardsPositions();
    }
}

void CardsGraphicsView::Implementation::reorderCardInColumns(const QModelIndex& _index)
{
    auto movedCard = modelItemsToCards.value(_index.internalPointer());
    if (movedCard == nullptr) {
        return;
    }

    //
    // Вспомогательный метод, чтобы определить область карточки, относительно сцены
    //
    auto niceCardRect = [this](AbstractCardItem* _card) {
        if (_card == nullptr) {
            return QRectF();
        }

        const auto cardAnimation = cardsAnimations[_card];
        const QPointF cardPosition
            = cardAnimation.isNull() ? _card->pos() : cardAnimation->endValue().toPointF();
        return QRectF(cardPosition, _card->boundingRect().size())
            .marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    };

    //
    // Определим место, куда перемещена карточка
    //
    const QRectF movedCardRect = niceCardRect(movedCard);
    const qreal movedCardLeft = movedCardRect.x();
    const auto movedCardCenterX = movedCardRect.center().x();
    const qreal movedCardRight = movedCardRect.right();
    const qreal movedCardTop = movedCardRect.y();
    const auto movedCardCenterY = movedCardRect.center().y();

    //
    // Ищем элемент, который будет последним перед карточкой в её новом месте
    //
    AbstractCardItem* containerCard = nullptr;
    AbstractCardItem* previousCard = nullptr;
    bool skipPreviousCardChildren = false;
    bool needToClearInsertionState = false;
    for (auto targetCard : std::as_const(cardsItems)) {
        //
        // Пропускаем саму двигаемую карточку, невидимые и вложенные карточки
        //
        if (!targetCard->isVisible() || targetCard == movedCard
            || movedCard->isContainerOf(targetCard)) {
            continue;
        }

        //
        // Снимаем признак вложения с карточек, т.к. текущая карточка двигается
        //
        targetCard->setInsertionState(AbstractCardItem::InsertionState::Empty);

        //
        // Если осталось только снять статус вставки, то дальше не идём
        //
        if (needToClearInsertionState) {
            continue;
        }

        //
        // Если нужно пропускать детей вложенных в previousCard, то пропутим детей, или снимем флаг
        //
        if (skipPreviousCardChildren) {
            if (previousCard->isContainerOf(targetCard)) {
                continue;
            } else {
                skipPreviousCardChildren = false;
            }
        }

        //
        // Определим параметры проверяемой карточки, чтобы сравнить с двигаемой
        //
        const QRectF cardRect = niceCardRect(targetCard);
        const qreal cardLeft = cardRect.x();
        const qreal cardRight = cardRect.right();
        const qreal cardTop = cardRect.y();
        const qreal cardBottom = cardRect.bottom();

        if ( // для письменности слева-направо
            (QLocale().textDirection() == Qt::LeftToRight
             && (
                 // ... в следующей колонке
                 movedCardLeft > cardRight
                 // ... ниже и середина перемещаемой правее
                 || (movedCardTop > cardBottom && movedCardCenterX > cardLeft)
                 // ... в рамках контейнера
                 || (targetCard->isContainer() && movedCardTop > cardTop
                     && movedCardTop < cardBottom
                     && ((movedCardLeft > cardLeft && movedCardLeft < cardRight)
                         || (movedCardCenterX > cardLeft && movedCardCenterX < cardRight)))))
            // для письменности справа-налево
            || (QLocale().textDirection() == Qt::RightToLeft
                && (
                    // ... в следующей колонке
                    movedCardRight < cardLeft
                    // ... ниже и середина перемещаемой левее
                    || (movedCardTop > cardBottom && movedCardCenterX < cardRight)
                    // ... в рамках контейнера
                    || (targetCard->isContainer() && movedCardTop > cardTop
                        && movedCardTop < cardBottom
                        && ((movedCardRight < cardRight && movedCardRight > cardLeft)
                            || (movedCardCenterX > cardLeft && movedCardCenterX < cardRight)))))
            // в той же колонке, но ниже
            || (containerCard != nullptr && containerCard->isContainerOf(targetCard)
                && movedCardTop > cardTop && (movedCardCenterY > cardTop))) {
            //
            // Если перемещаемая карточка находится за пределами контейнера, а текущая проверяемая
            // карточка внутри контейнера, то пропускаем её, оставив предыдущей карточкой контейнер
            //
            if (previousCard != nullptr && previousCard->isContainer() && containerCard == nullptr
                && niceCardRect(previousCard).contains(cardRect)) {
                continue;
            }

            //
            // Карточка вкладывается в последний контейнер, область которого она пересекает
            //
            if (targetCard->isContainer() && movedCard->canBeEmbedded(targetCard)
                && movedCardRect.intersects(cardRect)) {
                containerCard = targetCard;
            }
            previousCard = targetCard;

            //
            // Если контейнер целиком располагается над перемещаемой карточкой,
            // то не нужно проверять его детей, а просто пропускаем их
            //
            if (previousCard->isContainer() && movedCardTop > cardBottom) {
                skipPreviousCardChildren = true;
            }
        }
        //
        // Если не после данного элемента
        //
        else {
            //
            // ... если текущая проверяемая карточка не стоит перед перемещаемой, но она вложена в
            // контейнер, в который может быть вложена перемещаемая, идём дальше, пока не дойдём до
            // конца контейнера
            //
            if (containerCard != nullptr && containerCard->isContainerOf(targetCard)) {
                continue;
            }
            //
            // ... либо прерываем поиск и только снимаем статус вставки с оставшихся карточек
            //
            needToClearInsertionState = true;
            continue;
        }
    }

    //
    // Проверяем, а можно ли вообще карточку в эту позицию расположить
    //
    if (
        // нельзя вложить в контейнер
        (containerCard != nullptr && !movedCard->canBeEmbedded(containerCard))
        // нельзя поместить в начало
        || (containerCard == nullptr && previousCard == nullptr
            && !movedCard->canBePlacedAfter(previousCard))
        // нельзя поместить в корень после предыдущей карточки
        || (containerCard == nullptr && !movedCard->canBePlacedAfter(previousCard))) {
        return;
    }

    //
    // Определим позицию вставки
    //
    // ... поместить в самое начало
    //
    if (containerCard == nullptr && previousCard == nullptr) {
        moveTarget = { 0, {} };
    }
    //
    // ... поместить после previousCard внутрь того же родителя
    //
    else if (containerCard == nullptr) {
        moveTarget
            = { previousCard->modelItemIndex().row() + 1, previousCard->modelItemIndex().parent() };
    }
    //
    // ... поместить внутрь containerCard
    //
    else {
        //
        // ... в начало
        //
        if ((previousCard == containerCard && containerCard->isOpened())) {
            moveTarget = { 1, containerCard->modelItemIndex() };
        }
        //
        // ... после previousCard
        //
        else if (previousCard->modelItemIndex().parent() == containerCard->modelItemIndex()) {
            moveTarget
                = { previousCard->modelItemIndex().row() + 1, containerCard->modelItemIndex() };
        }
        //
        // ... в конец (но перед закрывающим папку блоком)
        //
        else {
            moveTarget = { model->rowCount(containerCard->modelItemIndex()) - 2,
                           containerCard->modelItemIndex() };
        }
    }

    //
    // Подсветим контейнер, в который будет вставлена карточка
    //
    if (containerCard != nullptr) {
        containerCard->setInsertionState(AbstractCardItem::InsertionState::InsertInside);
    }

    //
    // Если карточка остаётся на месте, ничего не делаем
    //
    if (moveTarget.row == _index.row() && moveTarget.parent == _index.parent()) {
        return;
    }

    //
    // А если карточка перемещается в другое место, то установим статусы вставки для соседних
    //
    if (previousCard != nullptr && previousCard != containerCard) {
        previousCard->setInsertionState(AbstractCardItem::InsertionState::InsertAfter);
        previousCard->updateEmbeddedCardsPositions();
    }
    //
    // .. и найдём карточку, следующую за местом вставки
    //
    AbstractCardItem* nextCard = nullptr;
    if (previousCard == nullptr) {
        for (auto card : std::as_const(cardsItems)) {
            if (card != movedCard) {
                nextCard = card;
                break;
            }
        }
    } else {
        const auto previousCardIndex = previousCard->modelItemIndex();
        const auto searchParent
            = previousCard == containerCard ? previousCardIndex : previousCardIndex.parent();
        const auto searchStartFromRow
            = previousCard == containerCard ? 1 : previousCardIndex.row() + 1;
        for (int row = searchStartFromRow; row < model->rowCount(searchParent); ++row) {
            const auto nextCardIndex = model->index(row, 0, searchParent);
            if (nextCardIndex == _index) {
                continue;
            }

            nextCard = modelItemsToCards.value(nextCardIndex.internalPointer());
            if (nextCard == nullptr) {
                continue;
            }

            break;
        }
    }
    //
    if (nextCard != nullptr) {
        nextCard->setInsertionState(AbstractCardItem::InsertionState::InsertBefore);
        nextCard->updateEmbeddedCardsPositions();
    }
}

void CardsGraphicsView::Implementation::reorderCardInHorizontalLines(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    auto movedCard = modelItemsToCards.value(_index.internalPointer());
    if (movedCard == nullptr || !scene->mouseGrabberItems().contains(movedCard)) {
        return;
    }

    //
    // Ставим здесь проверку, чтобы функция не вызывалась рекурсивно, т.к. изменение элемента
    // модели приводит к запуску алгоритма перераспределения карточки на холсте
    //
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    const auto positionsInterval = q->cardsPositionsInterval();
    const auto pxDistance
        = (positionsInterval.second - positionsInterval.first) / static_cast<qreal>(q->width());
    const auto movedCardX = q->mapFromScene(movedCard->scenePos()).x();
    const auto position = positionsInterval.first + movedCardX * pxDistance;
    movedCard->setPositionOnLine(position);
    emit model->dataChanged(_index, _index);

    //
    // Перерисуем сцену, чтобы не оставалось артифактов от перетаскивания карточек
    //
    q->updateScene({ scene->sceneRect() });
}

void CardsGraphicsView::Implementation::reorderCardInVerticalLines(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    auto movedCard = modelItemsToCards.value(_index.internalPointer());
    if (movedCard == nullptr || !scene->mouseGrabberItems().contains(movedCard)) {
        return;
    }

    //
    // Ставим здесь проверку, чтобы функция не вызывалась рекурсивно, т.к. изменение элемента
    // модели приводит к запуску алгоритма перераспределения карточки на холсте
    //
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    const auto positionsInterval = q->cardsPositionsInterval();
    const auto pxDistance
        = (positionsInterval.second - positionsInterval.first) / static_cast<qreal>(q->height());
    const auto movedCardY = q->mapFromScene(movedCard->scenePos()).y();
    const auto position = positionsInterval.first + movedCardY * pxDistance;
    movedCard->setPositionOnLine(position);
    emit model->dataChanged(_index, _index);

    //
    // Перерисуем сцену, чтобы не оставалось артифактов от перетаскивания карточек
    //
    q->updateScene({ scene->sceneRect() });
}

void CardsGraphicsView::Implementation::reorderCards()
{
    reorderCardsDebounceTimer.start();
}

void CardsGraphicsView::Implementation::reorderCardsImpl()
{
    switch (cardsOptions.type) {
    case CardsGraphicsViewType::Rows: {
        reorderCardsInRows();
        break;
    }

    case CardsGraphicsViewType::Columns: {
        reorderCardsInColumns();
        break;
    }

    case CardsGraphicsViewType::HorizontalLines: {
        reorderCardsInHorizontalLines();
        break;
    }

    case CardsGraphicsViewType::VerticalLines: {
        reorderCardsInVerticalLines();
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }

    //
    // Если необходимо, восстановим состояние вьюхи
    //
    if (!pendingState.isEmpty() && !reorderCardsDebounceTimer.isActive()) {
        q->restoreState(pendingState);
        pendingState.clear();
    }
}

void CardsGraphicsView::Implementation::reorderCardsInRows()
{
    QSignalBlocker signalBlocker(scene);

    //
    // Определим размер окна
    //
    const auto viewRect = [this] {
        if (scene->views().isEmpty()) {
            return QRectF(0, 0, 100, 100);
        }

        const auto view = scene->views().constFirst();
        const auto viewTopLeftPoint = view->mapToScene(QPoint(0, 0));
        const int scrollDelta = Ui::DesignSystem::scrollBar().maximumSize();
        const auto viewBottomRightPoint
            = view->mapToScene(QPoint(view->width() - scrollDelta, view->height() - scrollDelta));
        return QRectF(viewTopLeftPoint, viewBottomRightPoint);
    }();

    //
    // Определим количество карточек в ряду
    //
    const int cardsInRowCount = [this, width = viewRect.width()]() mutable {
        if (cardsOptions.cardsInRow != -1) {
            return cardsOptions.cardsInRow;
        }

        int count = 0;
        width -= Ui::DesignSystem::projectCard().margins().left();
        width -= Ui::DesignSystem::projectCard().margins().right();
        forever
        {
            width -= count > 0 ? cardsOptions.spacing : 0;
            width -= cardsOptions.size.width();

            if (width > 0) {
                ++count;
            } else {
                break;
            }
        }
        return std::max(1, count);
    }();

    //
    // Определим видимую область экрана
    //
    const QRectF viewportRect = [this] {
        if (scene->views().isEmpty()) {
            return QRectF();
        }

        const QGraphicsView* view = scene->views().constFirst();
        return view->mapToScene(view->viewport()->geometry()).boundingRect();
    }();

    //
    // Определим ширину экрана и расположение первой карточки в колонке
    //
    const qreal sceneRectWidth = std::max(Ui::DesignSystem::projectCard().margins().left()
                                              + cardsOptions.size.width() * cardsInRowCount
                                              + cardsOptions.spacing * (cardsInRowCount - 1)
                                              + Ui::DesignSystem::projectCard().margins().right(),
                                          viewRect.width());
    const auto isLeftToRight = q->isLeftToRight();
    const qreal firstCardInRowX = [this, isLeftToRight, sceneRectWidth]() {
        return isLeftToRight ? Ui::DesignSystem::projectCard().margins().left()
                             : sceneRectWidth - Ui::DesignSystem::projectCard().margins().right()
                - cardsOptions.size.width();
    }();

    //
    // Метод определения отступа для карточки, если рядом с ней будет происходить вставка
    //
    const auto findXInsertStateDelta = [isLeftToRight](AbstractCardItem* _card) {
        auto multiplier = 0.0;
        if ((isLeftToRight
             && _card->insertionState() == AbstractCardItem::InsertionState::InsertAfter)
            || (!isLeftToRight
                && _card->insertionState() == AbstractCardItem::InsertionState::InsertBefore)) {
            multiplier = -1.0;
        } else if ((isLeftToRight
                    && _card->insertionState() == AbstractCardItem::InsertionState::InsertBefore)
                   || (!isLeftToRight
                       && _card->insertionState()
                           == AbstractCardItem::InsertionState::InsertAfter)) {
            multiplier = 1.0;
        }
        return Ui::DesignSystem::layout().px(6) * multiplier;
    };

    //
    // Проходим все элементы (они упорядочены так, как должны идти элементы в сценарии
    //
    qreal x = firstCardInRowX;
    qreal xInsertStateDelta = 0.0;
    qreal y = Ui::DesignSystem::projectCard().margins().top();
    qreal z = scene->firstZValue();
    qreal maxY = 0.0;
    qreal lastItemHeight = 0.0;
    int currentCardInRow = 0;
    QStack<QModelIndex> containersStack;
    for (auto card : std::as_const(cardsItems)) {
        //
        // Пропускаем невидимые карточки
        //
        if (card == nullptr || !card->isVisible()
            || (card->container() != nullptr && !card->container()->isOpened())) {
            continue;
        }

        //
        // Если закончили вставлять карточки в родителя
        //
        while (!containersStack.isEmpty()
               && containersStack.top() != card->modelItemIndex().parent()) {
            //
            // Уберём его из списка родителей
            //
            auto containerIndex = containersStack.pop();
            auto containerCard = modelItemsToCards[containerIndex.internalPointer()];
            //
            // ... если он не перемещается, то скорректируем его размер по последнему из детей
            //
            if (!movedCards.contains(containerCard)) {
                auto containerRect = containerCard->rect();
                auto containerY = containerCard->pos().y();
                QPointer<QVariantAnimation>& moveAnimation = cardsAnimations[containerCard];
                if (!moveAnimation.isNull()
                    && moveAnimation->state() == QVariantAnimation::Running) {
                    containerY = moveAnimation->endValue().toPointF().y();
                }
                containerRect.setBottom(y - containerY + lastItemHeight
                                        + Ui::DesignSystem::layout().px12());
                containerCard->setRect(containerRect);
            }
            //
            // ... корректируем координаты для дальнейшей компоновки
            //
            xInsertStateDelta -= findXInsertStateDelta(containerCard);
            y += Ui::DesignSystem::layout().px12();
            currentCardInRow = cardsInRowCount;
        }

        //
        // Если карточка рядом со смещаемой, добавляем дополнительное смещение по y
        //
        xInsertStateDelta += findXInsertStateDelta(card);

        //
        // Настраиваем размер карточки в зависимости от того развёрнута ли она на весь экран
        //
        bool isFullWidth = false;
        bool hasChildren = false;
        qreal xDelta = 0.0; // выступ акта, или папки слева от карточек
        qreal widthDelta = 0.0; // дополнительная ширина справа
        if (card->isContainer() && card->isTopLevel()) {
            isFullWidth = true;
            hasChildren = card->childCount() > 0;
            xDelta = isLeftToRight ? Ui::DesignSystem::layout().px(48)
                                   : Ui::DesignSystem::layout().px24();
            widthDelta = isLeftToRight ? Ui::DesignSystem::layout().px24()
                                       : Ui::DesignSystem::layout().px(48);
            containersStack.push(card->modelItemIndex());
        } else if (card->isContainer() && !card->isTopLevel()) {
            isFullWidth = card->isOpened();
            if (isFullWidth) {
                hasChildren = card->childCount() > 0;
                xDelta = Ui::DesignSystem::layout().px12();
                widthDelta = Ui::DesignSystem::layout().px12();
                containersStack.push(card->modelItemIndex());
            }
        }
        //
        // ... собственно корректируем размер карточки, если её сейчас не таскают конечно же
        //
        if (!movedCards.contains(card)) {
            auto cardRect = card->rect();
            if (isFullWidth) {
                //
                // ... для карточек расположенных на всю ширину учитываем смещение влево, а также
                //     берём на один спейсинг больше, чтобы у карточек в группах был отступ справа
                //
                cardRect.setWidth(cardsInRowCount * cardsOptions.size.width()
                                  + (cardsInRowCount - 1) * cardsOptions.spacing + xDelta
                                  + widthDelta);
                cardRect.setHeight(cardsOptions.size.height());
            } else {
                cardRect.setSize(cardsOptions.size);
            }
            card->setRect(cardRect);
        }

        //
        // Определим положение карточки
        //
        if (isFullWidth || currentCardInRow == cardsInRowCount
            || x + card->boundingRect().width() > sceneRectWidth) {
            if (isFullWidth) {
                currentCardInRow = cardsInRowCount - 1;
            } else {
                currentCardInRow = 0;
            }
            x = firstCardInRowX - xDelta;
            y += lastItemHeight + cardsOptions.spacing;
        }
        //
        // ... позиционируем карточку, если она не перемещается в данный момент
        //     и новая позиция отличается от текущей
        //
        const auto xCorrected = (!isLeftToRight && isFullWidth)
            ? (x + cardsOptions.size.width() + xDelta + widthDelta - card->boundingRect().width())
            : x;
        const QPointF newItemPosition(xCorrected + xInsertStateDelta, y);
        if (!movedCards.contains(card) && card->pos() != newItemPosition) {
            const QRectF itemRect(card->pos(), card->boundingRect().size());
            const QRectF newItemRect(newItemPosition, card->boundingRect().size());
            if (cardsAnimationsAvailable && scene->isActive()
                && viewportRect.intersects(itemRect.united(newItemRect))
                // карточки, которые были вставлены по ходу пьесы, нужно сразу на место ставить,
                // чтобы они не вылетали из-за угла
                && card->pos() != kInvalidPosition) {
                //
                // ... анимируем смещение
                //
                QPointer<QVariantAnimation>& moveAnimation = cardsAnimations[card];
                if (moveAnimation.isNull()) {
                    moveAnimation = new QVariantAnimation(scene);
                    moveAnimation->setDuration(220);
                    moveAnimation->setEasingCurve(QEasingCurve::OutQuad);
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::valueChanged, scene,
                                     [this, card](const QVariant& _value) {
                                         QSignalBlocker signalBlocker(scene);
                                         card->setPos(_value.toPointF());
                                     });
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::finished, q,
                                     [this] { scene->fitToContents(); });
                }
                //
                // ... при необходимости анимируем положение карточки
                //
                if (moveAnimation->endValue().toPointF() != newItemPosition) {
                    if (moveAnimation->state() == QVariantAnimation::Running) {
                        moveAnimation->pause();
                    } else {
                        moveAnimation->setStartValue(card->pos());
                    }
                    moveAnimation->setEndValue(newItemPosition);
                    if (moveAnimation->state() == QVariantAnimation::Paused) {
                        moveAnimation->resume();
                    } else {
                        moveAnimation->start(QAbstractAnimation::DeleteWhenStopped);
                    }
                }
            } else {
                card->setPos(newItemPosition);
            }
        }
        //
        // ... расположим карточки, чтобы никто не пропадал под родителем
        //
        card->setZValue((movedCards.contains(card)) ? z + cardsItems.size() : z);
        //
        // ... и корректируем координаты для позиционирования следующих элементов
        //
        x += (isLeftToRight ? 1 : -1) * (card->boundingRect().width() + cardsOptions.spacing);
        if (isFullWidth && hasChildren) {
            lastItemHeight = card->headerHeight() - cardsOptions.spacing;
        } else {
            lastItemHeight = card->boundingRect().height();
        }

        if (maxY < y) {
            maxY = y;
        }

        //
        // ... смещение от вставки корректируем только для карточек, для контейнеров оно будет
        //     закрыто, когда будет закрыт сам контейнер
        //
        if (!card->isContainer()) {
            xInsertStateDelta -= findXInsertStateDelta(card);
        }

        z = scene->nextZValue();

        ++currentCardInRow;
    }
    //
    // Закрываем последнюю открытую папку, если есть
    //
    while (!containersStack.isEmpty()) {
        //
        // Уберём его из списка родителей
        //
        auto containerItem = containersStack.pop();
        auto containerCard = modelItemsToCards[containerItem.internalPointer()];
        //
        // ... если он не перемещается, то скорректируем его размер по последнему из детей
        //
        if (!movedCards.contains(containerCard)) {
            auto folderRect = containerCard->rect();
            auto folderY = containerCard->pos().y();
            QPointer<QVariantAnimation>& moveAnimation = cardsAnimations[containerCard];
            if (!moveAnimation.isNull() && moveAnimation->state() == QVariantAnimation::Running) {
                folderY = moveAnimation->endValue().toPointF().y();
            }
            folderRect.setBottom(y - folderY + lastItemHeight + Ui::DesignSystem::layout().px12());
            containerCard->setRect(folderRect);
        }
        //
        // ... корректируем координаты для дальнейшей компоновки
        //
        y += Ui::DesignSystem::layout().px12();
    }

    //
    // Корректируем размер, чтобы все карточки персонажей были видны
    //
    scene->fitToContents();
}

void CardsGraphicsView::Implementation::reorderCardsInColumns()
{
    QSignalBlocker signalBlocker(scene);

    //
    // Определим размер окна
    //
    const auto viewRect = [this] {
        if (scene->views().isEmpty()) {
            return QRectF(0, 0, 100, 100);
        }

        const auto view = scene->views().constFirst();
        const auto viewTopLeftPoint = view->mapToScene(QPoint(0, 0));
        const int scrollDelta = Ui::DesignSystem::scrollBar().maximumSize();
        const auto viewBottomRightPoint
            = view->mapToScene(QPoint(view->width() - scrollDelta, view->height() - scrollDelta));
        return QRectF(viewTopLeftPoint, viewBottomRightPoint);
    }();

    //
    // Определим видимую область экрана
    //
    const QRectF viewportRect = [this] {
        if (scene->views().isEmpty()) {
            return QRectF();
        }

        const QGraphicsView* view = scene->views().constFirst();
        return view->mapToScene(view->viewport()->geometry()).boundingRect();
    }();

    //
    // Определим ширину экрана и расположение первой карточки в колонке
    //
    const qreal sceneRectWidth
        = std::max(Ui::DesignSystem::projectCard().margins().left() + cardsOptions.size.width()
                       + Ui::DesignSystem::projectCard().margins().right(),
                   viewRect.width());
    const auto isLeftToRight = q->isLeftToRight();
    qreal firstCardInColumnX = [this, isLeftToRight, sceneRectWidth]() {
        return isLeftToRight ? Ui::DesignSystem::projectCard().margins().left()
                             : sceneRectWidth - Ui::DesignSystem::projectCard().margins().right()
                - cardsOptions.size.width();
    }();

    //
    // Метод определения отступа для карточки, если рядом с ней будет происходить вставка
    //
    const auto findYInsertStateDelta = [](AbstractCardItem* _card) {
        if (_card->insertionState() == AbstractCardItem::InsertionState::InsertAfter) {
            return -Ui::DesignSystem::layout().px(6);
        } else if (_card->insertionState() == AbstractCardItem::InsertionState::InsertBefore) {
            return Ui::DesignSystem::layout().px(6);
        } else {
            return 0.0;
        }
    };

    //
    // Проходим все элементы (они упорядочены так, как должны идти элементы в сценарии
    //
    qreal y = Ui::DesignSystem::projectCard().margins().top();
    qreal yInsertStateDelta = 0.0;
    qreal z = scene->firstZValue();
    qreal lastItemHeight = 0.0;
    qreal lastItemWidth = 0.0;
    QStack<QModelIndex> containersStack;
    for (auto card : std::as_const(cardsItems)) {
        //
        // Пропускаем невидимые карточки
        //
        if (card == nullptr || !card->isVisible()
            || (card->container() != nullptr && !card->container()->isOpened())) {
            continue;
        }

        //
        // Если закончили вставлять карточки в родителя
        //
        bool isActClosed = false;
        while (!containersStack.isEmpty()
               && containersStack.top() != card->modelItemIndex().parent()) {
            //
            // Уберём его из списка родителей
            //
            auto containerIndex = containersStack.pop();
            auto containerCard = modelItemsToCards[containerIndex.internalPointer()];
            //
            // ... если он не перемещается, то скорректируем его размер по последнему из детей
            //
            if (!movedCards.contains(containerCard)) {
                auto containerRect = containerCard->rect();
                auto containerY = containerCard->pos().y();
                QPointer<QVariantAnimation>& moveAnimation = cardsAnimations[containerCard];
                if (!moveAnimation.isNull()
                    && moveAnimation->state() == QVariantAnimation::Running) {
                    containerY = moveAnimation->endValue().toPointF().y();
                }
                containerRect.setHeight(y + yInsertStateDelta - cardsOptions.spacing - containerY
                                        + Ui::DesignSystem::layout().px12());
                containerCard->setRect(containerRect);
            }
            //
            // ... корректируем координаты для дальнейшей компоновки
            //
            y += Ui::DesignSystem::layout().px12();
            yInsertStateDelta -= findYInsertStateDelta(containerCard);
            lastItemWidth = containerCard->boundingRect().width();

            //
            // ... если вышли из акта, смещаем все остальные карточки в следующую колонку
            //
            if (containerCard->isTopLevel()) {
                isActClosed = true;
            }
        }

        //
        // Если карточка рядом со смещаемой, добавляем дополнительное смещение по y
        //
        yInsertStateDelta += findYInsertStateDelta(card);

        //
        // Настраиваем размер карточки в зависимости от того развёрнута ли она на весь экран
        //
        bool isAct = false;
        bool isFullWidth = false;
        bool hasChildren = false;
        qreal xDelta = 0.0; // выступ акта, или папки слева от карточек
        qreal widthDelta = 0.0; // дополнительная ширина справа
        if (card->isContainer() && card->isTopLevel()) {
            isAct = true;
            isFullWidth = true;
            hasChildren = card->childCount() > 0;
            xDelta = isLeftToRight ? Ui::DesignSystem::layout().px(48)
                                   : Ui::DesignSystem::layout().px24();
            widthDelta = isLeftToRight ? Ui::DesignSystem::layout().px24()
                                       : Ui::DesignSystem::layout().px(48);
            containersStack.push(card->modelItemIndex());
        } else if (card->isContainer() && !card->isTopLevel()) {
            isFullWidth = card->isOpened();
            if (isFullWidth) {
                hasChildren = card->childCount() > 0;
                xDelta = Ui::DesignSystem::layout().px12();
                widthDelta = Ui::DesignSystem::layout().px12();
                containersStack.push(card->modelItemIndex());
            }
        }
        //
        // ... собственно корректируем размер карточки, если её сейчас не таскают конечно же
        //
        if (!movedCards.contains(card)) {
            auto cardRect = card->rect();
            if (isFullWidth) {
                //
                // ... для карточек расположенных на всю ширину учитываем смещение влево, а также
                //     берём на один спейсинг больше, чтобы у карточек в группах был отступ справа
                //
                cardRect.setWidth(cardsOptions.size.width() + xDelta + widthDelta);
                cardRect.setHeight(cardsOptions.size.height());
            } else {
                cardRect.setSize(cardsOptions.size);
            }
            card->setRect(cardRect);
        }

        //
        // Если входим в акт
        //
        if (isAct || isActClosed) {
            firstCardInColumnX += (isLeftToRight ? 1 : -1) * (lastItemWidth + cardsOptions.spacing);
            if (!isAct && isActClosed) {
                firstCardInColumnX -= Ui::DesignSystem::layout().px(48);
            } else if (isAct && !isActClosed) {
                firstCardInColumnX += Ui::DesignSystem::layout().px(48);
            }
            y = Ui::DesignSystem::projectCard().margins().top();
        }
        //
        // Определим положение карточки
        //
        const auto x = firstCardInColumnX - xDelta;
        //
        // ... позиционируем карточку, если она не перемещается в данный момент
        //     и новая позиция отличается от текущей
        //
        QPointF newItemPosition(x, y + yInsertStateDelta);
        if (!movedCards.contains(card) && card->pos() != newItemPosition) {
            const QRectF itemRect(card->pos(), card->boundingRect().size());
            const QRectF newItemRect(newItemPosition, card->boundingRect().size());
            if (cardsAnimationsAvailable && scene->isActive()
                && viewportRect.intersects(itemRect.united(newItemRect))
                // карточки, которые были вставлены по ходу пьесы, нужно сразу на место ставить,
                // чтобы они не вылетали из-за угла
                && card->pos() != kInvalidPosition) {
                //
                // ... анимируем смещение
                //
                QPointer<QVariantAnimation>& moveAnimation = cardsAnimations[card];
                if (moveAnimation.isNull()) {
                    moveAnimation = new QVariantAnimation(scene);
                    moveAnimation->setDuration(220);
                    moveAnimation->setEasingCurve(QEasingCurve::OutQuad);
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::valueChanged, q,
                                     [this, card](const QVariant& _value) {
                                         QSignalBlocker signalBlocker(scene);
                                         card->setPos(_value.toPointF());
                                     });
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::finished, q,
                                     [this] { scene->fitToContents(); });
                }
                //
                // ... при необходимости анимируем положение карточки
                //
                if (moveAnimation->endValue().toPointF() != newItemPosition) {
                    if (moveAnimation->state() == QVariantAnimation::Running) {
                        moveAnimation->pause();
                    } else {
                        moveAnimation->setStartValue(card->pos());
                    }
                    moveAnimation->setEndValue(newItemPosition);
                    if (moveAnimation->state() == QVariantAnimation::Paused) {
                        moveAnimation->resume();
                    } else {
                        moveAnimation->start(QAbstractAnimation::DeleteWhenStopped);
                    }
                }
            } else {
                card->setPos(newItemPosition);
            }
        }

        //
        // ... расположим карточки, чтобы никто не пропадал под родителем
        //
        card->setZValue((movedCards.contains(card)) ? z + cardsItems.size() : z);
        //
        // ... и корректируем координаты для позиционирования следующих элементов
        //
        if (isFullWidth && hasChildren) {
            lastItemHeight = card->headerHeight() - cardsOptions.spacing;
        } else {
            lastItemHeight = card->boundingRect().height();
        }
        y += lastItemHeight + cardsOptions.spacing;

        lastItemWidth = card->boundingRect().width();

        //
        // ... смещение от вставки корректируем только для карточек, для контейнеров оно будет
        //     закрыто, когда будет закрыт сам контейнер
        //
        if (!card->isContainer()) {
            yInsertStateDelta -= findYInsertStateDelta(card);
        }

        z = scene->nextZValue();
    }
    //
    // Закрываем последнюю открытую папку, если есть
    //
    while (!containersStack.isEmpty()) {
        //
        // Уберём его из списка родителей
        //
        auto containerItem = containersStack.pop();
        auto containerCard = modelItemsToCards[containerItem.internalPointer()];
        //
        // ... если он не перемещается, то скорректируем его размер по последнему из детей
        //
        if (!movedCards.contains(containerCard)) {
            auto containerRect = containerCard->rect();
            auto containerY = containerCard->pos().y();
            QPointer<QVariantAnimation>& moveAnimation = cardsAnimations[containerCard];
            if (!moveAnimation.isNull() && moveAnimation->state() == QVariantAnimation::Running) {
                containerY = moveAnimation->endValue().toPointF().y();
            }
            containerRect.setBottom(y - cardsOptions.spacing - containerY
                                    + findYInsertStateDelta(containerCard)
                                    + Ui::DesignSystem::layout().px12());
            containerCard->setRect(containerRect);
        }
        //
        // ... корректируем координаты для дальнейшей компоновки
        //
        y += Ui::DesignSystem::layout().px12();
    }

    //
    // Корректируем размер, чтобы все карточки персонажей были видны
    //
    scene->fitToContents();
}

void CardsGraphicsView::Implementation::reorderCardsInHorizontalLines()
{
    QSignalBlocker signalBlocker(scene);

    //
    // Определим хронологическую последовательность карточек
    //
    auto sortedCardsItems = cardsItems;
    std::sort(sortedCardsItems.begin(), sortedCardsItems.end(),
              [this](AbstractCardItem* _lhs, AbstractCardItem* _rhs) {
                  if (_lhs->positionOnLine() == _rhs->positionOnLine()) {
                      return cardsItems.indexOf(_lhs) < cardsItems.indexOf(_rhs);
                  }
                  return _lhs->positionOnLine() < _rhs->positionOnLine();
              });

    //
    // Определим начало координат
    //
    qreal xDelta = 0.0;
    for (const auto card : std::as_const(sortedCardsItems)) {
        if (!qFuzzyCompare(card->positionOnLine(), 0.0)) {
            xDelta = card->positionOnLine();
            break;
        }
    }

    //
    // Определим видимую область экрана
    //
    const QRectF viewportRect = [this] {
        if (scene->views().isEmpty()) {
            return QRectF();
        }

        const QGraphicsView* view = scene->views().constFirst();
        return view->mapToScene(view->viewport()->geometry()).boundingRect();
    }();

    //
    // Определим расположение первой карточки в колонке
    //
    const auto isLeftToRight = q->isLeftToRight();

    //
    // Проходим все элементы (они упорядочены в хронологическом порядке)
    //
    qreal z = scene->firstZValue();
    QVector<QRectF> cardsRects;
    for (auto card : std::as_const(sortedCardsItems)) {
        qreal x = qFuzzyCompare(card->positionOnLine(), 0.0)
            ? 0
            : ((card->positionOnLine() - xDelta) / static_cast<qreal>(60 * 60 * 24));
        qreal y = Ui::DesignSystem::projectCard().margins().bottom() + cardsOptions.size.height();
        //
        // Пропускаем невидимые карточки
        //
        if (card == nullptr || !card->isVisible()
            || (card->container() != nullptr && !card->container()->isOpened())) {
            continue;
        }

        //
        // Корректируем размер карточки, если её сейчас не таскают конечно же
        //
        if (!movedCards.contains(card)) {
            auto cardRect = card->rect();
            cardRect.setSize(cardsOptions.size);
            card->setRect(cardRect);
        }

        //
        // Определим положение карточки
        //
        if (!cardsRects.isEmpty()) {
            if (cardsRects.constLast().right() > x) {
                y = cardsRects.constLast().top() - cardsOptions.spacing
                    - cardsOptions.size.height();
            }
        }
        //
        // ... позиционируем карточку, если она не перемещается в данный момент
        //     и новая позиция отличается от текущей
        //
        const QPointF newItemPosition(x, y);
        if (!movedCards.contains(card) && card->pos() != newItemPosition) {
            const QRectF itemRect(card->pos(), card->boundingRect().size());
            const QRectF newItemRect(newItemPosition, card->boundingRect().size());
            if (cardsAnimationsAvailable && scene->isActive()
                && viewportRect.intersects(itemRect.united(newItemRect))
                // карточки, которые были вставлены по ходу пьесы, нужно сразу на место ставить,
                // чтобы они не вылетали из-за угла
                && card->pos() != kInvalidPosition) {
                //
                // ... анимируем смещение
                //
                QPointer<QVariantAnimation>& moveAnimation = cardsAnimations[card];
                if (moveAnimation.isNull()) {
                    moveAnimation = new QVariantAnimation(scene);
                    moveAnimation->setDuration(220);
                    moveAnimation->setEasingCurve(QEasingCurve::OutQuad);
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::valueChanged, scene,
                                     [this, card](const QVariant& _value) {
                                         QSignalBlocker signalBlocker(scene);
                                         card->setPos(_value.toPointF());
                                     });
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::finished, q,
                                     [this] { scene->fitToContents(); });
                }
                //
                // ... при необходимости анимируем положение карточки
                //
                if (moveAnimation->endValue().toPointF() != newItemPosition) {
                    if (moveAnimation->state() == QVariantAnimation::Running) {
                        moveAnimation->pause();
                    } else {
                        moveAnimation->setStartValue(card->pos());
                    }
                    moveAnimation->setEndValue(newItemPosition);
                    if (moveAnimation->state() == QVariantAnimation::Paused) {
                        moveAnimation->resume();
                    } else {
                        moveAnimation->start(QAbstractAnimation::DeleteWhenStopped);
                    }
                }
            } else {
                card->setPos(newItemPosition);
            }
        }
        //
        // ... расположим карточки, чтобы никто не пропадал под родителем
        //
        card->setZValue((movedCards.contains(card)) ? z + cardsItems.size() : z);

        z = scene->nextZValue();

        cardsRects.append(QRectF(newItemPosition, cardsOptions.size));
    }

    //
    // Корректируем размер, чтобы все карточки персонажей были видны
    //
    scene->fitToContents();

    //
    // Если нет перетаскиваемых мышкой карточек, то сбросим интервал, чтобы обновить его
    //
    if (scene->mouseGrabberItems().isEmpty()) {
        cardsPositionsInterval = {};
    }
}

void CardsGraphicsView::Implementation::reorderCardsInVerticalLines()
{
    QSignalBlocker signalBlocker(scene);

    //
    // Определим хронологическую последовательность карточек
    //
    auto sortedCardsItems = cardsItems;
    std::sort(sortedCardsItems.begin(), sortedCardsItems.end(),
              [this](AbstractCardItem* _lhs, AbstractCardItem* _rhs) {
                  if (_lhs->positionOnLine() == _rhs->positionOnLine()) {
                      return cardsItems.indexOf(_lhs) < cardsItems.indexOf(_rhs);
                  }
                  return _lhs->positionOnLine() < _rhs->positionOnLine();
              });

    //
    // Определим начало координат
    //
    qreal yDelta = 0.0;
    for (const auto card : std::as_const(sortedCardsItems)) {
        if (!qFuzzyCompare(card->positionOnLine(), 0.0)) {
            yDelta = card->positionOnLine();
            break;
        }
    }

    //
    // Определим видимую область экрана
    //
    const QRectF viewportRect = [this] {
        if (scene->views().isEmpty()) {
            return QRectF();
        }

        const QGraphicsView* view = scene->views().constFirst();
        return view->mapToScene(view->viewport()->geometry()).boundingRect();
    }();

    //
    // Определим расположение первой карточки в колонке
    //
    const auto isLeftToRight = q->isLeftToRight();

    //
    // Проходим все элементы (они упорядочены в хронологическом порядке)
    //
    qreal z = scene->firstZValue();
    QVector<QRectF> cardsRects;
    for (auto card : std::as_const(sortedCardsItems)) {
        qreal x = Ui::DesignSystem::projectCard().margins().left() + cardsOptions.size.width();
        qreal y = qFuzzyCompare(card->positionOnLine(), 0.0)
            ? 0
            : ((card->positionOnLine() - yDelta) / static_cast<qreal>(60 * 60 * 24));
        //
        // Пропускаем невидимые карточки
        //
        if (card == nullptr || !card->isVisible()
            || (card->container() != nullptr && !card->container()->isOpened())) {
            continue;
        }

        //
        // Корректируем размер карточки, если её сейчас не таскают конечно же
        //
        if (!movedCards.contains(card)) {
            auto cardRect = card->rect();
            cardRect.setSize(cardsOptions.size);
            card->setRect(cardRect);
        }

        //
        // Определим положение карточки
        //
        if (!cardsRects.isEmpty()) {
            if (cardsRects.constLast().bottom() > y) {
                x = cardsRects.constLast().right() + cardsOptions.spacing;
            }
        }
        //
        // ... позиционируем карточку, если она не перемещается в данный момент
        //     и новая позиция отличается от текущей
        //
        const QPointF newItemPosition(x, y);
        if (!movedCards.contains(card) && card->pos() != newItemPosition) {
            const QRectF itemRect(card->pos(), card->boundingRect().size());
            const QRectF newItemRect(newItemPosition, card->boundingRect().size());
            if (cardsAnimationsAvailable && scene->isActive()
                && viewportRect.intersects(itemRect.united(newItemRect))
                // карточки, которые были вставлены по ходу пьесы, нужно сразу на место ставить,
                // чтобы они не вылетали из-за угла
                && card->pos() != kInvalidPosition) {
                //
                // ... анимируем смещение
                //
                QPointer<QVariantAnimation>& moveAnimation = cardsAnimations[card];
                if (moveAnimation.isNull()) {
                    moveAnimation = new QVariantAnimation(scene);
                    moveAnimation->setDuration(220);
                    moveAnimation->setEasingCurve(QEasingCurve::OutQuad);
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::valueChanged, scene,
                                     [this, card](const QVariant& _value) {
                                         QSignalBlocker signalBlocker(scene);
                                         card->setPos(_value.toPointF());
                                     });
                    QObject::connect(moveAnimation.data(), &QVariantAnimation::finished, q,
                                     [this] { scene->fitToContents(); });
                }
                //
                // ... при необходимости анимируем положение карточки
                //
                if (moveAnimation->endValue().toPointF() != newItemPosition) {
                    if (moveAnimation->state() == QVariantAnimation::Running) {
                        moveAnimation->pause();
                    } else {
                        moveAnimation->setStartValue(card->pos());
                    }
                    moveAnimation->setEndValue(newItemPosition);
                    if (moveAnimation->state() == QVariantAnimation::Paused) {
                        moveAnimation->resume();
                    } else {
                        moveAnimation->start(QAbstractAnimation::DeleteWhenStopped);
                    }
                }
            } else {
                card->setPos(newItemPosition);
            }
        }
        //
        // ... расположим карточки, чтобы никто не пропадал под родителем
        //
        card->setZValue((movedCards.contains(card)) ? z + cardsItems.size() : z);

        z = scene->nextZValue();

        cardsRects.append(QRectF(newItemPosition, cardsOptions.size));
    }

    //
    // Корректируем размер, чтобы все карточки персонажей были видны
    //
    scene->fitToContents();

    //
    // Если нет перетаскиваемых мышкой карточек, то сбросим интервал, чтобы обновить его
    //
    if (scene->mouseGrabberItems().isEmpty()) {
        cardsPositionsInterval = {};
    }
}


// ****


CardsGraphicsView::CardsGraphicsView(CardsGraphicsScene* _scene, QWidget* _parent)
    : ScalableGraphicsView(_parent)
    , d(new Implementation(this, _scene))
{
    setFrameShape(QFrame::NoFrame);
    setScene(d->scene);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);

    setVerticalScrollBar(new ScrollBar(this));
    setHorizontalScrollBar(new ScrollBar(this));

    viewport()->installEventFilter(this);

    connect(this, &CardsGraphicsView::scaleChanged, this, [this] { d->reorderCards(); });
    connect(d->scene, &CardsGraphicsScene::selectionChanged, this, [this] {
        if (d->scene->selectedItems().isEmpty()) {
            emit selectionCanceled();
            return;
        }

        auto selectedItem = d->scene->selectedItems().constFirst();
        auto selectedCard = static_cast<AbstractCardItem*>(selectedItem);
        emit itemSelected(selectedCard->modelItemIndex());
    });
    connect(d->scene, &CardsGraphicsScene::itemDoubleClicked, this,
            &CardsGraphicsView::itemDoubleClicked);
    connect(d->scene, &CardsGraphicsScene::itemChanged, this, [this](const QModelIndex& _index) {
        if (d->model->rowCount(_index) > 0) {
            std::function<void(const QModelIndex&, bool)> updateChildrenVisibility;
            updateChildrenVisibility = [this, &updateChildrenVisibility](const QModelIndex& _index,
                                                                         bool _isVisible) {
                const auto cardItem = d->modelItemsToCards.value(_index.internalPointer());
                const auto isChildVisible = _isVisible && cardItem->isOpened();
                for (int childRow = 0; childRow < d->model->rowCount(_index); ++childRow) {
                    const auto childIndex = d->model->index(childRow, 0, _index);
                    auto childCardIter = d->modelItemsToCards.find(childIndex.internalPointer());
                    if (childCardIter == d->modelItemsToCards.end()) {
                        continue;
                    }

                    childCardIter.value()->setVisible(isChildVisible);

                    if (d->model->rowCount(childIndex) > 0) {
                        updateChildrenVisibility(childIndex, isChildVisible);
                    }
                }
            };
            updateChildrenVisibility(
                _index, d->modelItemsToCards.value(_index.internalPointer())->isOpened());
        }

        d->reorderCards();
        emit itemChanged(_index);
    });
    connect(d->scene, &CardsGraphicsScene::itemMoved, this, [this](const QModelIndex& _index) {
        d->movedCards = d->scene->mouseGrabberItems();

        d->reorderCard(_index);
        emit itemChanged(_index);
    });
    connect(d->scene, &CardsGraphicsScene::itemDropped, this, [this](const QModelIndex& _index) {
        if (d->moveTarget.isValid()
            && (d->moveTarget.row != _index.row() || d->moveTarget.parent != _index.parent())) {
            auto movedCard = d->modelItemsToCards.value(_index.internalPointer());
            auto container = d->modelItemsToCards.value(d->moveTarget.parent.internalPointer());
            movedCard->dropToContainer(container);
            d->model->moveRow(_index.parent(), _index.row(), d->moveTarget.parent,
                              d->moveTarget.row);
        }

        for (auto card : std::as_const(d->cardsItems)) {
            card->setInsertionState(AbstractCardItem::InsertionState::Empty);
        }
        d->moveTarget = {};
        d->movedCards.clear();

        d->reorderCards();
    });
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        if (d->cardsOptions.type == CardsGraphicsViewType::HorizontalLines) {
            d->cardsPositionsInterval = {};
        }
    });
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        if (d->cardsOptions.type == CardsGraphicsViewType::VerticalLines) {
            d->cardsPositionsInterval = {};
        }
    });
}

CardsGraphicsView::~CardsGraphicsView() = default;

void CardsGraphicsView::setBackgroundColor(const QColor& _color)
{
    scene()->setBackgroundBrush(_color);
}

void CardsGraphicsView::setAdditionalScrollingAvailable(bool _available)
{
    d->scene->setAdditionalScrollingAvailable(_available);
}

void CardsGraphicsView::setCardsViewType(CardsGraphicsViewType _type)
{
    if (d->cardsOptions.type == _type) {
        return;
    }

    d->cardsOptions.type = _type;
    d->reorderCards();
}

void CardsGraphicsView::setCardsSize(int _size)
{
    if (d->cardsOptions.sizeScale == _size) {
        return;
    }

    d->cardsOptions.sizeScale = _size;
    d->updateCardsOptions();

    QScopedValueRollback<bool> cardsAnimationsBlocker(d->cardsAnimationsAvailable, false);
    d->reorderCardsImpl();
}

void CardsGraphicsView::setCardsRatio(int _ratio)
{
    if (d->cardsOptions.ratio == _ratio) {
        return;
    }

    d->cardsOptions.ratio = _ratio;
    d->updateCardsOptions();

    QScopedValueRollback<bool> cardsAnimationsBlocker(d->cardsAnimationsAvailable, false);
    d->reorderCardsImpl();
}

void CardsGraphicsView::setCardsSpacing(int _spacing)
{
    if (d->cardsOptions.spacingScale == _spacing) {
        return;
    }

    d->cardsOptions.spacingScale = _spacing;
    d->updateCardsOptions();

    QScopedValueRollback<bool> cardsAnimationsBlocker(d->cardsAnimationsAvailable, false);
    d->reorderCardsImpl();
}

void CardsGraphicsView::setCardsInRow(int _cardsInRow)
{
    if (d->cardsOptions.cardsInRow == _cardsInRow) {
        return;
    }

    d->cardsOptions.cardsInRow = _cardsInRow;
    d->reorderCards();
}

QPair<qreal, qreal> CardsGraphicsView::cardsPositionsInterval() const
{
    if (d->cardsOptions.type != CardsGraphicsViewType::HorizontalLines
        && d->cardsOptions.type != CardsGraphicsViewType::VerticalLines) {
        return {};
    }

    //
    // Если хотя бы для одной карточки задана позиция на линии
    //
    Ui::AbstractCardItem* cardWithPosition = nullptr;
    for (const auto card : std::as_const(d->cardsItems)) {
        const auto cardPosition = card->positionOnLine();
        if (!qFuzzyCompare(cardPosition, 0.0)) {
            cardWithPosition = card;
            break;
        }
    }
    if (cardWithPosition != nullptr) {
        //
        // Если заполнено закешированное значение интервала, то используем его
        //
        if (d->cardsPositionsInterval.isValid()) {
            return { d->cardsPositionsInterval.minimum, d->cardsPositionsInterval.maximum };
        }

        //
        // В противном случае, рассчитываем интервал и кешируем
        //
        const auto minimumPosition = cardWithPosition->positionOnLine();
        const auto maximumPosition = minimumPosition + 30 * 60 * 60 * 24;
        if (d->cardsOptions.type == CardsGraphicsViewType::HorizontalLines) {
            const auto minimumX = mapFromScene(cardWithPosition->scenePos()).x();
            const auto maximumPosTranslated = transform().map(
                QPointF((maximumPosition - minimumPosition) / static_cast<qreal>(60 * 60 * 24), 0));
            const auto maximumX = minimumX + maximumPosTranslated.x();
            const auto pxDistance = (maximumPosition - minimumPosition) / (maximumX - minimumX);
            d->cardsPositionsInterval.minimum = minimumPosition - minimumX * pxDistance;
            d->cardsPositionsInterval.maximum
                = d->cardsPositionsInterval.minimum + width() * pxDistance;
        } else {
            const auto minimumY = mapFromScene(cardWithPosition->scenePos()).y();
            const auto maximumPosTranslated = transform().map(
                QPointF(0, (maximumPosition - minimumPosition) / static_cast<qreal>(60 * 60 * 24)));
            const auto maximumY = minimumY + maximumPosTranslated.y();
            const auto pxDistance = (maximumPosition - minimumPosition) / (maximumY - minimumY);
            d->cardsPositionsInterval.minimum = minimumPosition - minimumY * pxDistance;
            d->cardsPositionsInterval.maximum
                = d->cardsPositionsInterval.minimum + height() * pxDistance;
        }
        return { d->cardsPositionsInterval.minimum, d->cardsPositionsInterval.maximum };
    }

    //
    // Если ни для одной карточки не задана позиция на линии, то считем, что интервал не определён
    //
    return {};
}

QAbstractItemModel* CardsGraphicsView::model() const
{
    return d->model;
}

void CardsGraphicsView::setModel(QAbstractItemModel* _model)
{
    if (d->model == _model) {
        return;
    }

    if (d->model != nullptr) {
        d->model->disconnect(this);
    }

    d->scene->clear();
    d->cardsAnimations.clear();
    d->modelItemsToCards.clear();
    d->cardsItems.clear();
    d->model = _model;

    if (d->model == nullptr) {
        return;
    }

    auto loadModelContent = [this] {
        //
        // Загружаем карточки
        //
        for (int row = 0; row < d->model->rowCount(); ++row) {
            const auto isVisible = true;
            d->insertCard(d->model->index(row, 0, {}), isVisible);
        }

        d->reorderCards();

        //
        // Корректируем размер сцены, добавляя небольшую прокрутку по сторонам,
        // делаем это отложенно, чтобы виджет появился на экране и не было дёргания
        //
        if (verticalScrollBar()->maximum() == 0 || horizontalScrollBar()->maximum() == 0) {
            QMetaObject::invokeMethod(
                this,
                [this] {
                    auto newSceneRect = d->scene->sceneRect();
                    const auto screenSize = screen()->size();
                    const auto widthDelta = screenSize.width();
                    const auto heightDelta = widthDelta;
                    newSceneRect.adjust(-widthDelta, -heightDelta, widthDelta, heightDelta);
                    d->scene->setSceneRect(newSceneRect);
                },
                Qt::QueuedConnection);
        }

        //
        // При необходимости посылаем запрос на отображение
        //
        d->notifyVisibleChange();
    };

    //
    // Настраиваем соединения на изменение состава модели
    //
    connect(d->model, &QAbstractItemModel::modelAboutToBeReset, this, [this] {
        d->scene->clear();
        d->cardsAnimations.clear();
        d->modelItemsToCards.clear();
        d->cardsItems.clear();
    });
    connect(d->model, &QAbstractItemModel::modelReset, this, loadModelContent);
    connect(d->model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& _topLeft) {
        auto cardIter = d->modelItemsToCards.find(_topLeft.internalPointer());
        if (cardIter == d->modelItemsToCards.end()) {
            return;
        }

        cardIter.value()->update();
        d->reorderCard(_topLeft);
    });
    connect(d->model, &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex& _parent, int _first, int _last) {
                //
                // Вставляем карточки
                //
                auto containerCard = d->modelItemsToCards.value(_parent.internalPointer());
                for (int row = _first; row <= _last; ++row) {
                    const bool isVisible = containerCard == nullptr || containerCard->isOpened();
                    auto card = d->insertCard(d->model->index(row, 0, _parent), isVisible);
                    if (card != nullptr) {
                        if (containerCard != nullptr) {
                            card->setContainer(containerCard);
                        }
                        card->update();
                    }
                }

                //
                // Корректируем положение карточек
                //
                d->reorderCards();

                //
                // При необходимости посылаем запрос на отображение
                //
                d->notifyVisibleChange();
            });
    connect(d->model, &QAbstractItemModel::rowsAboutToBeRemoved, this,
            [this](const QModelIndex& _parent, int _first, int _last) {
                //
                // Удаляем карточки
                //
                for (int row = _last; row >= _first; --row) {
                    const auto itemIndex = d->model->index(row, 0, _parent);
                    auto cardIter = d->modelItemsToCards.find(itemIndex.internalPointer());
                    if (cardIter == d->modelItemsToCards.end()) {
                        continue;
                    }

                    d->removeItem(cardIter.value());
                }
            });
    connect(d->model, &QAbstractItemModel::rowsRemoved, this,
            [this](const QModelIndex& _parent, int _first) {
                //
                // Смещаем индексы модели идущих за удалёнными карточками элементов того же уровня
                //
                const auto firstRemovedItemIndex = d->model->index(_first, 0, _parent);
                for (int row = firstRemovedItemIndex.row(); row < d->model->rowCount(_parent);
                     ++row) {
                    const auto index = d->model->index(row, 0, _parent);
                    if (!d->modelItemsToCards.contains(index.internalPointer())) {
                        continue;
                    }

                    d->modelItemsToCards[index.internalPointer()]->setModelItemIndex(index);
                }

                //
                // Корректируем положение карточек
                //
                d->reorderCards();

                //
                // При необходимости посылаем запрос на отображение
                //
                d->notifyVisibleChange();
            });
    connect(d->model, &QAbstractItemModel::rowsAboutToBeMoved, this,
            [this](const QModelIndex& _sourceParent, int _first, int _last) {
                //
                // Удаляем перемещаемые карточки
                //
                for (int row = _last; row >= _first; --row) {
                    const auto itemIndex = d->model->index(row, 0, _sourceParent);
                    auto cardIter = d->modelItemsToCards.find(itemIndex.internalPointer());
                    if (cardIter == d->modelItemsToCards.end()) {
                        continue;
                    }

                    d->removeItem(cardIter.value());
                }
            });
    connect(d->model, &QAbstractItemModel::rowsMoved, this,
            [this](const QModelIndex& _sourceParent, int _first, int _last,
                   const QModelIndex& _destination, int _destinationRow) {
                //
                // Смещаем индексы модели идущих за удалёнными карточками элементов того же уровня
                //
                const auto firstRemovedItemIndex = d->model->index(_first, 0, _sourceParent);
                for (int row = firstRemovedItemIndex.row(); row < d->model->rowCount(_sourceParent);
                     ++row) {
                    const auto index = d->model->index(row, 0, _sourceParent);
                    if (!d->modelItemsToCards.contains(index.internalPointer())) {
                        continue;
                    }

                    d->modelItemsToCards[index.internalPointer()]->setModelItemIndex(index);
                }

                //
                // Вставляем перемещённые карточки
                //
                auto containerCard = d->modelItemsToCards.value(_destination.internalPointer());
                const auto destinationCorrected
                    = _destinationRow < _first ? _destinationRow : _destinationRow - 1;
                for (int row = destinationCorrected; row <= destinationCorrected + (_last - _first);
                     ++row) {
                    const bool isVisible = containerCard == nullptr || containerCard->isOpened();
                    auto card = d->insertCard(d->model->index(row, 0, _destination), isVisible);
                    if (card != nullptr) {
                        if (containerCard != nullptr) {
                            card->setContainer(containerCard);
                        }
                        card->update();
                    }
                }

                //
                // Корректируем положение карточек
                //
                d->reorderCards();
            });

    loadModelContent();
    centerOn(sceneRect().center());
}

QModelIndex CardsGraphicsView::selectedCardItemIndex() const
{
    if (d->scene->selectedItems().isEmpty()) {
        return {};
    }

    auto lastSelectedCard
        = qgraphicsitem_cast<AbstractCardItem*>(d->scene->selectedItems().constLast());
    if (lastSelectedCard == nullptr) {
        return {};
    }

    return lastSelectedCard->modelItemIndex();
}

void CardsGraphicsView::selectCardItemIndex(const QModelIndex& _index)
{
    if (!d->movedCards.isEmpty()) {
        return;
    }

    auto card = d->modelItemsToCards.value(_index.internalPointer());
    if (card == nullptr) {
        return;
    }

    const auto selectedItems = d->scene->selectedItems();
    if (!selectedItems.isEmpty() && selectedItems.constFirst() == card) {
        return;
    }

    card->setSelected(true);
    for (auto item : selectedItems) {
        item->setSelected(false);
    }

    const auto animationIter = d->cardsAnimations.find(card);
    if (animationIter != d->cardsAnimations.end() && !animationIter.value().isNull()) {
        Once::connect(animationIter.value().data(), &QVariantAnimation::finished, this,
                      [this, card] { animateEnsureVisible(card); });
    } else if (d->reorderCardsDebounceTimer.isActive()) {
        Once::connect(&d->reorderCardsDebounceTimer, &QTimer::timeout, this,
                      [this, card] { animateEnsureVisible(card); });
    } else {
        animateEnsureVisible(card);
    }
}

void CardsGraphicsView::removeSelectedItem()
{
    const auto selectedItems = d->scene->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    for (auto selectedItem : selectedItems) {
        d->removeItem(static_cast<AbstractCardItem*>(selectedItem));
        break;
    }
    d->scene->clearSelection();
}

void CardsGraphicsView::setFilter(const QString& _text, bool _caseSensitive, int _filterType)
{
    for (auto card : std::as_const(d->cardsItems)) {
        if (_text.isEmpty()) {
            card->setOpacity(1.0);
            continue;
        }

        const auto isFilterAccepted = card->isFilterAccepted(_text, _caseSensitive, _filterType);
        card->setOpacity(isFilterAccepted ? 1.0 : 0.4);
        if (isFilterAccepted) {
            auto container = card->container();
            while (container != nullptr) {
                container->setOpacity(1.0);
                container = container->container();
            }
        }
    }
}

void CardsGraphicsView::setReadOnly(bool _readOnly)
{
    d->scene->setReadOnly(_readOnly);
}

void CardsGraphicsView::restoreState(const QByteArray& _state)
{
    if (d->reorderCardsDebounceTimer.isActive()) {
        d->pendingState = _state;
        return;
    }

    ScalableGraphicsView::restoreState(_state);
}

bool CardsGraphicsView::excludeFromFlatIndex(const QModelIndex& _index) const
{
    Q_UNUSED(_index)
    return false;
}

bool CardsGraphicsView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == viewport() && _event->type() == QEvent::Resize) {
        d->cardsAnimationsAvailable = false;
        d->reorderCardsImpl();
        d->cardsAnimationsAvailable = true;
    }

    return ScalableGraphicsView::eventFilter(_watched, _event);
}

bool CardsGraphicsView::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
        d->reorderCards();
        for (auto card : std::as_const(d->cardsItems)) {
            card->update();
        }
        return true;
    }

    default: {
        return ScalableGraphicsView::event(_event);
    }
    }
}

void CardsGraphicsView::keyPressEvent(QKeyEvent* _event)
{
    if (_event == QKeySequence::Undo) {
        emit undoPressed();
    } else if (_event == QKeySequence::Redo) {
        emit redoPressed();
    }

    ScalableGraphicsView::keyPressEvent(_event);
}

} // namespace Ui
