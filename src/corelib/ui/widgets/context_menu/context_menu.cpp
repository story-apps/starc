#include "context_menu.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QTimer>
#include <QScreen>
#include <QVariantAnimation>


class ContextMenu::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Tree* content = nullptr;

    QVariantAnimation positionAnimation;
    QVariantAnimation sizeAnimation;
};

ContextMenu::Implementation::Implementation(QWidget* _parent)
    : content(new Tree(_parent))
{
    content->setRootIsDecorated(false);
    content->setScrollBarVisible(false);

    positionAnimation.setEasingCurve(QEasingCurve::OutQuint);
    positionAnimation.setDuration(240);
    sizeAnimation.setEasingCurve(QEasingCurve::OutQuint);
    sizeAnimation.setDuration(240);
    sizeAnimation.setStartValue(QSize(0,0));
}


// ****


ContextMenu::ContextMenu(QWidget* _parent)
    : Card(_parent),
      d(new Implementation(this))
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);
    hide();

    QHBoxLayout* popupLayout = new QHBoxLayout;
    popupLayout->setMargin({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(d->content);
    setLayoutReimpl(popupLayout);

    connect(d->content, &Tree::currentIndexChanged, this, &ContextMenu::clicked);

    connect(&d->positionAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        move(_value.toPoint());
    });
    connect(&d->sizeAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        resize(_value.toSize());
    });
    connect(&d->sizeAnimation, &QVariantAnimation::finished, this, [this] {
        //
        // После завершении анимации скрытия скрываем сам виджет контекстного меню
        //
        if (d->sizeAnimation.direction() == QAbstractAnimation::Backward) {
            hide();
        }
    });
}

void ContextMenu::setModel(QAbstractItemModel* _model)
{
    d->content->setModel(_model);
}

ContextMenu::~ContextMenu() = default;

void ContextMenu::showContextMenu(const QPoint& _pos)
{
    if (d->content->model()->rowCount() == 0) {
        return;
    }

    //
    // Блокируем сигналы дерева, чтобы оно не активировало свои элементы при отображении
    //
    QSignalBlocker signalBlocker(d->content);

    //
    // Установим невалидный текущий элемент
    //
    d->content->setCurrentIndex({});

    d->sizeAnimation.stop();
    d->sizeAnimation.setDirection(QVariantAnimation::Forward);
    const auto itemsCount = d->content->model()->rowCount();
    const auto height = Ui::DesignSystem::treeOneLineItem().height() * itemsCount
                        + Ui::DesignSystem::card().shadowMargins().top()
                        + Ui::DesignSystem::card().shadowMargins().bottom();
    const auto width = Ui::DesignSystem::card().shadowMargins().left()
                       + Ui::DesignSystem::treeOneLineItem().margins().left()
                       + Ui::DesignSystem::treeOneLineItem().iconSize().width()
                       + Ui::DesignSystem::treeOneLineItem().spacing()
                       + d->content->sizeHintForColumn(0)
                       + Ui::DesignSystem::treeOneLineItem().margins().right()
                       + Ui::DesignSystem::card().shadowMargins().right();
    d->sizeAnimation.setEndValue(QSizeF(width, height).toSize());
    resize(1, 1);
    //
    d->positionAnimation.stop();
    d->positionAnimation.setDirection(QVariantAnimation::Forward);
    auto position = _pos - QPointF(Ui::DesignSystem::card().shadowMargins().left(),
                                   Ui::DesignSystem::card().shadowMargins().top());
    auto endPosition = position;
    const auto screenGeometry = QApplication::screenAt(position.toPoint())->geometry();
    //
    // Если контекстное меню не помещается на экране справа от указателя
    //
    if (endPosition.x() + width > screenGeometry.right()) {
        position.setX(position.x() - this->width());
        endPosition.setX(endPosition.x()
                         - width
                         + Ui::DesignSystem::card().shadowMargins().left()
                         + Ui::DesignSystem::card().shadowMargins().right());
    }
    //
    // Если контекстное меню не помещается на экране снизу от указателя
    //
    if (endPosition.y() + height > screenGeometry.bottom()) {
        position.setY(position.y() - this->height());
        endPosition.setY(endPosition.y()
                         - height
                         + Ui::DesignSystem::card().shadowMargins().top()
                         + Ui::DesignSystem::card().shadowMargins().bottom());
    }
    d->positionAnimation.setStartValue(position);
    d->positionAnimation.setEndValue(endPosition);
    move(position.toPoint());

    //
    // NOTE: Если сделать размер 1х1, то на Windows будет моргать окошечко при появлении
    //

    //
    // Отображаем контекстное меню
    //
    show();

    d->positionAnimation.start();
    d->sizeAnimation.start();
}

void ContextMenu::hideContextMenu()
{
    d->positionAnimation.setDirection(QVariantAnimation::Backward);
    d->positionAnimation.start();
    d->sizeAnimation.setDirection(QVariantAnimation::Backward);
    d->sizeAnimation.start();
}

void ContextMenu::processBackgroundColorChange()
{
    Card::processBackgroundColorChange();

    d->content->setBackgroundColor(backgroundColor());
}

void ContextMenu::processTextColorChange()
{
    Card::processTextColorChange();

    d->content->setTextColor(textColor());
}
