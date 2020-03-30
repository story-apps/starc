#include "screenplay_text_edit_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/tree/tree.h>

#include <QAbstractItemModel>
#include <QAction>
#include <QHBoxLayout>
#include <QVariantAnimation>


namespace Ui
{

namespace {
    const char* kActionWidthKey = "action-width";
}

class ScreenplayTextEditToolBar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Показать попап
     */
    void showPopup(ScreenplayTextEditToolBar* _parent);

    /**
     * @brief Скрыть попап
     */
    void hidePopup();


    QAction* paragraphTypeAction = nullptr;
    QAction* fastFormatAction = nullptr;
    QAction* searchAction = nullptr;
    QAction* reviewAction = nullptr;

    QAction* expandToolBarAction = nullptr;
    QVariantAnimation widthAnimation;

    bool isPopupShown = false;
    Card* popup = nullptr;
    Tree* popupContent = nullptr;
    QVariantAnimation popupHeightAnimation;
};

ScreenplayTextEditToolBar::Implementation::Implementation(QWidget* _parent)
    : popup(new Card(_parent)),
      popupContent(new Tree(popup))
{
    popup->setWindowFlag(Qt::SplashScreen);
    popup->setAttribute(Qt::WA_Hover, false);
    popup->setAttribute(Qt::WA_TranslucentBackground);
    popup->setAttribute(Qt::WA_ShowWithoutActivating);
    popup->hide();

    popupContent->setRootIsDecorated(false);

    QHBoxLayout* popupLayout = new QHBoxLayout;
    popupLayout->setMargin({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(popupContent);
    popup->setLayoutReimpl(popupLayout);

    popupHeightAnimation.setEasingCurve(QEasingCurve::OutQuint);
    popupHeightAnimation.setDuration(240);
    popupHeightAnimation.setStartValue(0);
    popupHeightAnimation.setEndValue(0);
}

void ScreenplayTextEditToolBar::Implementation::showPopup(ScreenplayTextEditToolBar* _parent)
{
    if (popupContent->model() == nullptr) {
        return;
    }

    isPopupShown = true;

    const auto popupWidth = Ui::DesignSystem::floatingToolBar().spacing() * 2
                            + _parent->actionWidth(paragraphTypeAction);
    popup->resize(static_cast<int>(popupWidth), 0);

    const auto left = QPoint(Ui::DesignSystem::floatingToolBar().shadowMargins().left()
                             + Ui::DesignSystem::floatingToolBar().margins().left()
                             + Ui::DesignSystem::floatingToolBar().iconSize().width() * 2
                             + Ui::DesignSystem::floatingToolBar().spacing()
                             - Ui::DesignSystem::card().shadowMargins().left(),
                             _parent->rect().bottom()
                             - Ui::DesignSystem::floatingToolBar().shadowMargins().bottom());
    const auto pos = _parent->mapToGlobal(left)
                     + QPointF(Ui::DesignSystem::textField().margins().left(),
                               - Ui::DesignSystem::textField().margins().bottom());
    popup->move(pos.toPoint());
    popup->show();

    popupContent->setScrollBarVisible(false);

    popupHeightAnimation.setDirection(QVariantAnimation::Forward);
    const auto itemsCount = popupContent->model()->rowCount();
    const auto height = Ui::DesignSystem::treeOneLineItem().height() * itemsCount
                        + Ui::DesignSystem::card().shadowMargins().top()
                        + Ui::DesignSystem::card().shadowMargins().bottom();
    popupHeightAnimation.setEndValue(static_cast<int>(height));
    popupHeightAnimation.start();
}

void ScreenplayTextEditToolBar::Implementation::hidePopup()
{
    isPopupShown = false;

    popupHeightAnimation.setDirection(QVariantAnimation::Backward);
    popupHeightAnimation.start();
}


// ****


ScreenplayTextEditToolBar::ScreenplayTextEditToolBar(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation(this))
{
    QAction* undoAction = new QAction;
    undoAction->setIconText(u8"\uf54c");
    addAction(undoAction);

    QAction* redoAction = new QAction;
    redoAction->setIconText(u8"\uf44e");
    addAction(redoAction);

    d->paragraphTypeAction = new QAction;
    d->paragraphTypeAction->setText(tr("Scene heading"));
    d->paragraphTypeAction->setIconText(u8"\uf35d");
    addAction(d->paragraphTypeAction);
    connect(d->paragraphTypeAction, &QAction::triggered, this, [this] {
        if (!d->isPopupShown) {
            d->paragraphTypeAction->setIconText(u8"\uf360");
            d->showPopup(this);
        } else {
            d->paragraphTypeAction->setIconText(u8"\uf35d");
            d->hidePopup();
        }
    });

    d->fastFormatAction = new QAction;
    d->fastFormatAction->setIconText(u8"\uf328");
    d->fastFormatAction->setCheckable(true);
    d->fastFormatAction->setVisible(false);
    addAction(d->fastFormatAction);

    d->searchAction = new QAction;
    d->searchAction->setIconText(u8"\uf349");
    d->searchAction->setCheckable(true);
    d->searchAction->setVisible(false);
    addAction(d->searchAction);

    d->reviewAction = new QAction;
    d->reviewAction->setIconText(u8"\ufe14");
    d->reviewAction->setCheckable(true);
    d->reviewAction->setVisible(false);
    addAction(d->reviewAction);

    d->expandToolBarAction = new QAction;
    d->expandToolBarAction->setIconText(u8"\uf1d9");
    addAction(d->expandToolBarAction);
    connect(d->expandToolBarAction, &QAction::triggered, this, [this] {
        d->expandToolBarAction->setVisible(false);
        d->fastFormatAction->setVisible(true);
        d->searchAction->setVisible(true);
        d->reviewAction->setVisible(true);

        d->widthAnimation.stop();
        d->widthAnimation.setDirection(QVariantAnimation::Forward);
        d->widthAnimation.setEasingCurve(QEasingCurve::OutQuad);
        d->widthAnimation.start();
    });
    d->widthAnimation.setDuration(120);
    d->widthAnimation.setEasingCurve(QEasingCurve::OutQuad);
    connect(&d->widthAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        resize(_value.toInt(), height());
    });

    connect(&d->popupHeightAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        const auto height = _value.toInt();
        d->popup->resize(d->popup->width(), height);
    });
    connect(&d->popupHeightAnimation, &QVariantAnimation::finished, this, [this] {
        if (!d->isPopupShown) {
            d->popup->hide();
        }
    });

    connect(d->popupContent, &Tree::currentIndexChanged, this, [this] (const QModelIndex& _index) {
        d->paragraphTypeAction->setText(_index.data().toString());
        d->hidePopup();
        update();

        emit paragraphTypeChanged(_index);
    });

    designSystemChangeEvent(nullptr);
}

void ScreenplayTextEditToolBar::setParagraphTypesModel(QAbstractItemModel* _model)
{
    if (d->popupContent->model() != nullptr) {
        d->popupContent->model()->disconnect(this);
    }

    d->popupContent->setModel(_model);

    if (_model != nullptr) {
        connect(_model, &QAbstractItemModel::rowsInserted, this, [this] {
            designSystemChangeEvent(nullptr);
        });
        if (_model->rowCount() > 0) {
            d->popupContent->setCurrentIndex(_model->index(0, 0));
        }
    }

    //
    // Обновим внешний вид, чтобы пересчитать ширину элемента с выбором стилей
    //
    designSystemChangeEvent(nullptr);
}

void ScreenplayTextEditToolBar::setCurrentParagraphType(const QModelIndex& _index)
{
    d->paragraphTypeAction->setText(_index.data().toString());
    d->popupContent->setCurrentIndex(_index);
}

ScreenplayTextEditToolBar::~ScreenplayTextEditToolBar() = default;

void ScreenplayTextEditToolBar::leaveEvent(QEvent* _event)
{
    FloatingToolBar::leaveEvent(_event);

    if (d->expandToolBarAction->isVisible()) {
        return;
    }

    d->expandToolBarAction->setVisible(true);
    d->fastFormatAction->setVisible(false);
    d->searchAction->setVisible(false);
    d->reviewAction->setVisible(false);

    d->widthAnimation.stop();
    d->widthAnimation.setDirection(QVariantAnimation::Backward);
    d->widthAnimation.setEasingCurve(QEasingCurve::InQuad);
    d->widthAnimation.start();
}

void ScreenplayTextEditToolBar::focusOutEvent(QFocusEvent* _event)
{
    FloatingToolBar::focusOutEvent(_event);

    d->paragraphTypeAction->setIconText(u8"\uf35d");
    d->hidePopup();
}

void ScreenplayTextEditToolBar::updateTranslations()
{

}

void ScreenplayTextEditToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    setActionWidth(d->paragraphTypeAction,
                   static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().left())
                   + d->popupContent->sizeHintForColumn(0)
                   + static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().right()));

    const qreal additionalWidth = [this] {
        const QFontMetricsF fontMetrics(Ui::DesignSystem::font().body1());
        qreal width = 0.0;
        for (const auto action : actions()) {
            const auto actionCustomWidth = actionWidth(action);
            if (actionCustomWidth > 0) {
                width += actionCustomWidth;
                width -= Ui::DesignSystem::floatingToolBar().iconSize().width();
            }
        }
        return width;
    }();
    auto findWidth = [additionalWidth] (int _iconsSize) {
        return Ui::DesignSystem::floatingToolBar().shadowMargins().left()
                + Ui::DesignSystem::floatingToolBar().margins().left()
                + Ui::DesignSystem::floatingToolBar().iconSize().width() * _iconsSize
                + Ui::DesignSystem::floatingToolBar().spacing() * (_iconsSize - 1)
                + Ui::DesignSystem::floatingToolBar().margins().right()
                + Ui::DesignSystem::floatingToolBar().shadowMargins().right()
                + additionalWidth;
    };

    const auto minimumWidth = findWidth(4);
    const auto maximumWidth = findWidth(6);
    d->widthAnimation.setStartValue(minimumWidth);
    d->widthAnimation.setEndValue(maximumWidth);

    d->popup->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->popupContent->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->popupContent->setTextColor(Ui::DesignSystem::color().onPrimary());

    resize(sizeHint());
}

} // namespace Ui
