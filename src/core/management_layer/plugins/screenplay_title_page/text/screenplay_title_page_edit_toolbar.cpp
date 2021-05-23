#include "screenplay_title_page_edit_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/tree/tree.h>

#include <utils/helpers/text_helper.h>

#include <QtMath>
#include <QAction>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QStringListModel>
#include <QVariantAnimation>


namespace Ui
{

class ScreenplayTitlePageEditToolbar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Показать попап
     */
    void showPopup(ScreenplayTitlePageEditToolbar* _parent, QAction* _forAction);

    /**
     * @brief Скрыть попап
     */
    void hidePopup();


    QAction* undoAction = nullptr;
    QAction* redoAction = nullptr;
    QAction* textFontAction = nullptr;
    QAction* textFontSizeAction = nullptr;

    QStringListModel fontsModel;
    QStringListModel fontSizesModel;

    bool isPopupShown = false;
    Card* popup = nullptr;
    Tree* popupContent = nullptr;
    QVariantAnimation popupHeightAnimation;
};

ScreenplayTitlePageEditToolbar::Implementation::Implementation(QWidget* _parent)
    : undoAction(new QAction),
      redoAction(new QAction),
      textFontAction(new QAction),
      textFontSizeAction(new QAction),
      popup(new Card(_parent)),
      popupContent(new Tree(popup))
{
    fontsModel.setStringList(QFontDatabase().families());
    fontSizesModel.setStringList({ "8","9","10","11","12","14","18","24","30","36","48","60","72","96" });

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
}

void ScreenplayTitlePageEditToolbar::Implementation::showPopup(
    ScreenplayTitlePageEditToolbar* _parent, QAction* _forAction)
{
    if (popupContent->model() == nullptr) {
        return;
    }

    isPopupShown = true;

    const auto popupWidth = Ui::DesignSystem::floatingToolBar().spacing() * 2
                            + _parent->actionCustomWidth(_forAction);
    popup->resize(static_cast<int>(popupWidth), 0);

    const auto left = QPoint(Ui::DesignSystem::floatingToolBar().shadowMargins().left()
                             + Ui::DesignSystem::floatingToolBar().margins().left()
                             + Ui::DesignSystem::floatingToolBar().iconSize().width() * 2
                             //
                             // Тут костылик, чтобы понимать, что нужно дополнительное смещение для попапа с размером шрифта
                             //
                             + (_forAction == textFontSizeAction
                                ? Ui::DesignSystem::floatingToolBar().spacing()
                                  + _parent->actionCustomWidth(textFontAction)
                                : 0)
                             //
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
    const auto height = Ui::DesignSystem::treeOneLineItem().height() * std::min(itemsCount, 12)
                        + Ui::DesignSystem::card().shadowMargins().top()
                        + Ui::DesignSystem::card().shadowMargins().bottom();
    popupHeightAnimation.setEndValue(static_cast<int>(height));
    popupHeightAnimation.start();
}

void ScreenplayTitlePageEditToolbar::Implementation::hidePopup()
{
    isPopupShown = false;

    popupHeightAnimation.setDirection(QVariantAnimation::Backward);
    popupHeightAnimation.start();
}


// ****


ScreenplayTitlePageEditToolbar::ScreenplayTitlePageEditToolbar(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation(this))
{
    d->undoAction->setIconText(u8"\U000f054c");
    addAction(d->undoAction);
    connect(d->undoAction, &QAction::triggered, this, &ScreenplayTitlePageEditToolbar::undoPressed);

    d->redoAction->setIconText(u8"\U000f044e");
    addAction(d->redoAction);
    connect(d->redoAction, &QAction::triggered, this, &ScreenplayTitlePageEditToolbar::redoPressed);

    //
    // FIXME: дефолтный шрифт из шаблона
    //
    const QFont defaultFont;
    d->textFontAction->setText(defaultFont.family());
    d->textFontAction->setIconText(u8"\U000f035d");
    addAction(d->textFontAction);
    d->textFontSizeAction->setText(QString::number(defaultFont.pointSize()));
    d->textFontSizeAction->setIconText(u8"\U000f035d");
    addAction(d->textFontSizeAction);
    auto activatePopup = [this] (QAction* _action, QStringListModel* _model) {
        if (!d->isPopupShown) {
            d->popupContent->setModel(_model);
            {
//                QSignalBlocker signalBlocker(this);
//                const int currentItemRow = _model->stringList().indexOf(_action->text());
//                d->popupContent->setCurrentIndex(_model->index(currentItemRow, 0));
            }
            _action->setIconText(u8"\U000f0360");
            d->showPopup(this, _action);
        } else {
            _action->setIconText(u8"\U000f035d");
            d->hidePopup();
        }
    };
    connect(d->textFontAction, &QAction::triggered, this, [this, activatePopup] {
        activatePopup(d->textFontAction, &d->fontsModel);
    });
    connect(d->textFontSizeAction, &QAction::triggered, this, [this, activatePopup] {
        activatePopup(d->textFontSizeAction, &d->fontSizesModel);
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
        if (d->popupContent->model() == &d->fontsModel) {
            d->textFontAction->setText(_index.data().toString());
        } else {
            d->textFontSizeAction->setText(_index.data().toString());
        }
        d->hidePopup();
        update();

        QFont newFont(d->textFontAction->text());
        newFont.setPixelSize(PageMetrics::ptToPx(d->textFontSizeAction->text().toInt()));
        emit fontChanged(newFont);
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ScreenplayTitlePageEditToolbar::~ScreenplayTitlePageEditToolbar() = default;

void ScreenplayTitlePageEditToolbar::setCurrentFont(const QFont& _font)
{
    //
    // Не вызываем сигнал о смене типа шрифта, т.к. это сделал не пользователь
    //
    QSignalBlocker blocker(this);

    d->textFontAction->setText(_font.family());
    d->textFontSizeAction->setText(QString::number(qCeil(PageMetrics::pxToPt(_font.pixelSize()))));
}

void ScreenplayTitlePageEditToolbar::focusOutEvent(QFocusEvent* _event)
{
    FloatingToolBar::focusOutEvent(_event);

    d->textFontAction->setIconText(u8"\U000f035d");
    d->textFontSizeAction->setIconText(u8"\U000f035d");
    d->hidePopup();
}

void ScreenplayTitlePageEditToolbar::updateTranslations()
{
    d->undoAction->setToolTip(tr("Undo last action") + QString(" (%1)").arg(QKeySequence(QKeySequence::Undo).toString(QKeySequence::NativeText)));
    d->redoAction->setToolTip(tr("Redo last action") + QString(" (%1)").arg(QKeySequence(QKeySequence::Redo).toString(QKeySequence::NativeText)));
    d->textFontAction->setToolTip(tr("Current text font family"));
    d->textFontSizeAction->setToolTip(tr("Current text font size"));
}

void ScreenplayTitlePageEditToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    auto findMaxWidthFor = [] (const QStringList& _list) {
        int maxFontNameWidth = 0;
        for (const auto& fontName : _list) {
            maxFontNameWidth = std::max(maxFontNameWidth,
                                        TextHelper::fineTextWidth(fontName, Ui::DesignSystem::font().subtitle2()));
        }
        return maxFontNameWidth;
    };

    setActionCustomWidth(d->textFontAction,
                   static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().left())
                   + findMaxWidthFor(d->fontsModel.stringList())
                   + static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().right()));
    setActionCustomWidth(d->textFontSizeAction,
                   static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().left())
                   + findMaxWidthFor(d->fontSizesModel.stringList())
                   + static_cast<int>(Ui::DesignSystem::treeOneLineItem().margins().right()));

    d->popup->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->popupContent->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->popupContent->setTextColor(Ui::DesignSystem::color().onPrimary());

    resize(sizeHint());
}

} // namespace Ui
