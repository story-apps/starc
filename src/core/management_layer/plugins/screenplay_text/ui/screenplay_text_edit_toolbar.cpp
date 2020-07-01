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


    QAction* undoAction = nullptr;
    QAction* redoAction = nullptr;
    QAction* paragraphTypeAction = nullptr;
    QAction* fastFormatAction = nullptr;
    QAction* searchAction = nullptr;
    QAction* reviewAction = nullptr;

    bool isPopupShown = false;
    Card* popup = nullptr;
    Tree* popupContent = nullptr;
    QVariantAnimation popupHeightAnimation;
};

ScreenplayTextEditToolBar::Implementation::Implementation(QWidget* _parent)
    : popup(new Card(_parent)),
      popupContent(new Tree(popup))
{
    popup->setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
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
    d->undoAction = new QAction;
    d->undoAction->setIconText(u8"\U000f054c");
    addAction(d->undoAction);

    d->redoAction = new QAction;
    d->redoAction->setIconText(u8"\U000f044e");
    addAction(d->redoAction);

    d->paragraphTypeAction = new QAction;
    d->paragraphTypeAction->setText(tr("Scene heading"));
    d->paragraphTypeAction->setIconText(u8"\U000f035d");
    addAction(d->paragraphTypeAction);
    connect(d->paragraphTypeAction, &QAction::triggered, this, [this] {
        if (!d->isPopupShown) {
            d->paragraphTypeAction->setIconText(u8"\U000f0360");
            d->showPopup(this);
        } else {
            d->paragraphTypeAction->setIconText(u8"\U000f035d");
            d->hidePopup();
        }
    });

    d->fastFormatAction = new QAction;
    d->fastFormatAction->setIconText(u8"\U000f0328");
    d->fastFormatAction->setCheckable(true);
    addAction(d->fastFormatAction);
    connect(d->fastFormatAction, &QAction::toggled, this, &ScreenplayTextEditToolBar::updateTranslations);
    connect(d->fastFormatAction, &QAction::toggled, this, &ScreenplayTextEditToolBar::fastFormatPanelVisibleChanged);
    connect(d->fastFormatAction, &QAction::toggled, [this] (bool _checked) {
        d->paragraphTypeAction->setVisible(!_checked);
        designSystemChangeEvent(nullptr);
    });

    d->searchAction = new QAction;
    d->searchAction->setIconText(u8"\U000f0349");
    d->searchAction->setCheckable(true);
    addAction(d->searchAction);

    d->reviewAction = new QAction;
    d->reviewAction->setIconText(u8"\U000f0e31");
    d->reviewAction->setCheckable(true);
    addAction(d->reviewAction);
    connect(d->reviewAction, &QAction::toggled, this, &ScreenplayTextEditToolBar::updateTranslations);

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

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTextEditToolBar::~ScreenplayTextEditToolBar() = default;

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

void ScreenplayTextEditToolBar::setParagraphTypesEnabled(bool _enabled)
{
    d->paragraphTypeAction->setEnabled(_enabled);
}

bool ScreenplayTextEditToolBar::isFastFormatPanelVisible() const
{
    return d->fastFormatAction->isChecked();
}

void ScreenplayTextEditToolBar::focusOutEvent(QFocusEvent* _event)
{
    FloatingToolBar::focusOutEvent(_event);

    d->paragraphTypeAction->setIconText(u8"\U000f035d");
    d->hidePopup();
}

void ScreenplayTextEditToolBar::updateTranslations()
{
    d->undoAction->setToolTip(tr("Undo last action"));
    d->redoAction->setToolTip(tr("Redo last action"));
    d->paragraphTypeAction->setToolTip(tr("Current paragraph format"));
    d->fastFormatAction->setToolTip(d->fastFormatAction->isChecked() ? tr("Hide fast format panel")
                                                                     : tr("Show fast format panel"));
    d->searchAction->setToolTip(tr("Search text"));
    d->reviewAction->setToolTip(d->reviewAction->isChecked() ? tr("Disable review mode")
                                                             : tr("Enable review mode"));
}

void ScreenplayTextEditToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    setActionWidth(d->paragraphTypeAction,
                   static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().left())
                   + d->popupContent->sizeHintForColumn(0)
                   + static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().right()));

    d->popup->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->popupContent->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->popupContent->setTextColor(Ui::DesignSystem::color().onPrimary());

    resize(sizeHint());
}

} // namespace Ui
