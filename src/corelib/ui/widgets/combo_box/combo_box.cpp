#include "combo_box.h"

#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/tree/tree.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemModel>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScrollBar>
#include <QVariantAnimation>


class ComboBox::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Показать попап
     */
    void showPopup(ComboBox* _parent);

    /**
     * @brief Скрыть попап
     */
    void hidePopup();


    bool useContentsWidth = false;
    bool isPopupShown = false;
    Card* popup = nullptr;
    Tree* popupContent = nullptr;
    QVariantAnimation popupHeightAnimation;
};

ComboBox::Implementation::Implementation(QWidget* _parent)
    : popup(new Card(_parent))
    , popupContent(new Tree(popup))
{
    popup->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    popup->setAttribute(Qt::WA_Hover, false);
    popup->setAttribute(Qt::WA_TranslucentBackground);
    popup->setAttribute(Qt::WA_ShowWithoutActivating);
    popup->hide();

    popupContent->setRootIsDecorated(false);

    QHBoxLayout* popupLayout = new QHBoxLayout;
    popupLayout->setContentsMargins({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(popupContent);
    popup->setLayoutReimpl(popupLayout);

    popupHeightAnimation.setEasingCurve(QEasingCurve::OutQuint);
    popupHeightAnimation.setDuration(240);
    popupHeightAnimation.setStartValue(0);
    popupHeightAnimation.setEndValue(0);
}

void ComboBox::Implementation::showPopup(ComboBox* _parent)
{
    if (popupContent->model() == nullptr) {
        return;
    }

    isPopupShown = true;

    auto leftMargin = Ui::DesignSystem::textField().contentsMargins().left();
    auto rightMargin = Ui::DesignSystem::textField().contentsMargins().right();
    if (!_parent->isDefaultMarginsEnabled()) {
        leftMargin = _parent->customMargins().left();
        rightMargin = _parent->customMargins().right();
    }
    auto width = _parent->width() - leftMargin - rightMargin;
    if (useContentsWidth) {
        const auto model = popupContent->model();
        for (int row = 0; row < model->rowCount(); ++row) {
            const auto itemText = model->index(row, 0).data().toString();
            const auto itemTextWidth = Ui::DesignSystem::treeOneLineItem().margins().left()
                + TextHelper::fineTextWidthF(itemText, Ui::DesignSystem::font().body1())
                + Ui::DesignSystem::treeOneLineItem().margins().right()
                + Ui::DesignSystem::layout().px24();
            if (itemTextWidth > width) {
                width = itemTextWidth;
            }
        }
    }
    width += Ui::DesignSystem::card().shadowMargins().left()
        + Ui::DesignSystem::card().shadowMargins().right();
    popup->resize(static_cast<int>(width), 0);
    auto pos = _parent->mapToGlobal(_parent->rect().bottomLeft())
        + QPointF(leftMargin - Ui::DesignSystem::card().shadowMargins().left(),
                  -Ui::DesignSystem::textField().margins().bottom());
    popup->move(pos.toPoint());
    popup->show();

    const int maxPopupItems = 5;
    popupContent->setScrollBarVisible(popupContent->model()->rowCount() > maxPopupItems);

    popupHeightAnimation.setDirection(QVariantAnimation::Forward);
    const auto itemsCount = std::min(popupContent->model()->rowCount(), maxPopupItems);
    const auto height = Ui::DesignSystem::treeOneLineItem().height() * itemsCount
        + Ui::DesignSystem::card().shadowMargins().top()
        + Ui::DesignSystem::card().shadowMargins().bottom();
    popupHeightAnimation.setEndValue(static_cast<int>(height));
    popupHeightAnimation.start();
}

void ComboBox::Implementation::hidePopup()
{
    isPopupShown = false;

    popupHeightAnimation.setDirection(QVariantAnimation::Backward);
    popupHeightAnimation.start();
}


// ****


ComboBox::ComboBox(QWidget* _parent)
    : TextField(_parent)
    , d(new Implementation(this))
{
    setReadOnly(true);
    setTrailingIcon(u8"\U000f035d");
    viewport()->setCursor(Qt::ArrowCursor);
    viewport()->setMouseTracking(false);

    connect(&d->popupHeightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                const auto height = _value.toInt();
                d->popup->resize(d->popup->width(), height);
            });
    connect(&d->popupHeightAnimation, &QVariantAnimation::finished, this, [this] {
        if (!d->isPopupShown) {
            d->popup->hide();
        }
    });

    connect(d->popupContent, &Tree::currentIndexChanged, this, [this](const QModelIndex& _index) {
        setText(_index.data().toString());
        d->hidePopup();
        emit currentIndexChanged(_index);
    });
}

ComboBox::~ComboBox() = default;

void ComboBox::setUseContentsWidth(bool _use)
{
    d->useContentsWidth = _use;
}

ContextMenu* ComboBox::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    Q_UNUSED(_position)
    Q_UNUSED(_parent)

    return nullptr;
}

QAbstractItemModel* ComboBox::model() const
{
    return d->popupContent->model();
}

void ComboBox::setModel(QAbstractItemModel* _model)
{
    if (d->popupContent->model() != nullptr) {
        disconnect(d->popupContent->model());
    }

    d->popupContent->setModel(_model);

    if (_model != nullptr && _model->rowCount() > 0) {
        d->popupContent->setCurrentIndex(_model->index(0, 0));
        connect(_model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& _index) {
            if (d->popupContent->currentIndex() == _index) {
                setText(_index.data().toString());
            }
        });
    }
}

QModelIndex ComboBox::currentIndex() const
{
    return d->popupContent->currentIndex();
}

void ComboBox::setCurrentIndex(const QModelIndex& _index)
{
    if (d->popupContent->currentIndex() == _index) {
        return;
    }

    d->popupContent->setCurrentIndex(_index);
    setText(_index.data().toString());
}

QString ComboBox::currentText() const
{
    return currentIndex().data().toString();
}

void ComboBox::setCurrentText(const QString& _text)
{
    for (int row = 0; row < model()->rowCount(); ++row) {
        const auto index = model()->index(row, 0);
        if (index.data().toString() == _text) {
            setCurrentIndex(index);
            return;
        }
    }
}

void ComboBox::reconfigure()
{
    TextField::reconfigure();

    d->popup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->popupContent->setBackgroundColor(Ui::DesignSystem::color().background());
    d->popupContent->setTextColor(Ui::DesignSystem::color().onBackground());
}

void ComboBox::focusOutEvent(QFocusEvent* _event)
{
    //
    // Запустим анимацию потери фокуса, если попап не показан или пользователь кликнул
    // за пределами виджета и выпадающего списка
    //
    if (!d->isPopupShown || (!underMouse() && !d->popupContent->underMouse())) {
        TextField::focusOutEvent(_event);
    }

    //
    // Не скрываем попап, если фокус перешёл на его полосу прокрутки
    //
    if (d->popupContent->verticalScrollBar()->underMouse()) {
        return;
    }

    setTrailingIcon(u8"\U000f035d");
    setTrailingIconColor({});
    d->hidePopup();
}

void ComboBox::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Up) {
        if (currentIndex().row() > 0) {
            setCurrentIndex(model()->index(currentIndex().row() - 1, 0));
        }
    } else if (_event->key() == Qt::Key_Down) {
        if (currentIndex().row() < model()->rowCount() - 1) {
            setCurrentIndex(model()->index(currentIndex().row() + 1, 0));
        }
    }

    TextField::keyPressEvent(_event);
}

void ComboBox::mousePressEvent(QMouseEvent* _event)
{
    TextField::mousePressEvent(_event);

    if (!d->isPopupShown) {
        setTrailingIcon(u8"\U000f0360");
        setTrailingIconColor(Ui::DesignSystem::color().secondary());
        d->showPopup(this);
    } else {
        setTrailingIcon(u8"\U000f035d");
        setTrailingIconColor({});
        d->hidePopup();
    }
}
