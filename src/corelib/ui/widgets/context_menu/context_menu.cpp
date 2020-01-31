#include "context_menu.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/tree/tree.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QTimer>
#include <QVariantAnimation>


class ContextMenu::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Tree* content = nullptr;

    QVariantAnimation sizeAnimation;
};

ContextMenu::Implementation::Implementation(QWidget* _parent)
    : content(new Tree(_parent))
{
    content->setRootIsDecorated(false);
    content->setScrollBarVisible(false);

    sizeAnimation.setEasingCurve(QEasingCurve::OutQuint);
    sizeAnimation.setDuration(240);
    sizeAnimation.setStartValue(QSize(0, 0));
    sizeAnimation.setEndValue(QSize(0, 0));
}


// ****


ContextMenu::ContextMenu(QWidget* _parent)
    : Card(_parent),
      d(new Implementation(this))
{
    setWindowFlag(Qt::SplashScreen);
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

    connect(&d->sizeAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        resize(_value.toSize());
    });
    connect(&d->sizeAnimation, &QVariantAnimation::finished, this, [this] {
        //
        // После завершении анимации отображения активируем виджет контекстного меню
        //
        if (d->sizeAnimation.direction() == QAbstractAnimation::Forward) {
            //
            // Блокируем сигналы дерева, чтобы оно не активировало свои элементы при отображении
            //
            QSignalBlocker signalBlocker(d->content);

            //
            // Активируем виджет
            //
            QApplication::setActiveWindow(this);
            setFocus();

            //
            // Установим невалидный текущий элемент
            //
            d->content->setCurrentIndex({});
        }
        //
        // После завершении анимации скрытия скрываем сам виджет контекстного меню
        //
        else {
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

    resize(0, 0);
    const auto pos = _pos - QPoint(Ui::DesignSystem::card().shadowMargins().left(),
                                   Ui::DesignSystem::card().shadowMargins().top());
    move(pos);
    show();

    d->sizeAnimation.stop();
    d->sizeAnimation.setDirection(QVariantAnimation::Forward);
    const auto itemsCount = d->content->model()->rowCount();
    const auto height = Ui::DesignSystem::treeOneLineItem().height() * itemsCount
                        + Ui::DesignSystem::card().shadowMargins().top()
                        + Ui::DesignSystem::card().shadowMargins().bottom();
    const auto width = d->content->sizeHint().width();
    d->sizeAnimation.setEndValue(QSizeF(width, height).toSize());
    d->sizeAnimation.start();
}

void ContextMenu::hideContextMenu()
{
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

bool ContextMenu::event(QEvent* _event)
{
    if (_event->type() == QEvent::WindowDeactivate) {
        hideContextMenu();
    }

    return Card::event(_event);
}
