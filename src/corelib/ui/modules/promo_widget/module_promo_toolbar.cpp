#include "module_promo_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/ui_helper.h>
#include <utils/tools/debouncer.h>

#include <QBoxLayout>
#include <QVariantAnimation>


namespace Ui {

class ModulePromoToolbar::Implementation
{
public:
    explicit Implementation(ModulePromoToolbar* _q);

    void correctToolbarPosition();
    void hideHeader();
    void showHeader();


    ModulePromoToolbar* q = nullptr;

    Widget* headerContainer = nullptr;
    H6Label* title = nullptr;
    Subtitle1Label* subtitle = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
    Button* goBack = nullptr;
    Button* purchase = nullptr;

    QPoint originalPos;
    QVariantAnimation posAnimation;
    Debouncer posAnimationDebouncer;
    QAbstractAnimation::Direction posAnimationDirection = QVariantAnimation::Backward;
};

ModulePromoToolbar::Implementation::Implementation(ModulePromoToolbar* _q)
    : q(_q)
    , headerContainer(new Widget(_q))
    , title(new H6Label(headerContainer))
    , subtitle(new Subtitle1Label(headerContainer))
    , goBack(new Button(q))
    , purchase(new Button(q))
    , posAnimationDebouncer(60)
{
    title->setAlignment(Qt::AlignHCenter);
    subtitle->setAlignment(Qt::AlignHCenter);

    posAnimation.setDuration(120);
    posAnimation.setStartValue(0);
    posAnimation.setEasingCurve(QEasingCurve::OutQuad);
    posAnimation.setDirection(posAnimationDirection);


    auto headerLayout = UiHelper::makeVBoxLayout();
    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);
    headerContainer->setLayout(headerLayout);

    auto buttonsLayout = UiHelper::makeHBoxLayout();
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(goBack);
    buttonsLayout->addWidget(purchase);
    buttonsLayout->addStretch();

    auto layout = UiHelper::makeVBoxLayout();
    layout->addWidget(headerContainer);
    layout->addLayout(buttonsLayout);
    layout->addStretch();
    q->setLayout(layout);
}

void ModulePromoToolbar::Implementation::correctToolbarPosition()
{
    q->move(originalPos.x(), originalPos.y() - posAnimation.currentValue().toInt());
}

void ModulePromoToolbar::Implementation::hideHeader()
{
    //
    // Если пользователь просто пронёс курсор мимо, то игнорируем это событие
    //
    if (posAnimationDebouncer.hasPendingWork()
        && posAnimationDirection == QVariantAnimation::Backward) {
        posAnimationDebouncer.abortWork();
        return;
    }

    posAnimationDirection = QVariantAnimation::Forward;
    posAnimationDebouncer.orderWork();
}

void ModulePromoToolbar::Implementation::showHeader()
{
    //
    // Если пользователь просто пронёс курсор мимо, то игнорируем это событие
    //
    if (posAnimationDebouncer.hasPendingWork()
        && posAnimationDirection == QVariantAnimation::Forward) {
        posAnimationDebouncer.abortWork();
        return;
    }

    posAnimationDirection = QVariantAnimation::Backward;
    posAnimationDebouncer.orderWork();
}


// ****


ModulePromoToolbar::ModulePromoToolbar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    setCurtain(true);


    connect(d->goBack, &Button::clicked, this, &ModulePromoToolbar::goBackPressed);
    connect(d->purchase, &Button::clicked, this, &ModulePromoToolbar::purchasePressed);
    connect(&d->posAnimation, &QVariantAnimation::valueChanged, this,
            [this] { d->correctToolbarPosition(); });
    connect(&d->posAnimationDebouncer, &Debouncer::gotWork, this, [this] {
        d->posAnimation.setDirection(d->posAnimationDirection);
        d->posAnimation.start();

        //
        // Настраиваем длительность ожидания таким образом, чтобы панель уезжала
        // не так быстро и пользователь мог вернуться для взаимодействия с ней
        //
        d->posAnimationDebouncer.setDelay(
            d->posAnimationDirection == QVariantAnimation::Forward ? 60 : 800);
    });
}

ModulePromoToolbar::~ModulePromoToolbar() = default;

void ModulePromoToolbar::setTitle(const QString& _title)
{
    d->title->setText(_title);
}

void ModulePromoToolbar::setSubtitle(const QString& _subtitle)
{
    d->subtitle->setText(_subtitle);
}

void ModulePromoToolbar::setGoBackButtonVisible(bool _visible)
{
    d->goBack->setVisible(_visible);
}

void ModulePromoToolbar::setGoBackButtonIcon(const QString& _icon)
{
    d->goBack->setIcon(_icon);
}

void ModulePromoToolbar::setGoBackButtonText(const QString& _text)
{
    d->goBack->setText(_text);
}

void ModulePromoToolbar::setPuchaseButtonText(const QString& _text)
{
    d->purchase->setText(_text);
}

void ModulePromoToolbar::setOriginalPos(const QPoint& _pos)
{
    if (d->originalPos == _pos) {
        return;
    }

    d->originalPos = _pos;
    d->correctToolbarPosition();
}

QSize ModulePromoToolbar::sizeHint() const
{
    return QSize(std::max(std::max(d->title->sizeHint().width(), d->subtitle->sizeHint().width()),
                          d->goBack->sizeHint().width() + d->purchase->sizeHint().width())
                     + DesignSystem::layout().px(120),
                 DesignSystem::layout().px(164));
}

void ModulePromoToolbar::processBackgroundColorChange()
{
    const auto headerColor = ColorHelper::transparent(ColorHelper::nearby(backgroundColor()),
                                                      DesignSystem::inactiveItemOpacity());
    for (auto widget : std::vector<Widget*>{
             d->title,
             d->subtitle,
         }) {
        widget->setBackgroundColor(headerColor);
    }
    d->goBack->setBackgroundColor(backgroundColor());
    d->purchase->setBackgroundColor(backgroundColor());
}

void ModulePromoToolbar::processTextColorChange()
{
    d->title->setTextColor(textColor());
    const auto inactiveTextColor
        = ColorHelper::transparent(textColor(), DesignSystem::inactiveTextOpacity());
    d->subtitle->setTextColor(inactiveTextColor);
    d->goBack->setTextColor(inactiveTextColor);
}

void ModulePromoToolbar::resizeEvent(QResizeEvent* _event)
{
    FloatingToolBar::resizeEvent(_event);

    d->posAnimation.setEndValue(static_cast<int>(
        d->headerContainer->height() - d->headerContainer->layout()->contentsMargins().top()
        - d->headerContainer->layout()->contentsMargins().bottom()));
}

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
void ModulePromoToolbar::enterEvent(QEnterEvent* _event)
#else
void ModulePromoToolbar::enterEvent(QEvent* _event)
#endif
{
    FloatingToolBar::enterEvent(_event);

    d->showHeader();
}

void ModulePromoToolbar::leaveEvent(QEvent* _event)
{
    FloatingToolBar::leaveEvent(_event);

    d->hideHeader();
}

void ModulePromoToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    d->headerContainer->setBackgroundColor(Qt::transparent);
    const auto shadowMargins = DesignSystem::floatingToolBar().shadowMargins().toMargins();
    d->headerContainer->layout()->setContentsMargins(shadowMargins.left(), shadowMargins.top(),
                                                     shadowMargins.right(),
                                                     DesignSystem::layout().px(10));
    d->title->setContentsMargins(0, DesignSystem::layout().px16(), 0, DesignSystem::layout().px4());
    d->subtitle->setContentsMargins(0, 0, 0, DesignSystem::layout().px12());
    d->purchase->setTextColor(DesignSystem::color().accent());
}


} // namespace Ui
