#include "card_popup_with_tree.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QApplication>
#include <QBoxLayout>
#include <QScreen>
#include <QVariantAnimation>


class CardPopupWithTree::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Tree* content = nullptr;
};

CardPopupWithTree::Implementation::Implementation(QWidget* _parent)
    : content(new Tree(_parent))
{
    content->setRootIsDecorated(false);
}


// ****


CardPopupWithTree::CardPopupWithTree(QWidget* _parent)
    : CardPopup(_parent)
    , d(new Implementation(this))
{
    auto popupLayout = new QHBoxLayout;
    popupLayout->setContentsMargins({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(d->content);
    setContentLayout(popupLayout);

    connect(d->content, &Tree::currentIndexChanged, this, [this](const QModelIndex& _index) {
        hidePopup();
        emit currentIndexChanged(_index);
    });


    designSystemChangeEvent(nullptr);
}

CardPopupWithTree::~CardPopupWithTree() = default;

QAbstractItemModel* CardPopupWithTree::contentModel() const
{
    return d->content->model();
}

void CardPopupWithTree::setContentModel(QAbstractItemModel* _model)
{
    d->content->setModel(_model);
}

QModelIndex CardPopupWithTree::currentIndex() const
{
    return d->content->currentIndex();
}

void CardPopupWithTree::setCurrentIndex(const QModelIndex& _index)
{
    d->content->setCurrentIndex(_index);
}

int CardPopupWithTree::sizeHintForColumn(int _column) const
{
    return d->content->sizeHintForColumn(_column);
}

void CardPopupWithTree::showPopup(const QPoint& _position, int _parentHeight, int _width,
                                  int _showMaxItems)
{
    if (d->content->model() == nullptr) {
        return;
    }

    //
    // Определим высоту попапа
    //
    const auto itemsCount = std::min(d->content->model()->rowCount(), _showMaxItems);
    const auto height = Ui::DesignSystem::treeOneLineItem().height() * itemsCount;

    d->content->setScrollBarVisible(d->content->model()->rowCount() > _showMaxItems);

    CardPopup::showPopup(_position, _parentHeight, QSize(_width, height));
}

void CardPopupWithTree::processBackgroundColorChange()
{
    d->content->setBackgroundColor(backgroundColor());
}

void CardPopupWithTree::processTextColorChange()
{
    d->content->setTextColor(textColor());
}
