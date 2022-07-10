#include "combo_box.h"

#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card_popup_with_tree.h>
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
    bool isReadOnly = false;
    CardPopupWithTree* popup = nullptr;
};

ComboBox::Implementation::Implementation(QWidget* _parent)
    : popup(new CardPopupWithTree(_parent))
{
}

void ComboBox::Implementation::showPopup(ComboBox* _parent)
{
    auto leftMargin = Ui::DesignSystem::textField().contentsMargins().left();
    auto rightMargin = Ui::DesignSystem::textField().contentsMargins().right();
    if (!_parent->isDefaultMarginsEnabled()) {
        leftMargin = _parent->customMargins().left();
        rightMargin = _parent->customMargins().right();
    }
    auto width = _parent->width() - leftMargin - rightMargin;
    if (useContentsWidth) {
        const auto model = popup->contentModel();
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

    const auto posBottomDelta = _parent->isDefaultMarginsEnabled()
        ? Ui::DesignSystem::textField().contentsMargins().bottom()
        : _parent->customMargins().bottom();
    auto pos = _parent->mapToGlobal(_parent->rect().bottomLeft() - QPoint(0, posBottomDelta))
        + QPointF(leftMargin - Ui::DesignSystem::card().shadowMargins().left(),
                  -Ui::DesignSystem::textField().margins().bottom());
    popup->showPopup(pos.toPoint(),
                     _parent->height() - Ui::DesignSystem::textField().margins().top(), width);
}

void ComboBox::Implementation::hidePopup()
{
    popup->hidePopup();
}


// ****


ComboBox::ComboBox(QWidget* _parent)
    : TextField(_parent)
    , d(new Implementation(this))
{
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setTrailingIcon(u8"\U000f035d");
    viewport()->setCursor(Qt::ArrowCursor);
    viewport()->setMouseTracking(false);

    connect(d->popup, &CardPopupWithTree::currentIndexChanged, this,
            [this](const QModelIndex& _index) {
                setText(_index.data().toString());
                emit currentIndexChanged(_index);
            });
    connect(d->popup, &Card::disappeared, this, [this] {
        setTrailingIcon(u8"\U000f035d");
        setTrailingIconColor({});
    });
}

ComboBox::~ComboBox() = default;

void ComboBox::setReadOnly(bool _readOnly)
{
    d->isReadOnly = _readOnly;
}

void ComboBox::setPopupBackgroundColor(const QColor& _color)
{
    d->popup->setBackgroundColor(_color);
}

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
    return d->popup->contentModel();
}

void ComboBox::setModel(QAbstractItemModel* _model)
{
    if (d->popup->contentModel() != nullptr) {
        disconnect(d->popup->contentModel());
    }

    d->popup->setContentModel(_model);

    if (_model != nullptr && _model->rowCount() > 0) {
        d->popup->setCurrentIndex(_model->index(0, 0));
        connect(_model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& _index) {
            if (d->popup->currentIndex() == _index) {
                setText(_index.data().toString());
            }
        });
    }
}

QModelIndex ComboBox::currentIndex() const
{
    return d->popup->currentIndex();
}

void ComboBox::setCurrentIndex(const QModelIndex& _index)
{
    if (d->popup->currentIndex() == _index) {
        return;
    }

    d->popup->setCurrentIndex(_index);
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

    d->popup->setTextColor(textColor());
}

void ComboBox::focusOutEvent(QFocusEvent* _event)
{
    //
    // Запустим анимацию потери фокуса, если попап не показан или пользователь кликнул
    // за пределами виджета и выпадающего списка
    //
    if (!d->popup->isVisible() || (!underMouse() && !d->popup->underMouse())) {
        TextField::focusOutEvent(_event);
    }
}

void ComboBox::keyPressEvent(QKeyEvent* _event)
{
    if (!d->isReadOnly) {
        if (_event->key() == Qt::Key_Up) {
            if (currentIndex().row() > 0) {
                setCurrentIndex(model()->index(currentIndex().row() - 1, 0));
            }
        } else if (_event->key() == Qt::Key_Down) {
            if (currentIndex().row() < model()->rowCount() - 1) {
                setCurrentIndex(model()->index(currentIndex().row() + 1, 0));
            }
        }
    }

    TextField::keyPressEvent(_event);
}

void ComboBox::mousePressEvent(QMouseEvent* _event)
{
    TextField::mousePressEvent(_event);

    if (!d->isReadOnly) {
        if (!d->popup->isVisible()) {
            setTrailingIcon(u8"\U000f0360");
            setTrailingIconColor(Ui::DesignSystem::color().secondary());
            d->showPopup(this);
        } else {
            setTrailingIcon(u8"\U000f035d");
            setTrailingIconColor({});
            d->hidePopup();
        }
    }
}
