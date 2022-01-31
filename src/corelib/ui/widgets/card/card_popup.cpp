#include "card_popup.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QBoxLayout>
#include <QVariantAnimation>


class CardPopup::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Tree* content = nullptr;
    QVariantAnimation heightAnimation;
};

CardPopup::Implementation::Implementation(QWidget* _parent)
    : content(new Tree(_parent))
{
    content->setRootIsDecorated(false);

    heightAnimation.setEasingCurve(QEasingCurve::OutQuint);
    heightAnimation.setDuration(240);
    heightAnimation.setStartValue(0);
    heightAnimation.setEndValue(0);
}


// ****


CardPopup::CardPopup(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation(this))
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    hide();

    auto popupLayout = new QHBoxLayout;
    popupLayout->setContentsMargins({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(d->content);
    setLayoutReimpl(popupLayout);


    connect(&d->heightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { resize(width(), _value.toInt()); });
    connect(&d->heightAnimation, &QVariantAnimation::finished, this, [this] {
        if (d->heightAnimation.direction() == QVariantAnimation::Backward
            && d->heightAnimation.currentValue().toInt() == 0) {
            hide();
        }
    });

    connect(d->content, &Tree::currentIndexChanged, this, [this](const QModelIndex& _index) {
        hidePopup();
        emit currentIndexChanged(_index);
    });


    designSystemChangeEvent(nullptr);
}

CardPopup::~CardPopup() = default;

QAbstractItemModel* CardPopup::contentModel() const
{
    return d->content->model();
}

void CardPopup::setContentModel(QAbstractItemModel* _model)
{
    d->content->setModel(_model);
}

QModelIndex CardPopup::currentIndex() const
{
    return d->content->currentIndex();
}

void CardPopup::setCurrentIndex(const QModelIndex& _index)
{
    d->content->setCurrentIndex(_index);
}

int CardPopup::sizeHintForColumn(int _column) const
{
    return d->content->sizeHintForColumn(_column);
}

void CardPopup::showPopup(const QPoint& _position, int _width, int _showMaxItems)
{
    if (d->content->model() == nullptr) {
        return;
    }

    resize(_width, 0);
    move(_position);
    show();

    d->content->setScrollBarVisible(d->content->model()->rowCount() > _showMaxItems);

    d->heightAnimation.setDirection(QVariantAnimation::Forward);
    const auto itemsCount = std::min(d->content->model()->rowCount(), _showMaxItems);
    const auto height = Ui::DesignSystem::treeOneLineItem().height() * itemsCount
        + Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::card().shadowMargins().bottom();
    d->heightAnimation.setEndValue(static_cast<int>(height));
    d->heightAnimation.start();
}

void CardPopup::hidePopup()
{
    d->heightAnimation.setDirection(QVariantAnimation::Backward);
    d->heightAnimation.start();
}

void CardPopup::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    d->content->setBackgroundColor(Ui::DesignSystem::color().background());
    d->content->setTextColor(Ui::DesignSystem::color().onBackground());
}
