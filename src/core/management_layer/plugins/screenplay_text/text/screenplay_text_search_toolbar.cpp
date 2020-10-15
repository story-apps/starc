#include "screenplay_text_search_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/tree/tree.h>

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QEvent>
#include <QKeyEvent>
#include <QStringListModel>
#include <QVariantAnimation>


namespace Ui
{

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
    QString lastSearchText; // чтобы защититься от флуда при активации окна
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


// ****


ScreenplayTextSearchToolbar::ScreenplayTextSearchToolbar(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation(this))
{
    _parent->installEventFilter(this);
    d->searchText->installEventFilter(this);
    d->replaceText->installEventFilter(this);
    setFocusProxy(d->searchText);

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
    connect(d->searchText, &TextField::textChanged, this, [this] {
        if (d->lastSearchText == d->searchText->text()) {
            return;
        }

        d->lastSearchText = d->searchText->text();
        emit findTextRequested();
    });
    //
    d->goToPreviousAction->setIconText(u8"\U000f0143");
    d->goToPreviousAction->setShortcut(QKeySequence::FindPrevious);
    addAction(d->goToPreviousAction);
    connect(d->goToPreviousAction, &QAction::triggered, this, &ScreenplayTextSearchToolbar::findPreviousRequested);
    d->goToNextAction->setIconText(u8"\U000f0140");
    d->goToNextAction->setShortcut(QKeySequence::FindNext);
    addAction(d->goToNextAction);
    connect(d->goToNextAction, &QAction::triggered, this, &ScreenplayTextSearchToolbar::findNextRequested);
    d->matchCaseAction->setIconText(u8"\U000f0b34");
    d->matchCaseAction->setCheckable(true);
    addAction(d->matchCaseAction);
    connect(d->matchCaseAction, &QAction::toggled, this, [this] {
        d->matchCaseAction->setToolTip(d->matchCaseAction->isChecked() ? tr("Search without case sensitive")
                                                                       : tr("Search with case sensitive"));
    });
    connect(d->matchCaseAction, &QAction::toggled, this, &ScreenplayTextSearchToolbar::findTextRequested);
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

        emit findTextRequested();
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
    connect(d->replace, &Button::clicked, this, &ScreenplayTextSearchToolbar::replaceOnePressed);
    addAction(d->replaceAllAction);
    connect(d->replaceAll, &Button::clicked, this, &ScreenplayTextSearchToolbar::replaceAllPressed);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTextSearchToolbar::~ScreenplayTextSearchToolbar() = default;

void ScreenplayTextSearchToolbar::refocus()
{
    d->searchText->setFocus();
}

QString ScreenplayTextSearchToolbar::searchText() const
{
    return d->lastSearchText;
}

bool ScreenplayTextSearchToolbar::isCaseSensitive() const
{
    return d->matchCaseAction->isChecked();
}

int ScreenplayTextSearchToolbar::searchInType() const
{
    return d->popupContent->currentIndex().row();
}

QString ScreenplayTextSearchToolbar::replaceText() const
{
    return d->replaceText->text();
}

bool ScreenplayTextSearchToolbar::eventFilter(QObject* _watched, QEvent* _event)
{
    switch (_event->type()) {
        case QEvent::Resize: {
            if (_watched == parent()) {
                designSystemChangeEvent(nullptr);
            }
            break;
        }

        case QEvent::FocusOut: {
            if ((QApplication::focusWidget() == nullptr
                 || QApplication::focusWidget()->parent() != this)
                && d->popup->isVisible()) {
                d->searchInAction->trigger();
            }
            break;
        }

        case QEvent::KeyPress: {
            if (_watched == d->searchText) {
                const auto keyEvent = static_cast<QKeyEvent*>(_event);
                if ((keyEvent->key() == Qt::Key_Enter
                     || keyEvent->key() == Qt::Key_Return)
                    && !d->searchText->text().isEmpty()) {
                    emit findTextRequested();
                }
            }
            break;
        }

        default: break;
    }

    return FloatingToolBar::eventFilter(_watched, _event);
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
    if (textFieldWidth < 0) {
        return;
    }

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
