#include "theme_preview.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>

#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>


class ThemePreview::Implementation
{
public:
    Ui::ApplicationTheme theme = Ui::ApplicationTheme::Light;
};


// ****


ThemePreview::ThemePreview(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    designSystemChangeEvent(nullptr);
}

ThemePreview::~ThemePreview() = default;

void ThemePreview::setTheme(Ui::ApplicationTheme _theme)
{
    if (d->theme == _theme) {
        return;
    }

    d->theme = _theme;
    update();
}

QSize ThemePreview::sizeHint() const
{
    const auto width = Ui::DesignSystem::layout().px(120);
    const auto height = width + Ui::DesignSystem::layout().px(30);
    return QSize(width, height);
}

void ThemePreview::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Закрашиваем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Будем рисовать в области со скруглёнными краями
    //
    QPainterPath clipPath;
    clipPath.addRoundedRect(contentsRect(), Ui::DesignSystem::layout().px12(),
                            Ui::DesignSystem::layout().px12());
    painter.setClipPath(clipPath);
    painter.translate(contentsRect().topLeft());

    //
    // Сормируем цвета темы, которыми будем рисовать
    //
    const auto themeColor = d->theme == Ui::ApplicationTheme::Custom
        ? Ui::DesignSystem::Color(
            settingsValue(DataStorageLayer::kApplicationCustomThemeColorsKey).toString())
        : Ui::DesignSystem::color(d->theme);

    //
    // Собственно отрисовка
    //
    painter.setPen(Qt::NoPen);
    //
    // ... фон
    //
    painter.fillRect(_event->rect(), themeColor.surface());
    //
    // ... сайдбар
    //
    const QRectF sideBarRect(0, 0, Ui::DesignSystem::layout().px(40), height());
    painter.fillRect(sideBarRect, themeColor.primary());
    const QRectF selectedItemBackgroundRect(0, Ui::DesignSystem::layout().px(22),
                                            sideBarRect.width(), Ui::DesignSystem::layout().px12());
    //
    painter.fillRect(selectedItemBackgroundRect,
                     ColorHelper::transparent(themeColor.secondary(),
                                              Ui::DesignSystem::hoverBackgroundOpacity()));
    const QRectF selectedItemTextRect(
        Ui::DesignSystem::layout().px4(),
        selectedItemBackgroundRect.center().y() - Ui::DesignSystem::layout().px(),
        selectedItemBackgroundRect.width() - Ui::DesignSystem::layout().px8(),
        Ui::DesignSystem::layout().px2());
    painter.setBrush(themeColor.secondary());
    painter.drawRoundedRect(selectedItemTextRect, selectedItemTextRect.height() / 2,
                            selectedItemTextRect.height() / 2);
    //
    painter.setBrush(
        ColorHelper::transparent(themeColor.onPrimary(), Ui::DesignSystem::inactiveTextOpacity()));
    auto notSelectedItemTextRect = selectedItemTextRect;
    notSelectedItemTextRect.moveCenter(notSelectedItemTextRect.center()
                                       + QPointF(0, selectedItemBackgroundRect.height()));
    for (int item = 0; item < 5; ++item) {
        painter.drawRoundedRect(notSelectedItemTextRect, notSelectedItemTextRect.height() / 2,
                                notSelectedItemTextRect.height() / 2);
        notSelectedItemTextRect.moveCenter(notSelectedItemTextRect.center()
                                           + QPointF(0, selectedItemBackgroundRect.height() * 0.7));
    }
    //
    // ... тулбар
    //
    painter.setBrush(ColorHelper::nearby(themeColor.background()));
    const QRectF toolBarRect(sideBarRect.right() + Ui::DesignSystem::layout().px(6),
                             Ui::DesignSystem::layout().px8(),
                             width() - sideBarRect.width() - Ui::DesignSystem::layout().px(40),
                             Ui::DesignSystem::layout().px12());
    painter.drawRoundedRect(toolBarRect, toolBarRect.height() / 2, toolBarRect.height() / 2);
    //
    QRectF toolBarActionRect(toolBarRect.left() + Ui::DesignSystem::layout().px(3),
                             toolBarRect.top() + Ui::DesignSystem::layout().px(3),
                             toolBarRect.height() - Ui::DesignSystem::layout().px(6),
                             toolBarRect.height() - Ui::DesignSystem::layout().px(6));
    for (int item = 0; item < 4; ++item) {
        painter.setBrush(item == 1
                             ? themeColor.secondary()
                             : ColorHelper::transparent(themeColor.onPrimary(),
                                                        Ui::DesignSystem::inactiveTextOpacity()));
        painter.drawRoundedRect(toolBarActionRect, toolBarActionRect.height() / 2,
                                toolBarActionRect.height() / 2);
        toolBarActionRect.moveLeft(toolBarActionRect.right() + Ui::DesignSystem::layout().px(3));
    }
    //
    // ... контент
    //
    const QRectF contentRect(
        sideBarRect.right() + Ui::DesignSystem::layout().px12(),
        toolBarRect.bottom() + Ui::DesignSystem::layout().px12(),
        contentsRect().width() - sideBarRect.width() - Ui::DesignSystem::layout().px24(),
        contentsRect().height() - toolBarRect.bottom() - Ui::DesignSystem::layout().px24());
    painter.fillRect(contentRect, themeColor.background());
    //
    painter.setBrush(ColorHelper::transparent(themeColor.onBackground(),
                                              Ui::DesignSystem::inactiveTextOpacity()));
    QRectF contentItemRect(contentRect.left() + Ui::DesignSystem::layout().px8(),
                           contentRect.top() + Ui::DesignSystem::layout().px8(),
                           contentRect.width() - Ui::DesignSystem::layout().px16(),
                           Ui::DesignSystem::layout().px2());
    painter.drawRoundedRect(contentItemRect, contentItemRect.height() / 2,
                            contentItemRect.height() / 2);
    //
    contentItemRect.moveTop(contentItemRect.bottom() + Ui::DesignSystem::layout().px4());
    contentItemRect.setRight(contentItemRect.right() - Ui::DesignSystem::layout().px4());
    painter.drawRoundedRect(contentItemRect, contentItemRect.height() / 2,
                            contentItemRect.height() / 2);
    //
    contentItemRect.moveTop(contentItemRect.bottom() + Ui::DesignSystem::layout().px4());
    contentItemRect.setLeft(contentItemRect.left() + Ui::DesignSystem::layout().px12());
    contentItemRect.setRight(contentItemRect.right() - Ui::DesignSystem::layout().px8());
    painter.drawRoundedRect(contentItemRect, contentItemRect.height() / 2,
                            contentItemRect.height() / 2);
    //
    contentItemRect.moveTop(contentItemRect.bottom() + Ui::DesignSystem::layout().px(3));
    contentItemRect.setLeft(contentItemRect.left() - Ui::DesignSystem::layout().px4());
    contentItemRect.setRight(contentItemRect.right() + Ui::DesignSystem::layout().px4());
    painter.drawRoundedRect(contentItemRect, contentItemRect.height() / 2,
                            contentItemRect.height() / 2);
    //
    contentItemRect.moveTop(contentItemRect.bottom() + Ui::DesignSystem::layout().px(3));
    painter.drawRoundedRect(contentItemRect, contentItemRect.height() / 2,
                            contentItemRect.height() / 2);
    //
    contentItemRect.moveTop(contentItemRect.bottom() + Ui::DesignSystem::layout().px4());
    contentItemRect.setLeft(contentItemRect.left() - Ui::DesignSystem::layout().px8());
    contentItemRect.setRight(contentItemRect.right() + Ui::DesignSystem::layout().px8());
    painter.drawRoundedRect(contentItemRect, contentItemRect.height() / 2,
                            contentItemRect.height() / 2);
    //
    contentItemRect.moveTop(contentItemRect.bottom() + Ui::DesignSystem::layout().px4());
    contentItemRect.setRight(contentItemRect.right() - Ui::DesignSystem::layout().px16());
    painter.drawRoundedRect(contentItemRect, contentItemRect.height() / 2,
                            contentItemRect.height() / 2);

    //
    // Отключаем кадрирование
    //
    painter.setClipping(false);

    //
    // Используется ли текущая тема для самого приложения
    //
    const bool isThemeUsed = Ui::DesignSystem::theme() == d->theme;

    //
    // Название темы
    //
    painter.setPen(isThemeUsed ? Ui::DesignSystem::color().secondary() : textColor());
    painter.setFont(Ui::DesignSystem::font().body1());
    QString themeName;
    switch (d->theme) {
    case Ui::ApplicationTheme::Light: {
        themeName = tr("Light", "Theme, will be used in case \"Theme: Light\"");
        break;
    }
    case Ui::ApplicationTheme::DarkAndLight: {
        themeName = tr("Mixed", "Theme, will be used in case \"Theme: Dark and Light\"");
        break;
    }
    case Ui::ApplicationTheme::Dark: {
        themeName = tr("Dark", "Theme, will be used in case \"Theme: Dark\"");
        break;
    }
    case Ui::ApplicationTheme::Custom: {
        themeName = tr("Custom", "Theme, will be used in case \"Theme: Custom\"");
        break;
    }
    }
    const QRectF nameRect(0, height() - Ui::DesignSystem::layout().px24(), width(),
                          Ui::DesignSystem::layout().px24());
    painter.drawText(nameRect, Qt::AlignCenter, themeName);

    //
    // Если тема установлена как текущая в приложении, выделяем её
    //
    if (isThemeUsed) {
        painter.translate(-contentsRect().topLeft());
        painter.setPen(
            QPen(Ui::DesignSystem::color().secondary(), Ui::DesignSystem::layout().px2()));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(contentsRect(), Ui::DesignSystem::layout().px12(),
                                Ui::DesignSystem::layout().px12());
    }
}

void ThemePreview::mousePressEvent(QMouseEvent* _event)
{
    Widget::mousePressEvent(_event);

    if (_event->button() == Qt::LeftButton) {
        emit themePressed(d->theme);
    } else if (_event->button() == Qt::RightButton) {
        auto copyAction = new QAction;
        copyAction->setText(tr("Copy theme HASH"));
        connect(copyAction, &QAction::triggered, this, [this] {
            QGuiApplication::clipboard()->setText(Ui::DesignSystem::color(d->theme).toString());
        });

        auto menu = new ContextMenu(this);
        menu->setActions({ copyAction });
        menu->setBackgroundColor(Ui::DesignSystem::color().background());
        menu->setTextColor(Ui::DesignSystem::color().onBackground());
        connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

        menu->showContextMenu(_event->globalPos());
    }
}

void ThemePreview::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setContentsMargins(QMargins(Ui::DesignSystem::layout().px2(), Ui::DesignSystem::layout().px2(),
                                Ui::DesignSystem::layout().px2(),
                                Ui::DesignSystem::layout().px(30)));
}
