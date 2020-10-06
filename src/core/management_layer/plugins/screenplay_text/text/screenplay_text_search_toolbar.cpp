#include "screenplay_text_search_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/tree/tree.h>

#include <QAction>
#include <QHBoxLayout>
#include <QEvent>
#include <QPainter>
#include <QStringListModel>
#include <QVariantAnimation>


namespace Ui
{

class ToolbarAnimationWrapper::Implementation {
public:
    QPointF sourceIconPosition;
    QWidget* sourceWidget = nullptr;
    QPixmap sourceWidgetImage;
    QWidget* targetWidget = nullptr;
    QPixmap targetWidgetImage;

    QVariantAnimation geometryAnimation;
    QVariantAnimation iconPositionAnimation;
    QVariantAnimation opacityAnimation;
};

// **

ToolbarAnimationWrapper::ToolbarAnimationWrapper(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation)
{
    hide();

    d->geometryAnimation.setEasingCurve(QEasingCurve::OutQuad);
    d->geometryAnimation.setDuration(240);
    connect(&d->geometryAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        setGeometry(_value.toRect());
    });
    connect(&d->geometryAnimation, &QVariantAnimation::finished, this, [this] {
        hide();
        if (d->geometryAnimation.direction() == QVariantAnimation::Forward) {
            d->targetWidget->show();
            d->targetWidget->setFocus();
        } else {
            d->sourceWidget->show();
        }
    });

    d->iconPositionAnimation.setEasingCurve(QEasingCurve::OutQuad);
    d->iconPositionAnimation.setDuration(240);
    connect(&d->iconPositionAnimation, &QVariantAnimation::valueChanged, this, [this] {
        update();
    });

    d->opacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    d->opacityAnimation.setDuration(240);
    d->opacityAnimation.setStartValue(1.0);
    d->opacityAnimation.setEndValue(0.0);
    connect(&d->opacityAnimation, &QVariantAnimation::valueChanged, this, [this] {
        update();
    });
}

ToolbarAnimationWrapper::~ToolbarAnimationWrapper() = default;

void ToolbarAnimationWrapper::animateToolbarShowing(const QPointF _sourceIconPosition, QWidget* _sourceWidget, QWidget* _targetWidget)
{
    d->sourceIconPosition = _sourceIconPosition;
    d->sourceWidget = _sourceWidget;
    d->targetWidget = _targetWidget;

    d->sourceWidgetImage = d->sourceWidget->grab();
    d->targetWidget->show();
    d->targetWidgetImage = d->targetWidget->grab();
    d->targetWidget->hide();

    d->geometryAnimation.setStartValue(d->sourceWidget->geometry());
    d->geometryAnimation.setEndValue(d->targetWidget->geometry());
    d->iconPositionAnimation.setStartValue(_sourceIconPosition);
    d->iconPositionAnimation.setEndValue(QPointF(Ui::DesignSystem::floatingToolBar().shadowMargins().left()
                                                 + Ui::DesignSystem::floatingToolBar().margins().left(),
                                                 Ui::DesignSystem::floatingToolBar().shadowMargins().top()
                                                 + Ui::DesignSystem::floatingToolBar().margins().top()));

    setGeometry(d->sourceWidget->geometry());
    show();
    raise();
    d->sourceWidget->hide();

    d->geometryAnimation.setDirection(QVariantAnimation::Forward);
    d->geometryAnimation.start();
    d->iconPositionAnimation.setDirection(QVariantAnimation::Forward);
    d->iconPositionAnimation.start();
    d->opacityAnimation.setDirection(QVariantAnimation::Forward);
    d->opacityAnimation.start();
}

void ToolbarAnimationWrapper::animateToolbarHiding()
{
    d->sourceWidget->show();
    d->sourceWidgetImage = d->sourceWidget->grab();
    d->sourceWidget->hide();
    d->targetWidgetImage = d->targetWidget->grab();

    d->geometryAnimation.setStartValue(d->sourceWidget->geometry());
    d->geometryAnimation.setEndValue(d->targetWidget->geometry());

    setGeometry(d->targetWidget->geometry());
    show();
    raise();
    d->targetWidget->hide();

    d->geometryAnimation.setDirection(QVariantAnimation::Backward);
    d->geometryAnimation.start();
    d->iconPositionAnimation.setDirection(QVariantAnimation::Backward);
    d->iconPositionAnimation.start();
    d->opacityAnimation.setDirection(QVariantAnimation::Backward);
    d->opacityAnimation.start();
}

void ToolbarAnimationWrapper::paintEvent(QPaintEvent* _event)
{
    FloatingToolBar::paintEvent(_event);

    QPainter painter(this);
    const auto leftMargin = Ui::DesignSystem::floatingToolBar().shadowMargins().left()
                            + Ui::DesignSystem::floatingToolBar().margins().left();
    const auto rightMargin = Ui::DesignSystem::floatingToolBar().shadowMargins().right()
                             + Ui::DesignSystem::floatingToolBar().margins().right();
    const auto topMargin = Ui::DesignSystem::floatingToolBar().shadowMargins().top();
    const auto bottomMargin = Ui::DesignSystem::floatingToolBar().shadowMargins().bottom();

    const auto iconRect = QRectF(d->iconPositionAnimation.currentValue().toPointF(),
                                 Ui::DesignSystem::floatingToolBar().iconSize());
    painter.drawPixmap(iconRect.left(), topMargin,
                       d->targetWidgetImage,
                       leftMargin, topMargin,
                       width() - iconRect.left() - rightMargin,
                       height() - topMargin - bottomMargin);

    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.setPen(textColor());
    painter.drawText(iconRect, Qt::AlignCenter, u8"\U000f004d");

    painter.setOpacity(d->opacityAnimation.currentValue().toReal());
    painter.drawPixmap(leftMargin, topMargin,
                       d->sourceWidgetImage,
                       leftMargin, topMargin,
                       d->sourceWidgetImage.width() - leftMargin - rightMargin,
                       d->sourceWidgetImage.height() - topMargin - bottomMargin);

    painter.setOpacity(d->opacityAnimation.currentValue().toReal());
    painter.fillRect(iconRect, backgroundColor());
    painter.drawText(iconRect, Qt::AlignCenter, u8"\U000f0349");
}


// ****


class ScreenplayTextSearchToolbar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Показать попап
     */
    void showPopup(ScreenplayTextSearchToolbar* _parent);

    /**
     * @brief Скрыть попап
     */
    void hidePopup();


    QAction* closeAction = nullptr;

    QAction* searchTextAction = nullptr;
    TextField* searchText = nullptr;
    QAction* goToNextAction = nullptr;
    QAction* goToPreviousAction = nullptr;
    QAction* matchCaseAction = nullptr;
    QAction* searchInAction = nullptr;

    bool isPopupShown = false;
    Card* popup = nullptr;
    Tree* popupContent = nullptr;
    QVariantAnimation popupHeightAnimation;

    QAction* replaceTextAction = nullptr;
    TextField* replaceText = nullptr;
    QAction* replaceAction = nullptr;
    Button* replace = nullptr;
    QAction* replaceAllAction = nullptr;
    Button* replaceAll = nullptr;

};

ScreenplayTextSearchToolbar::Implementation::Implementation(QWidget* _parent)
    : closeAction(new QAction),
      searchTextAction(new QAction),
      searchText(new TextField(_parent)),
      goToNextAction(new QAction),
      goToPreviousAction(new QAction),
      matchCaseAction(new QAction),
      searchInAction(new QAction),
      popup(new Card(_parent)),
      popupContent(new Tree(popup)),
      replaceTextAction(new QAction),
      replaceText(new TextField(_parent)),
      replaceAction(new QAction),
      replace(new Button(_parent)),
      replaceAllAction(new QAction),
      replaceAll(new Button(_parent))
{
    searchText->setUnderlineDecorationVisible(false);

    popup->setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    popup->setAttribute(Qt::WA_Hover, false);
    popup->setAttribute(Qt::WA_TranslucentBackground);
    popup->setAttribute(Qt::WA_ShowWithoutActivating);
    popup->hide();

    popupContent->setRootIsDecorated(false);

    auto popupLayout = new QHBoxLayout;
    popupLayout->setMargin({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(popupContent);
    popup->setLayoutReimpl(popupLayout);

    popupHeightAnimation.setEasingCurve(QEasingCurve::OutQuint);
    popupHeightAnimation.setDuration(240);
    popupHeightAnimation.setStartValue(0);
    popupHeightAnimation.setEndValue(0);

    replaceText->setUnderlineDecorationVisible(false);

    replace->setFocusPolicy(Qt::NoFocus);
    replaceAll->setFocusPolicy(Qt::NoFocus);
}

void ScreenplayTextSearchToolbar::Implementation::showPopup(ScreenplayTextSearchToolbar* _parent)
{
    if (popupContent->model() == nullptr) {
        return;
    }

    isPopupShown = true;

    const auto popupWidth = Ui::DesignSystem::floatingToolBar().spacing() * 2
                            + _parent->actionCustomWidth(searchInAction);
    popup->resize(static_cast<int>(popupWidth), 0);

    const auto left = QPoint(Ui::DesignSystem::floatingToolBar().shadowMargins().left()
                             + Ui::DesignSystem::floatingToolBar().margins().left()
                             + searchText->width() + Ui::DesignSystem::floatingToolBar().spacing()
                             + (Ui::DesignSystem::floatingToolBar().iconSize().width()
                                + Ui::DesignSystem::floatingToolBar().spacing()) * 4
                             - Ui::DesignSystem::floatingToolBar().spacing()
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

void ScreenplayTextSearchToolbar::Implementation::hidePopup()
{
    isPopupShown = false;

    popupHeightAnimation.setDirection(QVariantAnimation::Backward);
    popupHeightAnimation.start();
}

// **

ScreenplayTextSearchToolbar::ScreenplayTextSearchToolbar(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation(this))
{
    _parent->installEventFilter(this);

    d->closeAction->setIconText(u8"\U000f004d");
    d->closeAction->setShortcut(QKeySequence::Find);
    addAction(d->closeAction);
    connect(d->closeAction, &QAction::triggered, this, [this] {
        if (d->searchText->hasFocus()) {
            emit closePressed();
        } else {
            d->searchText->setFocus();
            d->searchText->selectAll();
        }
    });
    //
    addAction(d->searchTextAction);
    //
    d->goToNextAction->setIconText(u8"\U000f0140");
    d->goToNextAction->setShortcut(QKeySequence::FindNext);
    addAction(d->goToNextAction);
    connect(d->goToNextAction, &QAction::triggered, this, &ScreenplayTextSearchToolbar::goToNextPressed);
    d->goToPreviousAction->setIconText(u8"\U000f0143");
    d->goToPreviousAction->setShortcut(QKeySequence::FindPrevious);
    addAction(d->goToPreviousAction);
    connect(d->goToPreviousAction, &QAction::triggered, this, &ScreenplayTextSearchToolbar::goToNextPressed);
    d->matchCaseAction->setIconText(u8"\U000f0b34");
    d->matchCaseAction->setCheckable(true);
    addAction(d->matchCaseAction);
    connect(d->matchCaseAction, &QAction::toggled, this, [this] {
        d->matchCaseAction->setToolTip(d->matchCaseAction->isChecked() ? tr("Search without case sensitive")
                                                                       : tr("Search with case sensitive"));
    });
    connect(d->matchCaseAction, &QAction::toggled, this, &ScreenplayTextSearchToolbar::goToNextPressed);
    //
    d->searchInAction->setText(tr("In the whole text"));
    d->searchInAction->setIconText(u8"\U000f035d");
    auto _model = new QStringListModel(d->popupContent);
    d->popupContent->setModel(_model);
    connect(_model, &QAbstractItemModel::rowsInserted, this, [this] {
        designSystemChangeEvent(nullptr);
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
        d->searchInAction->setText(_index.data().toString());
        d->hidePopup();
        update();

//        emit paragraphTypeChanged(_index);
    });
    //
    addAction(d->searchInAction);
    connect(d->searchInAction, &QAction::triggered, this, [this] {
        if (!d->isPopupShown) {
            d->searchInAction->setIconText(u8"\U000f0360");
            d->showPopup(this);
        } else {
            d->searchInAction->setIconText(u8"\U000f035d");
            d->hidePopup();
        }
    });

    addAction(d->replaceTextAction);
    addAction(d->replaceAction);
    addAction(d->replaceAllAction);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

bool ScreenplayTextSearchToolbar::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == parent() && _event->type() == QEvent::Resize) {
        designSystemChangeEvent(nullptr);
    }

    return FloatingToolBar::eventFilter(_watched, _event);
}

ScreenplayTextSearchToolbar::~ScreenplayTextSearchToolbar() = default;

void ScreenplayTextSearchToolbar::focusInEvent(QFocusEvent* _event)
{
    FloatingToolBar::focusInEvent(_event);

    d->searchText->setFocus();
    d->searchText->selectAll();
}

void ScreenplayTextSearchToolbar::focusOutEvent(QFocusEvent* _event)
{
    FloatingToolBar::focusOutEvent(_event);

    d->searchInAction->setIconText(u8"\U000f035d");
    d->hidePopup();
}

void ScreenplayTextSearchToolbar::processBackgroundColorChange()
{
    d->searchText->setBackgroundColor(backgroundColor());
    d->replaceText->setBackgroundColor(backgroundColor());
    d->replace->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->replaceAll->setBackgroundColor(Ui::DesignSystem::color().secondary());
}

void ScreenplayTextSearchToolbar::processTextColorChange()
{
    d->searchText->setTextColor(textColor());
    d->replaceText->setTextColor(textColor());
    d->replace->setTextColor(Ui::DesignSystem::color().secondary());
    d->replaceAll->setTextColor(Ui::DesignSystem::color().secondary());
}

void ScreenplayTextSearchToolbar::updateTranslations()
{
    d->closeAction->setToolTip(tr("Exit from search")
                                + QString(" (%1)").arg(QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText)));
    d->searchText->setLabel(tr("Search"));
    d->searchText->setPlaceholderText(tr("Enter search phrase here"));
    if (auto model = qobject_cast<QStringListModel*>(d->popupContent->model())) {
        model->setStringList({ tr("In the whole text"),
                               tr("In scene heading"),
                               tr("In action"),
                               tr("In character"),
                               tr("In dialogue") });
        d->popupContent->setCurrentIndex(model->index(0, 0));
    }
    d->goToNextAction->setToolTip(tr("Go to the next search result")
                                  + QString(" (%1)").arg(QKeySequence(QKeySequence::FindNext).toString(QKeySequence::NativeText)));
    d->goToPreviousAction->setToolTip(tr("Go to the previous search result")
                                      + QString(" (%1)").arg(QKeySequence(QKeySequence::FindPrevious).toString(QKeySequence::NativeText)));
    d->matchCaseAction->setToolTip(d->matchCaseAction->isChecked() ? tr("Search without case sensitive")
                                                                   : tr("Search with case sensitive"));

    d->replaceText->setLabel(tr("Replace with"));
    d->replaceText->setPlaceholderText(tr("Enter phrase to replace"));
    d->replace->setText(tr("Replace"));
    d->replaceAll->setText(tr("All"));
}

void ScreenplayTextSearchToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    //
    // Рассчитываем размер полей поиска и замены
    //
    const auto searchInActionWidth = Ui::DesignSystem::treeOneLineItem().margins().left()
                                     + d->popupContent->sizeHintForColumn(0)
                                     + Ui::DesignSystem::treeOneLineItem().margins().right();
    d->replace->resize(d->replace->sizeHint());
    const auto replaceActionWidth = d->replace->sizeHint().width() - Ui::DesignSystem::floatingToolBar().spacing();
    d->replaceAll->resize(d->replaceAll->sizeHint());
    const auto replaceAllActionWidth = d->replaceAll->sizeHint().width() - Ui::DesignSystem::floatingToolBar().spacing();
    auto textFieldWidth = parentWidget()->width() * 0.8;
    textFieldWidth -= (Ui::DesignSystem::floatingToolBar().iconSize().width()
                       + Ui::DesignSystem::floatingToolBar().spacing()) * 4 // 4 обычных кнопки
                      + searchInActionWidth // выпадающий список мест поиска
                      + replaceActionWidth // кнопка единичной замены
                      + replaceAllActionWidth; // кнопка полной замены
    textFieldWidth /= 2; // делим поровну

    //
    // Расставляем элементы в панели
    //
    setActionCustomWidth(d->searchTextAction, textFieldWidth);
    d->searchText->setFixedWidth(textFieldWidth);
    const auto searchLeft = Ui::DesignSystem::floatingToolBar().shadowMargins().left()
                            + Ui::DesignSystem::floatingToolBar().iconSize().width()
                            + Ui::DesignSystem::floatingToolBar().spacing();
    d->searchText->move(searchLeft, Ui::DesignSystem::floatingToolBar().shadowMargins().top());
    //
    setActionCustomWidth(d->searchInAction, static_cast<int>(searchInActionWidth));
    //
    d->popup->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->popupContent->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->popupContent->setTextColor(Ui::DesignSystem::color().onPrimary());


    const auto replaceLeft = searchLeft + d->searchText->width() + Ui::DesignSystem::floatingToolBar().spacing()
                             + (Ui::DesignSystem::floatingToolBar().iconSize().width()
                                + Ui::DesignSystem::floatingToolBar().spacing()) * 3
                             + actionCustomWidth(d->searchInAction) + Ui::DesignSystem::floatingToolBar().spacing();

    setActionCustomWidth(d->replaceTextAction, textFieldWidth);
    d->replaceText->setFixedWidth(textFieldWidth);
    d->replaceText->move(replaceLeft, Ui::DesignSystem::floatingToolBar().shadowMargins().top());
    //
    setActionCustomWidth(d->replaceAction, replaceActionWidth);
    d->replace->move(d->replaceText->geometry().right() + Ui::DesignSystem::floatingToolBar().spacing(),
                     Ui::DesignSystem::floatingToolBar().shadowMargins().top());
    setActionCustomWidth(d->replaceAllAction, replaceAllActionWidth);
    d->replaceAll->move(d->replace->geometry().right()
                        /*+ Ui::DesignSystem::floatingToolBar().spacing() / 2*/, // тут ручками подобрал, чтобы красиво было
                        Ui::DesignSystem::floatingToolBar().shadowMargins().top());

    resize(sizeHint());
}

} // namespace Ui
