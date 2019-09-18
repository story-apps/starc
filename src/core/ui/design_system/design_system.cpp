#include "design_system.h"

#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QIcon>
#include <QMarginsF>
#include <QPixmap>
#include <QPointF>
#include <QSizeF>

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#else
#include <QScreen>
#endif


namespace Ui
{

class DesignSystem::AppBarPrivate
{
public:
    explicit AppBarPrivate(qreal _scaleFactor);

    QMarginsF margins = {16.0, 16.0, 16.0, 16.0};
    QMarginsF bigIconMargins = {0.0, 8.0, 16.0, 8.0};
    qreal heightRegular = 56.0;
    QSizeF iconSize = {24.0, 24.0};
    QSizeF bigIconSize = {40.0, 40.0};
    qreal iconsSpacing = 24.0;
    qreal leftTitleMargin = 72.0;
    qreal shadowRadius = 12.0;
    QPointF shadowOffset = {0.0, 3.0};
    QFont font;
};

DesignSystem::AppBarPrivate::AppBarPrivate(qreal _scaleFactor)
{
    margins *= _scaleFactor;
    bigIconMargins *= _scaleFactor;
    heightRegular *= _scaleFactor;
    iconSize *= _scaleFactor;
    bigIconSize *= _scaleFactor;
    iconsSpacing *= _scaleFactor;
    leftTitleMargin *= _scaleFactor;
    shadowRadius *= _scaleFactor;
    shadowOffset *= _scaleFactor;
    font = QFont("Roboto");
    font.setPixelSize(static_cast<int>(20.0 * _scaleFactor));
    font.setWeight(QFont::Normal);
}

// **

DesignSystem::AppBar::~AppBar() = default;

QMarginsF DesignSystem::AppBar::margins() const
{
    return d->margins;
}

QMarginsF DesignSystem::AppBar::bigIconMargins() const
{
    return d->bigIconMargins;
}

qreal DesignSystem::AppBar::heightRegular() const
{
    return d->heightRegular;
}

QSizeF DesignSystem::AppBar::iconSize() const
{
    return d->iconSize;
}

QSizeF DesignSystem::AppBar::bigIconSize() const
{
    return d->bigIconSize;
}

qreal DesignSystem::AppBar::iconsSpacing() const
{
    return d->iconsSpacing;
}

qreal DesignSystem::AppBar::leftTitleMargin() const
{
    return d->leftTitleMargin;
}

qreal DesignSystem::AppBar::shadowRadius() const
{
    return d->shadowRadius;
}

QPointF DesignSystem::AppBar::shadowOffset() const
{
    return d->shadowOffset;
}

QFont DesignSystem::AppBar::font() const
{
    return d->font;
}

DesignSystem::AppBar::AppBar(qreal _scaleFactor)
    : d(new AppBarPrivate(_scaleFactor))
{
}


// ****


class DesignSystem::DrawerPrivate
{
public:
    explicit DrawerPrivate(qreal _scaleFactor, const Color& _color);

    QMarginsF margins = {16.0, 16.0, 16.0, 16.0};
    QMarginsF actionMargins = {16.0, 12.0, 12.0, 12.0};
    QMarginsF selectionMargins = {8.0, 4.0, 8.0, 4.0};
    qreal subtitleBottomMargin = 18.0;
    qreal iconRightMargin = 32.0;
    qreal width = 304.0;
    qreal titleHeight = 26.0;
    qreal subtitleHeight = 20.0;
    qreal actionHeight = 48.0;
    QSizeF iconSize = {24.0, 24.0};
    qreal separatorHeight = 0.5;
    qreal separatorSpacing = 8.0;
    QColor selectionColor;
    QFont titleFont;
    QFont subtitleFont;
    QFont actionFont;
};

DesignSystem::DrawerPrivate::DrawerPrivate(qreal _scaleFactor, const Color& _color)
{
    margins *= _scaleFactor;
    actionMargins *= _scaleFactor;
    selectionMargins *= _scaleFactor;
    subtitleBottomMargin *= _scaleFactor;
    iconRightMargin *= _scaleFactor;
    width *= _scaleFactor;
    titleHeight *= _scaleFactor;
    subtitleHeight *= _scaleFactor;
    actionHeight *= _scaleFactor;
    iconSize *= _scaleFactor;
    separatorHeight *= _scaleFactor;
    separatorSpacing *= _scaleFactor;
    selectionColor = _color.secondary();
    selectionColor.setAlphaF(0.14);
    titleFont = QFont("Roboto");
    titleFont.setPixelSize(static_cast<int>(20.0 * _scaleFactor));
    titleFont.setWeight(QFont::Medium);
    subtitleFont = QFont("Roboto");
    subtitleFont.setPixelSize(static_cast<int>(13.0 * _scaleFactor));
    subtitleFont.setWeight(QFont::Normal);
    actionFont = QFont("Roboto");
    actionFont.setPixelSize(static_cast<int>(14.0 * _scaleFactor));
    actionFont.setWeight(QFont::Medium);
}

// **

DesignSystem::Drawer::~Drawer() = default;

QMarginsF DesignSystem::Drawer::margins() const
{
    return d->margins;
}

QMarginsF DesignSystem::Drawer::actionMargins() const
{
    return d->actionMargins;
}

QMarginsF DesignSystem::Drawer::selectionMargins() const
{
    return d->selectionMargins;
}

qreal DesignSystem::Drawer::subtitleBottomMargin() const
{
    return d->subtitleBottomMargin;
}

qreal DesignSystem::Drawer::iconRightMargin() const
{
    return d->iconRightMargin;
}

qreal DesignSystem::Drawer::width() const
{
    return d->width;
}

qreal DesignSystem::Drawer::titleHeight() const
{
    return d->titleHeight;
}

qreal DesignSystem::Drawer::subtitleHeight() const
{
    return d->subtitleHeight;
}

qreal DesignSystem::Drawer::actionHeight() const
{
    return d->actionHeight;
}

QSizeF DesignSystem::Drawer::iconSize() const
{
    return d->iconSize;
}

qreal DesignSystem::Drawer::separatorHeight() const
{
    return d->separatorHeight;
}

qreal DesignSystem::Drawer::separatorSpacing() const
{
    return d->separatorSpacing;
}

QColor DesignSystem::Drawer::selectionColor() const
{
    return d->selectionColor;
}

QFont DesignSystem::Drawer::titleFont() const
{
    return d->titleFont;
}

QFont DesignSystem::Drawer::subtitleFont() const
{
    return d->subtitleFont;
}

QFont DesignSystem::Drawer::actionFont() const
{
    return d->actionFont;
}

DesignSystem::Drawer::Drawer(qreal _scaleFactor, const Color& _color)
    : d(new DrawerPrivate(_scaleFactor, _color))
{
}


// ****


class DesignSystem::TabPrivate
{
public:
    explicit TabPrivate(qreal _scaleFactor);

    qreal minimumWidth = 90.0;
    qreal heightWithText = 48.0;
    qreal heightWithIcon = 48.0;
    qreal heightWithTextAndIcon = 72.0;
    QMarginsF margins = {16.0, 12.0, 16.0, 12.0};
    QSizeF iconSize = {24.0, 24.0};
};

DesignSystem::TabPrivate::TabPrivate(qreal _scaleFactor)
{
    minimumWidth *= _scaleFactor;
    heightWithText *= _scaleFactor;
    heightWithIcon *= _scaleFactor;
    heightWithTextAndIcon *= _scaleFactor;
    margins *= _scaleFactor;
    iconSize *= _scaleFactor;
}

// **

DesignSystem::Tab::~Tab() = default;

qreal DesignSystem::Tab::minimumWidth() const
{
    return d->minimumWidth;
}

qreal DesignSystem::Tab::heightWithText() const
{
    return d->heightWithText;
}

qreal DesignSystem::Tab::heightWithIcon() const
{
    return d->heightWithIcon;
}

qreal DesignSystem::Tab::heightWithTextAndIcon() const
{
    return d->heightWithTextAndIcon;
}

QMarginsF DesignSystem::Tab::margins() const
{
    return d->margins;
}

QSizeF DesignSystem::Tab::iconSize() const
{
    return d->iconSize;
}

DesignSystem::Tab::Tab(qreal _scaleFactor)
    : d(new TabPrivate(_scaleFactor))
{
}


// ****


class DesignSystem::TabsPrivate
{
public:
    explicit TabsPrivate(qreal _scaleFactor);

    qreal scrollableLeftMargin = 52.0;
    qreal underlineHeight = 2.0;
    QFont font;
};

DesignSystem::TabsPrivate::TabsPrivate(qreal _scaleFactor)
{
    scrollableLeftMargin *= _scaleFactor;
    underlineHeight *= _scaleFactor;
    font = QFont("Roboto");
    font.setPixelSize(static_cast<int>(14.0 * _scaleFactor));
    font.setWeight(QFont::Medium);
}

// **

DesignSystem::Tabs::~Tabs() = default;

qreal DesignSystem::Tabs::scrollableLeftMargin() const
{
    return d->scrollableLeftMargin;
}

qreal DesignSystem::Tabs::underlineHeight() const
{
    return d->underlineHeight;
}

QFont DesignSystem::Tabs::font() const
{
    return d->font;
}

DesignSystem::Tabs::Tabs(qreal _scaleFactor)
    : d(new TabsPrivate(_scaleFactor))
{
}


// ****


class DesignSystem::TextFieldPrivate
{
public:
    explicit TextFieldPrivate(qreal _scaleFactor, const Color& _color);

    QColor backgroundInactiveColor;
    QColor backgroundActiveColor;
    QColor helperColor;
    QFont labelFont;
    QFont textFont;
    QFont helperFont;
    QMarginsF margins = {12.0, 26.0, 12.0, 12.0};
    QPointF labelTopLeft = {12.0, 6.0};
    qreal iconTop = 16.0;
    QSizeF iconSize = {24.0, 24.0};
    qreal underlineHeight = 1.0;
    qreal underlineHeightInFocus = 2.0;
    qreal helperHeight = 20.0;
};

DesignSystem::TextFieldPrivate::TextFieldPrivate(qreal _scaleFactor, const Color& _color)
{
    backgroundInactiveColor = _color.onSurface();
    backgroundInactiveColor.setAlphaF(0.06);
    backgroundActiveColor = _color.onSurface();
    backgroundActiveColor.setAlphaF(0.10);
    helperColor = _color.onSurface();
    helperColor.setAlphaF(0.4);
    labelFont = QFont("Roboto");
    labelFont.setPixelSize(static_cast<int>(12.0 * _scaleFactor));
    labelFont.setWeight(QFont::Normal);
    textFont = QFont("Roboto");
    textFont.setPixelSize(static_cast<int>(16.0 * _scaleFactor));
    textFont.setWeight(QFont::Normal);
    helperFont = QFont("Roboto");
    helperFont.setPixelSize(static_cast<int>(12.0 * _scaleFactor));
    helperFont.setWeight(QFont::Normal);
    margins *= _scaleFactor;
    labelTopLeft *= _scaleFactor;
    iconTop *= _scaleFactor;
    iconSize *= _scaleFactor;
    underlineHeight *= _scaleFactor;
    underlineHeightInFocus *= _scaleFactor;
    helperHeight *= _scaleFactor;
}

// **

DesignSystem::TextField::~TextField() = default;

QColor DesignSystem::TextField::backgroundInactiveColor() const
{
    return d->backgroundInactiveColor;
}

QColor DesignSystem::TextField::backgroundActiveColor() const
{
    return d->backgroundActiveColor;
}

QColor DesignSystem::TextField::foregroundColor() const
{
    return d->helperColor;
}

QFont DesignSystem::TextField::labelFont() const
{
    return d->labelFont;
}

QFont DesignSystem::TextField::textFont() const
{
    return d->textFont;
}

QFont DesignSystem::TextField::helperFont() const
{
    return d->helperFont;
}

QMarginsF DesignSystem::TextField::margins() const
{
    return d->margins;
}

QPointF DesignSystem::TextField::labelTopLeft() const
{
    return d->labelTopLeft;
}

qreal DesignSystem::TextField::iconTop() const
{
    return d->iconTop;
}

QSizeF DesignSystem::TextField::iconSize() const
{
    return d->iconSize;
}

qreal DesignSystem::TextField::underlineHeight() const
{
    return d->underlineHeight;
}

qreal DesignSystem::TextField::underlineHeightInFocus() const
{
    return d->underlineHeightInFocus;
}

qreal DesignSystem::TextField::helperHeight() const
{
    return d->helperHeight;
}

DesignSystem::TextField::TextField(qreal _scaleFactor, const Color& _color)
    : d(new TextFieldPrivate(_scaleFactor, _color))
{
}


// ****


class DesignSystem::ColorPickerPrivate
{
public:
    explicit ColorPickerPrivate(qreal _scaleFactor);

    QMarginsF margins = {16.0, 16.0, 16.0, 16.0};
    qreal height = 72.0;
    QSizeF iconSize = {40.0, 40.0};
    qreal iconsSpacing = 32.0;
    qreal iconBorderWidth = 1.2;
};

DesignSystem::ColorPickerPrivate::ColorPickerPrivate(qreal _scaleFactor)
{
    margins *= _scaleFactor;
    height *= _scaleFactor;
    iconSize *= _scaleFactor;
    iconsSpacing *= _scaleFactor;
    iconBorderWidth *= _scaleFactor;
}

// **

DesignSystem::ColorPicker::~ColorPicker() = default;

QMarginsF DesignSystem::ColorPicker::margins() const
{
    return d->margins;
}

qreal DesignSystem::ColorPicker::height() const
{
    return d->height;
}

QSizeF DesignSystem::ColorPicker::iconSize() const
{
    return d->iconSize;
}

qreal DesignSystem::ColorPicker::iconsSpacing() const
{
    return d->iconsSpacing;
}

qreal DesignSystem::ColorPicker::iconBorderWidth() const
{
    return d->iconBorderWidth;
}

DesignSystem::ColorPicker::ColorPicker(qreal _scaleFactor)
    : d(new ColorPickerPrivate(_scaleFactor))
{
}


// ****


class DesignSystem::TextTogglePrivate
{
public:
    explicit TextTogglePrivate(qreal _scaleFactor);

    QMarginsF margins = {4.0, 4.0, 4.0, 4.0};
    QSizeF toggleSize = {64.0, 36.0};
    qreal toggleRadius = 17.0;
    qreal spacing = 12.0;
    qreal toggleSpacing = 3.0;
    QFont font;
};

DesignSystem::TextTogglePrivate::TextTogglePrivate(qreal _scaleFactor)
{
    margins *= _scaleFactor;
    toggleSize *= _scaleFactor;
    toggleRadius *= _scaleFactor;
    spacing *= _scaleFactor;
    toggleSpacing *= _scaleFactor;
    font = QFont("Roboto");
    font.setPixelSize(static_cast<int>(14.0 * _scaleFactor));
    font.setWeight(QFont::Medium);
}


DesignSystem::TextToggle::~TextToggle() = default;

QMarginsF DesignSystem::TextToggle::margins() const
{
    return d->margins;
}

QSizeF DesignSystem::TextToggle::toggleSize() const
{
    return d->toggleSize;
}

qreal DesignSystem::TextToggle::toggleRadius() const
{
    return d->toggleRadius;
}

qreal DesignSystem::TextToggle::spacing() const
{
    return d->spacing;
}

qreal DesignSystem::TextToggle::toggleSpacing() const
{
    return d->toggleSpacing;
}

QFont DesignSystem::TextToggle::font() const
{
    return d->font;
}

DesignSystem::TextToggle::TextToggle(qreal _scaleFactor)
    : d(new TextTogglePrivate(_scaleFactor))
{
}


// ****


class DesignSystem::ScrollBarPrivate
{
public:
    explicit ScrollBarPrivate(qreal _scaleFactor, const Color& _color);

    QColor backgroundColor;
    QColor handleColor;
    QMarginsF margins = {2.0, 2.0, 2.0, 2.0};
    qreal verticalWidth = 4.0;
    qreal horizontalHeight = 4.0;
};

DesignSystem::ScrollBarPrivate::ScrollBarPrivate(qreal _scaleFactor, const Color& _color)
{
    backgroundColor = _color.onSurface();
    backgroundColor.setAlphaF(0.08);
    handleColor = _color.onSurface();
    handleColor.setAlphaF(0.24);
    margins *= _scaleFactor;
    verticalWidth *= _scaleFactor;
    horizontalHeight *= _scaleFactor;
}

// **

DesignSystem::ScrollBar::~ScrollBar() = default;

QColor DesignSystem::ScrollBar::backgroundColor() const
{
    return d->backgroundColor;
}

QColor DesignSystem::ScrollBar::handleColor() const
{
    return d->handleColor;
}

QMarginsF DesignSystem::ScrollBar::margins() const
{
    return d->margins;
}

qreal DesignSystem::ScrollBar::verticalWidth() const
{
    return d->verticalWidth;
}

qreal DesignSystem::ScrollBar::horizontalHeight() const
{
    return d->horizontalHeight;
}

DesignSystem::ScrollBar::ScrollBar(qreal _scaleFactor, const Color& _color)
    : d(new ScrollBarPrivate(_scaleFactor, _color))
{
}


// ****


class DesignSystem::ListTwoLineItemPrivate
{
public:
    ListTwoLineItemPrivate(qreal _scaleFactor, const Color& _color);

    QMarginsF margins = {16.0, 16.0, 16.0, 16.0};
    qreal secondaryIconTopMargin = 24.0;
    qreal iconsSpacing = 16.0;
    qreal height = 64.0;
    qreal heightWithMainIcon = 72.0;
    qreal shadowHeight = 6.0;
    QSizeF mainIconSize = {40.0, 40.0};
    QSizeF secondaryIconSize = {24.0, 24.0};
    QColor currentItemColor;
    QFont firstLineFont;
    QFont secondLineFont;
};

DesignSystem::ListTwoLineItemPrivate::ListTwoLineItemPrivate(qreal _scaleFactor, const Color& _color)
{
    margins *= _scaleFactor;
    secondaryIconTopMargin *= _scaleFactor;
    iconsSpacing *= _scaleFactor;
    height *= _scaleFactor;
    heightWithMainIcon *= _scaleFactor;
    shadowHeight *= _scaleFactor;
    mainIconSize *= _scaleFactor;
    secondaryIconSize *= _scaleFactor;
    currentItemColor = _color.onBackground();
    currentItemColor.setAlphaF(0.12);
    firstLineFont = QFont("Roboto");
    firstLineFont.setPixelSize(static_cast<int>(16.0 * _scaleFactor));
    firstLineFont.setWeight(QFont::Normal);
    secondLineFont = QFont("Roboto");
    secondLineFont.setPixelSize(static_cast<int>(14.0 * _scaleFactor));
    secondLineFont.setWeight(QFont::Normal);
}

// **

DesignSystem::ListTwoLineItem::~ListTwoLineItem() = default;

QMarginsF DesignSystem::ListTwoLineItem::margins() const
{
    return d->margins;
}

qreal DesignSystem::ListTwoLineItem::secondaryIconTopMargin() const
{
    return d->secondaryIconTopMargin;
}

qreal DesignSystem::ListTwoLineItem::iconsSpacing() const
{
    return d->iconsSpacing;
}

qreal DesignSystem::ListTwoLineItem::height() const
{
    return d->height;
}

qreal DesignSystem::ListTwoLineItem::heightWithMainIcon() const
{
    return d->heightWithMainIcon;
}

qreal DesignSystem::ListTwoLineItem::shadowHeight() const
{
    return d->shadowHeight;
}

QSizeF DesignSystem::ListTwoLineItem::mainIconSize() const
{
    return d->mainIconSize;
}

QSizeF DesignSystem::ListTwoLineItem::secondaryIconSize() const
{
    return d->secondaryIconSize;
}

QColor DesignSystem::ListTwoLineItem::currentItemColor() const
{
    return d->currentItemColor;
}

QFont DesignSystem::ListTwoLineItem::firstLineFont() const
{
    return d->firstLineFont;
}

QFont DesignSystem::ListTwoLineItem::secondLineFont() const
{
    return d->secondLineFont;
}

DesignSystem::ListTwoLineItem::ListTwoLineItem(qreal _scaleFactor, const Color& _color)
    : d(new ListTwoLineItemPrivate(_scaleFactor, _color))
{
}


// ****


class DesignSystem::ListPrivate
{
public:
    ListPrivate(qreal _scaleFactor);

    QMarginsF margins = {0.0, 24.0, 0.0, 24.0};
};

DesignSystem::ListPrivate::ListPrivate(qreal _scaleFactor)
{
    margins *= _scaleFactor;
}

// **

DesignSystem::List::~List() = default;

QMarginsF DesignSystem::List::margins() const
{
    return d->margins;
}

DesignSystem::List::List(qreal _scaleFactor)
    : d(new ListPrivate(_scaleFactor))
{
}


// ****


class DesignSystem::DialogPrivate
{
public:
    explicit DialogPrivate(qreal _scaleFactor);

    QMarginsF margins = {24.0, 24.0, 24.0, 28.0};
    QMarginsF buttonsMargins = {8.0, 8.0, 8.0, 8.0};
    qreal width = 280.0;
    qreal textSpacing = 24.0;
    qreal buttonsHeight = 52.0;
    qreal buttonsSpacing = 8.0;
    qreal shadowRadius = 64.0;
    QFont titleFont;
    QFont supportingTextFont;
    QFont buttonsFont;
};

DesignSystem::DialogPrivate::DialogPrivate(qreal _scaleFactor)
{
    margins *= _scaleFactor;
    buttonsMargins *= _scaleFactor;
    width *= _scaleFactor;
    textSpacing *= _scaleFactor;
    buttonsHeight *= _scaleFactor;
    buttonsSpacing *= _scaleFactor;
    shadowRadius *= _scaleFactor;
    titleFont = QFont("Roboto");
    titleFont.setPixelSize(static_cast<int>(20.0 * _scaleFactor));
    titleFont.setWeight(QFont::Normal);
    supportingTextFont = QFont("Roboto");
    supportingTextFont.setPixelSize(static_cast<int>(16.0 * _scaleFactor));
    supportingTextFont.setWeight(QFont::Normal);
    buttonsFont = QFont("Roboto");
    buttonsFont.setPixelSize(static_cast<int>(14.0 * _scaleFactor));
    buttonsFont.setWeight(QFont::Bold);
}

// **

DesignSystem::Dialog::~Dialog() = default;

QMarginsF DesignSystem::Dialog::margins() const
{
    return d->margins;
}

QMarginsF DesignSystem::Dialog::buttonsMargins() const
{
    return d->buttonsMargins;
}

qreal DesignSystem::Dialog::width() const
{
    return d->width;
}

qreal DesignSystem::Dialog::textSpacing() const
{
    return d->textSpacing;
}

qreal DesignSystem::Dialog::buttonsHeight() const
{
    return d->buttonsHeight;
}

qreal DesignSystem::Dialog::buttonsSpacing() const
{
    return d->buttonsSpacing;
}

qreal DesignSystem::Dialog::shadowRadius() const
{
    return d->shadowRadius;
}

QFont DesignSystem::Dialog::titleFont() const
{
    return d->titleFont;
}

QFont DesignSystem::Dialog::supportingTextFont() const
{
    return d->supportingTextFont;
}

QFont DesignSystem::Dialog::buttonsFont() const
{
    return d->buttonsFont;
}

DesignSystem::Dialog::Dialog(qreal _scaleFactor)
    : d(new DialogPrivate(_scaleFactor))
{
}


// ****


class DesignSystem::Color::Implementation
{
public:
    Implementation();

    QColor primary;
    QColor primaryDark;
    QColor secondary;
    QColor background;
    QColor surface;
    QColor error;
    QColor shadow;
    QColor onPrimary;
    QColor onSecondary;
    QColor onBackground;
    QColor onSurface;
    QColor onError;
};

DesignSystem::Color::Implementation::Implementation()
{
    primary = QColor("#323740");
    primaryDark = QColor("#22252b");
    secondary = QColor("#448AFF");
    background = QColor("#FFFFFF");
    surface = QColor("#FFFFFF");
    error = QColor("#B00020");
    shadow = QColor("#000000");
    shadow.setAlphaF(0.5);
    onPrimary = QColor("#FFFFFF");
    onSecondary = QColor("#FFFFFF");
    onBackground = QColor("#000000");
    onSurface = QColor("#000000");
    onError = QColor("#FFFFFF");
}


// **


DesignSystem::Color::Color(const DesignSystem::Color& _rhs)
    : d(new Implementation(*_rhs.d))
{
}

DesignSystem::Color& DesignSystem::Color::operator=(const DesignSystem::Color& _rhs)
{
    if (&_rhs == this) {
        return *this;
    }

    d.reset(new Implementation(*_rhs.d));
    return *this;
}

DesignSystem::Color::~Color() = default;

const QColor& DesignSystem::Color::primary() const
{
    return d->primary;
}

const QColor& DesignSystem::Color::primaryDark() const
{
    return d->primaryDark;
}

const QColor& DesignSystem::Color::secondary() const
{
    return d->secondary;
}

const QColor& DesignSystem::Color::background() const
{
    return d->background;
}

const QColor& DesignSystem::Color::surface() const
{
    return d->surface;
}

const QColor& DesignSystem::Color::error() const
{
    return d->error;
}

const QColor& DesignSystem::Color::shadow() const
{
    return d->shadow;
}

const QColor& DesignSystem::Color::onPrimary() const
{
    return d->onPrimary;
}

const QColor& DesignSystem::Color::onSecondary() const
{
    return d->onSecondary;
}

const QColor& DesignSystem::Color::onBackground() const
{
    return d->onBackground;
}

const QColor& DesignSystem::Color::onSurface() const
{
    return d->onSurface;
}

const QColor& DesignSystem::Color::onError() const
{
    return d->onError;
}

void DesignSystem::Color::setPrimary(const QColor& _color)
{
    d->primary = _color;
}

void DesignSystem::Color::setPrimaryDark(const QColor& _color)
{
    d->primaryDark = _color;
}

void DesignSystem::Color::setSecondary(const QColor& _color)
{
    d->secondary = _color;
}

void DesignSystem::Color::setBackground(const QColor& _color)
{
    d->background = _color;
}

void DesignSystem::Color::setSurface(const QColor& _color)
{
    d->surface = _color;
}

void DesignSystem::Color::setError(const QColor& _color)
{
    d->error = _color;
}

void DesignSystem::Color::setShadow(const QColor& _color)
{
    d->shadow = _color;
}

void DesignSystem::Color::setOnPrimary(const QColor& _color)
{
    d->onPrimary = _color;
}

void DesignSystem::Color::setOnSecondary(const QColor& _color)
{
    d->onSecondary = _color;
}

void DesignSystem::Color::setOnBackground(const QColor& _color)
{
    d->onBackground = _color;
}

void DesignSystem::Color::setOnSurface(const QColor& _color)
{
    d->onSurface = _color;
}

void DesignSystem::Color::setOnError(const QColor& _color)
{
    d->onError = _color;
}

DesignSystem::Color::Color()
    : d(new Implementation)
{
}


// ****


class DesignSystem::Font::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    QFont h1 = QFont("Roboto");
    QFont h2 = QFont("Roboto");
    QFont h3 = QFont("Roboto");
    QFont h4 = QFont("Roboto");
    QFont h5 = QFont("Roboto");
    QFont h6 = QFont("Roboto");
    QFont subtitle1 = QFont("Roboto");
    QFont subtitle2 = QFont("Roboto");
    QFont body1 = QFont("Roboto");
    QFont body2 = QFont("Roboto");
    QFont button = QFont("Roboto");
    QFont caption = QFont("Roboto");
    QFont overline = QFont("Roboto");

    QFont iconsSmall = QFont("Material Design Icons");
    QFont iconsMid = QFont("Material Design Icons");
};

DesignSystem::Font::Implementation::Implementation(qreal _scaleFactor)
{
    auto initFont = [_scaleFactor] (QFont::Weight _weight, QFont::Capitalization _capitalization,
            int _pixelSize, qreal _letterSpacing, QFont& _font) {
        _font.setWeight(_weight);
        _font.setCapitalization(_capitalization);
        _font.setPixelSize(static_cast<int>(_pixelSize * _scaleFactor));
        _font.setLetterSpacing(QFont::AbsoluteSpacing, _letterSpacing * _scaleFactor);
    };

    initFont(QFont::Light, QFont::Capitalization::MixedCase, 96, -1.5, h1);
    initFont(QFont::Light, QFont::Capitalization::MixedCase, 60, -0.5, h2);
    initFont(QFont::Normal, QFont::Capitalization::MixedCase, 48, 0.0, h3);
    initFont(QFont::Normal, QFont::Capitalization::MixedCase, 34, 0.25, h4);
    initFont(QFont::Normal, QFont::Capitalization::MixedCase, 24, 0.0, h5);
    initFont(QFont::Medium, QFont::Capitalization::MixedCase, 20, 0.15, h6);
    initFont(QFont::Normal, QFont::Capitalization::MixedCase, 16, 0.15, subtitle1);
    initFont(QFont::Medium, QFont::Capitalization::MixedCase, 14, 0.1, subtitle2);
    initFont(QFont::Normal, QFont::Capitalization::MixedCase, 16, 0.5, body1);
    initFont(QFont::Normal, QFont::Capitalization::MixedCase, 14, 0.25, body2);
    initFont(QFont::Medium, QFont::Capitalization::AllUppercase, 14, 1.25, button);
    initFont(QFont::Normal, QFont::Capitalization::MixedCase, 12, 0.4, caption);
    initFont(QFont::Normal, QFont::Capitalization::AllUppercase, 10, 1.5, overline);

    iconsSmall.setPixelSize(static_cast<int>(16 * _scaleFactor));
    iconsMid.setPixelSize(static_cast<int>(24 * _scaleFactor));
}

// **

DesignSystem::Font::~Font() = default;

const QFont& DesignSystem::Font::h1() const
{
    return d->h1;
}

const QFont& DesignSystem::Font::h2() const
{
    return d->h2;
}

const QFont& DesignSystem::Font::h3() const
{
    return d->h3;
}

const QFont& DesignSystem::Font::h4() const
{
    return d->h4;
}

const QFont& DesignSystem::Font::h5() const
{
    return d->h5;
}

const QFont&DesignSystem::Font::h6() const
{
    return d->h6;
}

const QFont& DesignSystem::Font::subtitle1() const
{
    return d->subtitle1;
}

const QFont& DesignSystem::Font::subtitle2() const
{
    return d->subtitle2;
}

const QFont& DesignSystem::Font::body1() const
{
    return d->body1;
}

const QFont& DesignSystem::Font::body2() const
{
    return d->body2;
}

const QFont& DesignSystem::Font::button() const
{
    return d->button;
}

const QFont& DesignSystem::Font::caption() const
{
    return d->caption;
}

const QFont& DesignSystem::Font::overline() const
{
    return d->overline;
}

const QFont& DesignSystem::Font::iconsSmall() const
{
    return d->iconsSmall;
}

const QFont& DesignSystem::Font::iconsMid() const
{
    return d->iconsMid;
}

DesignSystem::Font::Font(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystem::Layout::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    qreal px4 = 4.0;
    qreal px8 = 8.0;
    qreal px12 = 12.0;
    qreal px16 = 16.0;
    qreal px24 = 24.0;
    qreal px62 = 62.0;
    qreal buttonsSpacing = 8.0;
};

DesignSystem::Layout::Implementation::Implementation(qreal _scaleFactor)
{
    px4 *= _scaleFactor;
    px8 *= _scaleFactor;
    px12 *= _scaleFactor;
    px16 *= _scaleFactor;
    px24 *= _scaleFactor;
    px62 *= _scaleFactor;
    buttonsSpacing *= _scaleFactor;
}


// **


DesignSystem::Layout::~Layout() = default;

qreal DesignSystem::Layout::px4() const
{
    return d->px4;
}

qreal DesignSystem::Layout::px8() const
{
    return d->px8;
}

qreal DesignSystem::Layout::px12() const
{
    return d->px12;
}

qreal DesignSystem::Layout::px16() const
{
    return d->px16;
}

qreal DesignSystem::Layout::px24() const
{
    return d->px24;
}

qreal DesignSystem::Layout::px62() const
{
    return d->px62;
}

qreal DesignSystem::Layout::buttonsSpacing() const
{
    return d->buttonsSpacing;
}

DesignSystem::Layout::Layout(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystem::Label::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    QMarginsF margins = {24.0, 24.0, 24.0, 24.0};
};

DesignSystem::Label::Implementation::Implementation(qreal _scaleFactor)
{
    margins *= _scaleFactor;
}


// **


DesignSystem::Label::~Label() = default;

const QMarginsF& DesignSystem::Label::margins() const
{
    return d->margins;
}

DesignSystem::Label::Label(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystem::Button::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    qreal height = 38.0;
    qreal minimumWidth = 64.0;
    QMarginsF margins = {16.0, 0.0, 16.0, 0.0};
    qreal spacing = 16.0;
    QMarginsF shadowMargins = {14.0, 14.0, 14.0, 16.0};
    qreal minimumShadowBlurRadius = 8.0;
    qreal maximumShadowBlurRadius = 32.0;
    qreal borderRadius = 4.0;
    QSizeF iconSize = {22.0, 22.0};
};

DesignSystem::Button::Implementation::Implementation(qreal _scaleFactor)
{
    height *= _scaleFactor;
    minimumWidth *= _scaleFactor;
    margins *= _scaleFactor;
    spacing *= _scaleFactor;
    shadowMargins *= _scaleFactor;
    minimumShadowBlurRadius *= _scaleFactor;
    maximumShadowBlurRadius *= _scaleFactor;
    borderRadius *= _scaleFactor;
    iconSize *= _scaleFactor;
}


// **


DesignSystem::Button::~Button() = default;

qreal DesignSystem::Button::height() const
{
    return d->height;
}

qreal DesignSystem::Button::minimumWidth() const
{
    return d->minimumWidth;
}

const QMarginsF& DesignSystem::Button::margins() const
{
    return d->margins;
}

qreal DesignSystem::Button::spacing() const
{
    return d->spacing;
}

const QMarginsF& DesignSystem::Button::shadowMargins() const
{
    return d->shadowMargins;
}

qreal DesignSystem::Button::minimumShadowBlurRadius() const
{
    return  d->minimumShadowBlurRadius;
}

qreal DesignSystem::Button::maximumShadowBlurRadius() const
{
    return d->maximumShadowBlurRadius;
}

qreal DesignSystem::Button::borderRadius() const
{
    return d->borderRadius;
}

const QSizeF& DesignSystem::Button::iconSize() const
{
    return d->iconSize;
}

DesignSystem::Button::Button(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystem::RadioButton::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    qreal height = 48.0;
    QMarginsF margins = {24.0, 13.0, 24.0, 13.0};
    QSizeF iconSize = {22.0, 22.0};
    qreal spacing = 16.0;

};

DesignSystem::RadioButton::Implementation::Implementation(qreal _scaleFactor)
{
    height *= _scaleFactor;
    margins *= _scaleFactor;
    iconSize *= _scaleFactor;
    spacing *= _scaleFactor;
}

// **

DesignSystem::RadioButton::~RadioButton() = default;

qreal DesignSystem::RadioButton::height() const
{
    return d->height;
}

const QMarginsF& DesignSystem::RadioButton::margins() const
{
    return d->margins;
}

const QSizeF& DesignSystem::RadioButton::iconSize() const
{
    return d->iconSize;
}

qreal DesignSystem::RadioButton::spacing() const
{
    return d->spacing;
}

DesignSystem::RadioButton::RadioButton(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystem::Slider::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    qreal height = 48.0;
    qreal thumbRadius = 6.0;
    qreal trackHeight = 2.0;
};

DesignSystem::Slider::Implementation::Implementation(qreal _scaleFactor)
{
    height *= _scaleFactor;
    thumbRadius *= _scaleFactor;
    trackHeight *= _scaleFactor;
}


// **


DesignSystem::Slider::~Slider() = default;

qreal DesignSystem::Slider::height() const
{
    return d->height;
}

qreal DesignSystem::Slider::thumbRadius() const
{
    return d->thumbRadius;
}

qreal DesignSystem::Slider::trackHeight() const
{
    return d->trackHeight;
}

DesignSystem::Slider::Slider(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystem::FloatingToolBar::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    QMarginsF margins = {16.0, 16.0, 16.0, 16.0};
    QMarginsF shadowMargins = {14.0, 14.0, 14.0, 16.0};
    qreal minimumShadowBlurRadius = 8.0;
    qreal maximumShadowBlurRadius = 32.0;
    qreal borderRadius = 4.0;
    qreal height = 56.0;
    QSizeF iconSize = {24.0, 24.0};
    qreal spacing = 24.0;
};

DesignSystem::FloatingToolBar::Implementation::Implementation(qreal _scaleFactor)
{
    margins *= _scaleFactor;
    shadowMargins *= _scaleFactor;
    minimumShadowBlurRadius *= _scaleFactor;
    maximumShadowBlurRadius *= _scaleFactor;
    height *= _scaleFactor;
    iconSize *= _scaleFactor;
    spacing *= _scaleFactor;
}


// **


DesignSystem::FloatingToolBar::~FloatingToolBar() = default;

const QMarginsF& DesignSystem::FloatingToolBar::margins() const
{
    return d->margins;
}

const QMarginsF& DesignSystem::FloatingToolBar::shadowMargins() const
{
    return d->shadowMargins;
}

qreal DesignSystem::FloatingToolBar::minimumShadowBlurRadius() const
{
    return d->minimumShadowBlurRadius;
}

qreal DesignSystem::FloatingToolBar::maximumShadowBlurRadius() const
{
    return d->maximumShadowBlurRadius;
}

qreal DesignSystem::FloatingToolBar::height() const
{
    return d->height;
}

const QSizeF& DesignSystem::FloatingToolBar::iconSize() const
{
    return d->iconSize;
}

qreal DesignSystem::FloatingToolBar::spacing() const
{
    return d->spacing;
}

DesignSystem::FloatingToolBar::FloatingToolBar(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystem::Stepper::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    qreal stepHeight = 72.0;
    QMarginsF margins = {24.0, 24.0, 24.0, 24.0};
    QSizeF iconSize = {24.0, 24.0};
    qreal spacing = 12.0;
    qreal pathSpacing = 8.0;
    qreal pathWidth = 2.0;
};

DesignSystem::Stepper::Implementation::Implementation(qreal _scaleFactor)
{
    stepHeight *= _scaleFactor;
    margins *= _scaleFactor;
    iconSize *= _scaleFactor;
    spacing *= _scaleFactor;
    pathSpacing *= _scaleFactor;
    pathWidth *= _scaleFactor;
}

// **

DesignSystem::Stepper::~Stepper() = default;

qreal DesignSystem::Stepper::height() const
{
    return d->stepHeight;
}

QMarginsF DesignSystem::Stepper::margins() const
{
    return d->margins;
}

QSizeF DesignSystem::Stepper::iconSize() const
{
    return d->iconSize;
}

qreal DesignSystem::Stepper::spacing() const
{
    return d->spacing;
}

qreal DesignSystem::Stepper::pathSpacing() const
{
    return d->pathSpacing;
}

qreal DesignSystem::Stepper::pathWidth() const
{
    return d->pathWidth;
}

DesignSystem::Stepper::Stepper(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystem::Card::Implementation
{
public:
    explicit Implementation(qreal _scaleFactor);

    qreal borderRadius = 4.0;
    QMarginsF shadowMargins = {14.0, 14.0, 14.0, 16.0};
    qreal minimumShadowBlurRadius = 8.0;
    qreal maximumShadowBlurRadius = 32.0;
};

DesignSystem::Card::Implementation::Implementation(qreal _scaleFactor)
{
    borderRadius *= _scaleFactor;
    shadowMargins *= _scaleFactor;
    minimumShadowBlurRadius *= _scaleFactor;
    maximumShadowBlurRadius *= _scaleFactor;
}


// **


DesignSystem::Card::~Card() = default;

qreal DesignSystem::Card::borderRadius() const
{
    return d->borderRadius;
}

const QMarginsF& DesignSystem::Card::shadowMargins() const
{
    return d->shadowMargins;
}

qreal DesignSystem::Card::minimumShadowBlurRadius() const
{
    return d->minimumShadowBlurRadius;
}

qreal DesignSystem::Card::maximumShadowBlurRadius() const
{
    return d->maximumShadowBlurRadius;
}

DesignSystem::Card::Card(qreal _scaleFactor)
    : d(new Implementation(_scaleFactor))
{
}


// ****


class DesignSystemPrivate
{
public:
    explicit DesignSystemPrivate(ApplicationTheme _theme = ApplicationTheme::DarkAndLight,
        qreal _scaleFactor = 1.0, const DesignSystem::Color& _color = {});

    ApplicationTheme theme = ApplicationTheme::DarkAndLight;
    qreal scaleFactor = 1.0;

    QMarginsF pageMargins = {16.0, 26.0, 16.0, 16.0};
    qreal pageSpacing = 16.0;
    qreal inactiveTextOpacity = 0.68;
    qreal disabledTextOpacity = 0.46;
    qreal elevationStartOpacity = 0.04;
    qreal elevationEndOpacity = 0.08;

    DesignSystem::Color color;
    DesignSystem::AppBar appBar;
    DesignSystem::Drawer drawer;
    DesignSystem::Tab tab;
    DesignSystem::Tabs tabs;
    DesignSystem::TextField textField;
    DesignSystem::ColorPicker colorPicker;
    DesignSystem::TextToggle textToggle;
    DesignSystem::ScrollBar scrollBar;
    DesignSystem::ListTwoLineItem listTwoLineItem;
    DesignSystem::List list;
    DesignSystem::Dialog dialog;

    DesignSystem::Font font;
    DesignSystem::Layout layout;
    DesignSystem::Label label;
    DesignSystem::Button button;
    DesignSystem::RadioButton radioButton;
    DesignSystem::Slider slider;
    DesignSystem::FloatingToolBar floatingAppBar;
    DesignSystem::Stepper stepper;
    DesignSystem::Card card;
};

DesignSystemPrivate::DesignSystemPrivate(ApplicationTheme _theme, qreal _scaleFactor,
    const DesignSystem::Color& _color)
    : theme(_theme),
      scaleFactor(_scaleFactor),
      color(_color),
      appBar(_scaleFactor),
      drawer(_scaleFactor, _color),
      tab(_scaleFactor),
      tabs(_scaleFactor),
      textField(_scaleFactor, _color),
      colorPicker(_scaleFactor),
      textToggle(_scaleFactor),
      scrollBar(_scaleFactor, _color),
      listTwoLineItem(_scaleFactor, _color),
      list(_scaleFactor),
      dialog(_scaleFactor),
      font(_scaleFactor),
      layout(_scaleFactor),
      label(_scaleFactor),
      button(_scaleFactor),
      radioButton(_scaleFactor),
      slider(_scaleFactor),
      floatingAppBar(_scaleFactor),
      stepper(_scaleFactor),
      card(_scaleFactor)
{
    pageMargins *= _scaleFactor;
    pageSpacing *= _scaleFactor;
}

// **

ApplicationTheme DesignSystem::theme()
{
    return instance()->d->theme;
}

void DesignSystem::setTheme(ApplicationTheme _theme)
{
    if (instance()->d->theme == _theme) {
        return;
    }

    instance()->d->theme = _theme;

    QColor primary;
    QColor primaryDark;
    QColor secondary;
    QColor background;
    QColor surface;
    QColor error;
    QColor shadow;
    QColor onPrimary;
    QColor onSecondary;
    QColor onBackground;
    QColor onSurface;
    QColor onError;

    switch (_theme) {
        case Ui::ApplicationTheme::DarkAndLight: {
            primary = "#323740";
            primaryDark = "#22252b";
            secondary = "#448AFF";
            background = "#FFFFFF";
            surface = "#FFFFFF";
            error = "#B00020";
            shadow = [] { QColor color = "#000000";
                          color.setAlphaF(0.3);
                          return color; } ();
            onPrimary = "#FFFFFF";
            onSecondary = "#FFFFFF";
            onBackground = "#000000";
            onSurface = "#000000";
            onError = "#FFFFFF";
            break;
        }

        case Ui::ApplicationTheme::Dark: {
            primary = "#1F1F1F";
            primaryDark = "#0A0A0A";
            secondary = "#448AFF";
            background = "#121212";
            surface = "#121212";
            error = "#CF6679";
            shadow = [] { QColor color = "#000000";
                          color.setAlphaF(0.68);
                          return color; } ();
            onPrimary = "#FFFFFF";
            onSecondary = "#FFFFFF";
            onBackground = "#FFFFFF";
            onSurface = "#FFFFFF";
            onError = "#000000";
            break;
        }

        case Ui::ApplicationTheme::Light: {
            primary = "#E4E4E4";
            primaryDark = "#C8C8C8";
            secondary = "#448AFF";
            background = "#FFFFFF";
            surface = "#FFFFFF";
            error = "#B00020";
            shadow = [] { QColor color = "#000000";
                          color.setAlphaF(0.36);
                          return color; } ();
            onPrimary = "#38393A";
            onSecondary = "#FFFFFF";
            onBackground = "#000000";
            onSurface = "#000000";
            onError = "#FFFFFF";
            break;
        }

        case Ui::ApplicationTheme::Custom: {
            //
            // TODO:
            //
            break;
        }
    }

    auto newColor(instance()->d->color);
    newColor.setPrimary(primary);
    newColor.setPrimaryDark(primaryDark);
    newColor.setSecondary(secondary);
    newColor.setBackground(background);
    newColor.setSurface(surface);
    newColor.setError(error);
    newColor.setShadow(shadow);
    newColor.setOnPrimary(onPrimary);
    newColor.setOnSecondary(onSecondary);
    newColor.setOnBackground(onBackground);
    newColor.setOnSurface(onSurface);
    newColor.setOnError(onError);
    setColor(newColor);
}

qreal DesignSystem::scaleFactor()
{
    return instance()->d->scaleFactor;
}

void DesignSystem::setScaleFactor(qreal _scaleFactor)
{
    if (qFuzzyCompare(instance()->d->scaleFactor, _scaleFactor)) {
        return;
    }

    instance()->d.reset(new DesignSystemPrivate(theme(), _scaleFactor, color()));
}

QMarginsF DesignSystem::pageMargins()
{
    return instance()->d->pageMargins;
}

qreal DesignSystem::pageSpacing()
{
    return instance()->d->pageSpacing;
}

qreal DesignSystem::inactiveTextOpacity()
{
    return instance()->d->inactiveTextOpacity;
}

qreal DesignSystem::disabledTextOpacity()
{
    return instance()->d->disabledTextOpacity;
}

qreal DesignSystem::elevationStartOpacity()
{
    return instance()->d->elevationStartOpacity;
}

qreal DesignSystem::elevationEndOpacity()
{
    return instance()->d->elevationEndOpacity;
}

const DesignSystem::AppBar& DesignSystem::appBar()
{
    return instance()->d->appBar;
}

const DesignSystem::Drawer& DesignSystem::drawer()
{
    return instance()->d->drawer;
}

const DesignSystem::Tab& DesignSystem::tab()
{
    return instance()->d->tab;
}

const DesignSystem::Tabs& DesignSystem::tabs()
{
    return instance()->d->tabs;
}

const DesignSystem::TextField& DesignSystem::textField()
{
    return instance()->d->textField;
}

const DesignSystem::ColorPicker& DesignSystem::colorPicker()
{
    return instance()->d->colorPicker;
}

const DesignSystem::TextToggle&DesignSystem::textToggle()
{
    return instance()->d->textToggle;
}

const DesignSystem::ScrollBar& DesignSystem::scrollBar()
{
    return instance()->d->scrollBar;
}

const DesignSystem::ListTwoLineItem&DesignSystem::listTwoLineItem()
{
    return instance()->d->listTwoLineItem;
}

const DesignSystem::List&DesignSystem::list()
{
    return instance()->d->list;
}

const DesignSystem::Dialog& DesignSystem::dialog()
{
    return instance()->d->dialog;
}

const DesignSystem::Color& DesignSystem::color()
{
    return instance()->d->color;
}

void DesignSystem::setColor(const DesignSystem::Color& _color)
{
    instance()->d.reset(new DesignSystemPrivate(theme(), scaleFactor(), _color));
}

const DesignSystem::Font& DesignSystem::font()
{
    return instance()->d->font;
}

const DesignSystem::Layout& DesignSystem::layout()
{
    return instance()->d->layout;
}

const DesignSystem::Label& DesignSystem::label()
{
    return instance()->d->label;
}

const DesignSystem::Button&DesignSystem::button()
{
    return instance()->d->button;
}

const DesignSystem::RadioButton& DesignSystem::radioButton()
{
    return instance()->d->radioButton;
}

const DesignSystem::Slider& DesignSystem::slider()
{
    return instance()->d->slider;
}

const DesignSystem::FloatingToolBar& DesignSystem::floatingToolBar()
{
    return instance()->d->floatingAppBar;
}

const DesignSystem::Stepper& DesignSystem::stepper()
{
    return instance()->d->stepper;
}

const DesignSystem::Card& DesignSystem::card()
{
    return instance()->d->card;
}

DesignSystem::~DesignSystem() = default;

DesignSystem::DesignSystem()
    : d(new DesignSystemPrivate)
{
}

DesignSystem* DesignSystem::instance()
{
    static DesignSystem* s_designSystem = new DesignSystem;
    return s_designSystem;
}

} // namespace Ui
