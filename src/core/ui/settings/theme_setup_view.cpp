#include "theme_setup_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>
#include <QClipboard>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <QVariantAnimation>


namespace Ui {

class ThemeColor::Implementation
{
public:
    /**
     * @brief Получить области, которые занимают цвета
     */
    QPair<QRectF, QRectF> colorRects(const QRect& _rect) const;


    QColor color;
    QColor onColor;
    QString title;
};

QPair<QRectF, QRectF> ThemeColor::Implementation::colorRects(const QRect& _rect) const
{
    const QRectF colorsRect(_rect.left() + Ui::DesignSystem::layout().px4(),
                            _rect.top() + Ui::DesignSystem::layout().px4(),
                            _rect.width() - Ui::DesignSystem::layout().px8(),
                            Ui::DesignSystem::layout().px(28));
    const QRectF leftColorRect(colorsRect.left(), colorsRect.top(), colorsRect.width() / 2,
                               colorsRect.height());
    const QRectF rightColorRect(leftColorRect.topRight(), leftColorRect.size());
    return { leftColorRect, rightColorRect };
}

// **

ThemeColor::ThemeColor(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setFocusPolicy(Qt::StrongFocus);
}

ThemeColor::~ThemeColor() = default;

QColor ThemeColor::color() const
{
    return d->color;
}

void ThemeColor::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    update();
}

QColor ThemeColor::onColor() const
{
    return d->onColor;
}

void ThemeColor::setOnColor(const QColor& _color)
{
    if (d->onColor == _color) {
        return;
    }

    d->onColor = _color;
    update();
}

void ThemeColor::setTitle(const QString& _title)
{
    if (d->title == _title) {
        return;
    }

    d->title = _title;
    update();
}

QSize ThemeColor::sizeHint() const
{
    return QSize(Ui::DesignSystem::layout().px(112), Ui::DesignSystem::layout().px62());
}

QSize ThemeColor::minimumSizeHint() const
{
    return sizeHint();
}

bool ThemeColor::event(QEvent* _event)
{
    if (_event->type() == QEvent::ToolTip) {
        const auto colorRects = d->colorRects(contentsRect());
        QHelpEvent* event = static_cast<QHelpEvent*>(_event);
        if (colorRects.first.contains(event->pos())) {
            QToolTip::showText(event->globalPos(), tr("Background color"));
        } else if (colorRects.second.contains(event->pos())) {
            QToolTip::showText(event->globalPos(), tr("Text color"));
        } else {
            QToolTip::hideText();
        }
        return true;
    }

    return Widget::event(_event);
}

void ThemeColor::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(_event->rect(), backgroundColor());

    const auto rect = contentsRect();
    QRectF colorsRect(rect.left() + Ui::DesignSystem::layout().px4(),
                      rect.top() + Ui::DesignSystem::layout().px4(),
                      rect.width() - Ui::DesignSystem::layout().px8(),
                      Ui::DesignSystem::layout().px(28));
    QPainterPath clipPath;
    clipPath.addRoundedRect(colorsRect, colorsRect.height() / 2, colorsRect.height() / 2);
    painter.setClipPath(clipPath);
    QRectF colorRect(colorsRect.left(), colorsRect.top(), colorsRect.width() / 2,
                     colorsRect.height());
    painter.fillRect(colorRect, d->color);
    colorRect.moveLeft(colorRect.right());
    painter.fillRect(colorRect, d->onColor);
    painter.setPen(ColorHelper::contrasted(d->onColor));
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    const QRectF colorTextRect = colorRect.adjusted(0, 0, -Ui::DesignSystem::layout().px4(), 0);
    painter.drawText(colorTextRect, Qt::AlignCenter, u8"\U000F0284");
    painter.setClipping(false);

    painter.setPen(textColor());
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(colorsRect, colorsRect.height() / 2, colorsRect.height() / 2);
    if (hasFocus()) {
        painter.setPen(
            QPen(Ui::DesignSystem::color().accent(), Ui::DesignSystem::layout().px2()));
    }
    colorsRect.adjust(-Ui::DesignSystem::layout().px4(), -Ui::DesignSystem::layout().px4(),
                      Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px4());
    painter.drawRoundedRect(colorsRect, colorsRect.height() / 2, colorsRect.height() / 2);

    const QRectF textRect(0, sizeHint().height() - Ui::DesignSystem::layout().px(28), width(),
                          Ui::DesignSystem::layout().px(28));
    painter.setPen(textColor());
    painter.setFont(Ui::DesignSystem::font().subtitle2());
    painter.drawText(textRect, Qt::AlignCenter, d->title);
}

void ThemeColor::mouseReleaseEvent(QMouseEvent* _event)
{
    const auto colorRects = d->colorRects(contentsRect());
    if (colorRects.first.contains(_event->pos())) {
        emit colorPressed(d->color);
    } else if (colorRects.second.contains(_event->pos())) {
        emit onColorPressed(d->onColor);
    }
}

void ThemeColor::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setContentsMargins(QMargins(Ui::DesignSystem::layout().px(22), Ui::DesignSystem::layout().px(),
                                Ui::DesignSystem::layout().px(22), 0));
}


// ****


class ThemeSetupView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QString sourceThemeHash;

    IconButton* copyHash = nullptr;
    IconButton* pasteHash = nullptr;

    QHBoxLayout* layout = nullptr;
    ThemeColor* primary = nullptr;
    ThemeColor* secondary = nullptr;
    ThemeColor* background = nullptr;
    ThemeColor* surface = nullptr;
    ThemeColor* error = nullptr;
    ThemeColor* textEditor = nullptr;

    ColorPickerPopup* colorPickerPopup = nullptr;

    Button* cancelButton = nullptr;
    Button* saveButton = nullptr;

    QVariantAnimation heightAnimation;
};

ThemeSetupView::Implementation::Implementation(QWidget* _parent)
    : copyHash(new IconButton(_parent))
    , pasteHash(new IconButton(_parent))
    , layout(new QHBoxLayout)
    , primary(new ThemeColor(_parent))
    , secondary(new ThemeColor(_parent))
    , background(new ThemeColor(_parent))
    , surface(new ThemeColor(_parent))
    , error(new ThemeColor(_parent))
    , textEditor(new ThemeColor(_parent))
    , colorPickerPopup(new ColorPickerPopup(_parent))
    , cancelButton(new Button(_parent))
    , saveButton(new Button(_parent))
{
    copyHash->setIcon(u8"\U000F018F");
    pasteHash->setIcon(u8"\U000F0192");

    colorPickerPopup->setAutoHideOnSelection(false);

    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(copyHash);
    layout->addWidget(pasteHash);
    layout->addStretch();
    layout->addWidget(primary);
    layout->addWidget(secondary);
    layout->addWidget(background);
    layout->addWidget(surface);
    layout->addWidget(error);
    layout->addWidget(textEditor);
    layout->addStretch();
    layout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    layout->addWidget(saveButton, 0, Qt::AlignVCenter);

    heightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    heightAnimation.setDuration(160);
}

// **

ThemeSetupView::ThemeSetupView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setLayout(d->layout);

    auto notifyThemeChanged = [this] {
        auto color(Ui::DesignSystem::color());
        color.setPrimary(d->primary->color());
        color.setOnPrimary(d->primary->onColor());
        color.setAccent(d->secondary->color());
        color.setOnAccent(d->secondary->onColor());
        color.setBackground(d->background->color());
        color.setOnBackground(d->background->onColor());
        color.setSurface(d->surface->color());
        color.setOnSurface(d->surface->onColor());
        color.setError(d->error->color());
        color.setOnError(d->error->onColor());
        color.setTextEditor(d->textEditor->color());
        color.setOnTextEditor(d->textEditor->onColor());
        emit customThemeColorsChanged(color);
    };

    for (auto themeColor : {
             d->primary,
             d->secondary,
             d->background,
             d->surface,
             d->error,
             d->textEditor,
         }) {
        themeColor->installEventFilter(this);

        connect(themeColor, &ThemeColor::colorPressed, this,
                [this, notifyThemeChanged, themeColor](const QColor& _color) {
                    d->colorPickerPopup->setSelectedColor(_color);
                    if (!d->colorPickerPopup->isPopupShown()) {
                        d->colorPickerPopup->showPopup(this);
                    }

                    d->colorPickerPopup->disconnect();
                    connect(d->colorPickerPopup, &ColorPickerPopup::selectedColorChanged,
                            themeColor, &ThemeColor::setColor);
                    connect(d->colorPickerPopup, &ColorPickerPopup::selectedColorChanged, this,
                            notifyThemeChanged);
                });
        connect(themeColor, &ThemeColor::onColorPressed, this,
                [this, notifyThemeChanged, themeColor](const QColor& _color) {
                    d->colorPickerPopup->setSelectedColor(_color);
                    if (!d->colorPickerPopup->isPopupShown()) {
                        d->colorPickerPopup->showPopup(this);
                    }

                    d->colorPickerPopup->disconnect();
                    connect(d->colorPickerPopup, &ColorPickerPopup::selectedColorChanged,
                            themeColor, &ThemeColor::setOnColor);
                    connect(d->colorPickerPopup, &ColorPickerPopup::selectedColorChanged, this,
                            notifyThemeChanged);
                });
    }
    auto showToolTip = [](const QString& _text) {
        QToolTip::showText(QCursor::pos(), _text, nullptr, {}, 1600);
    };
    connect(d->copyHash, &IconButton::clicked, this, [showToolTip] {
        QGuiApplication::clipboard()->setText(Ui::DesignSystem::color().toString());
        showToolTip(tr("HASH copied."));
    });
    connect(d->pasteHash, &IconButton::clicked, this, [this, showToolTip] {
        const auto hash = QGuiApplication::clipboard()->text();
        if (hash.length() != 84) {
            showToolTip(tr("Pasted HASH has incorrect length"));
            return;
        }

        QVector<QColor> colors;
        int startIndex = 0;
        for (int colorIndex = 0; colorIndex < 14; ++colorIndex) {
            const int length = 6;
            const QColor color = "#" + hash.mid(startIndex, length);
            if (!color.isValid()) {
                showToolTip(tr("Pasted HASH has invalid colors"));
                return;
            }

            startIndex += length;
            colors.append(color);
        }
        for (const auto& color : std::as_const(colors)) {
            if (colors.count(color) > 6) {
                showToolTip(tr("Pasted HASH has too many equal colors"));
                return;
            }
        }

        showToolTip(tr("HASH pasted."));
        emit customThemeColorsChanged(Ui::DesignSystem::Color(hash));
    });
    connect(d->cancelButton, &Button::clicked, this, [this] {
        emit customThemeColorsChanged(Ui::DesignSystem::Color(d->sourceThemeHash));
        hideView();
    });
    connect(d->saveButton, &Button::clicked, this, &ThemeSetupView::hideView);
    connect(&d->heightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) { setFixedHeight(_value.toInt()); });
    connect(&d->heightAnimation, &QVariantAnimation::finished, this, [this] {
        if (d->heightAnimation.direction() == QVariantAnimation::Backward) {
            hide();
        }
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ThemeSetupView::~ThemeSetupView() = default;

void ThemeSetupView::setSourceThemeHash(const QString& _themeHash)
{
    d->sourceThemeHash = _themeHash;
}

void ThemeSetupView::showView()
{
    if (isVisible()) {
        return;
    }

    show();
    d->heightAnimation.setDirection(QVariantAnimation::Forward);
    d->heightAnimation.start();
}

void ThemeSetupView::hideView()
{
    if (!isVisible()) {
        return;
    }

    d->heightAnimation.setDirection(QVariantAnimation::Backward);
    d->heightAnimation.start();
}

bool ThemeSetupView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_event->type() == QEvent::FocusOut) {
        const auto focusedObject = QGuiApplication::focusObject();
        const auto focusedThemeColor = qobject_cast<ThemeColor*>(focusedObject);
        if (d->colorPickerPopup->isPopupShown()
            && (focusedObject == nullptr
                || (focusedObject != nullptr && focusedObject != d->colorPickerPopup
                    && (focusedObject->parent() != this || focusedThemeColor == nullptr)))) {
            d->colorPickerPopup->hidePopup();
        }
    }

    return Widget::eventFilter(_watched, _event);
}

void ThemeSetupView::updateTranslations()
{
    d->copyHash->setToolTip(tr("Copy theme HASH to share your custom theme with others"));
    d->pasteHash->setToolTip(tr("Paste theme HASH from clipboard and apply it"));

    d->primary->setTitle(tr("Primary"));
    d->secondary->setTitle(tr("Accent"));
    d->background->setTitle(tr("Background"));
    d->surface->setTitle(tr("Surface"));
    d->error->setTitle(tr("Error"));
    d->textEditor->setTitle(tr("Text edit"));

    d->cancelButton->setText(tr("Cancel"));
    d->saveButton->setText(tr("Save"));
}

void ThemeSetupView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    const auto theme = Ui::DesignSystem::color();

    setBackgroundColor(theme.primary());

    d->layout->setContentsMargins(
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16(),
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16());

    for (auto widget : std::vector<Widget*>{
             d->pasteHash,
             d->copyHash,
             d->primary,
             d->secondary,
             d->background,
             d->surface,
             d->error,
             d->textEditor,
         }) {
        widget->setBackgroundColor(theme.primary());
        widget->setTextColor(
            ColorHelper::transparent(theme.onPrimary(), Ui::DesignSystem::inactiveTextOpacity()));
    }

    d->primary->setColor(theme.primary());
    d->primary->setOnColor(theme.onPrimary());
    d->secondary->setColor(theme.accent());
    d->secondary->setOnColor(theme.onAccent());
    d->background->setColor(theme.background());
    d->background->setOnColor(theme.onBackground());
    d->surface->setColor(theme.surface());
    d->surface->setOnColor(theme.onSurface());
    d->error->setColor(theme.error());
    d->error->setOnColor(theme.onError());
    d->textEditor->setColor(theme.textEditor());
    d->textEditor->setOnColor(theme.onTextEditor());

    d->colorPickerPopup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->colorPickerPopup->setTextColor(Ui::DesignSystem::color().onBackground());

    for (auto button : { d->cancelButton, d->saveButton }) {
        button->setBackgroundColor(theme.accent());
        button->setTextColor(theme.accent());
    }

    d->heightAnimation.setStartValue(0);
    d->heightAnimation.setEndValue(sizeHint().height());
}

} // namespace Ui
