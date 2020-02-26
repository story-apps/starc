#include "screenplay_text_edit_toolbar.h"

#include <ui/design_system/design_system.h>

#include <QAction>
#include <QVariantAnimation>


namespace Ui
{

class ScreenplayTextEditToolBar::Implementation
{
public:
    QAction* paragraphTypeAction = nullptr;
    QAction* fastFormatAction = nullptr;
    QAction* searchAction = nullptr;
    QAction* reviewAction = nullptr;

    QAction* expandToolBarAction = nullptr;
    QVariantAnimation widthAnimation;
};


// ****


ScreenplayTextEditToolBar::ScreenplayTextEditToolBar(QWidget* _parent)
    : FloatingToolBar(_parent),
      d(new Implementation)
{
    QAction* undoAction = new QAction;
    undoAction->setIconText("\uf54c");
    addAction(undoAction);

    QAction* redoAction = new QAction;
    redoAction->setIconText("\uf44e");
    addAction(redoAction);

    d->paragraphTypeAction = new QAction;
    d->paragraphTypeAction->setText(tr("Scene heading  ▾"));
    addAction(d->paragraphTypeAction);
    connect(d->paragraphTypeAction, &QAction::triggered, this, [this] {

    });

    d->fastFormatAction = new QAction;
    d->fastFormatAction->setIconText("\uf328");
    d->fastFormatAction->setVisible(false);
    addAction(d->fastFormatAction);

    d->searchAction = new QAction;
    d->searchAction->setIconText("\uf349");
    d->searchAction->setVisible(false);
    addAction(d->searchAction);

    d->reviewAction = new QAction;
    d->reviewAction->setIconText("\ufe14");
    d->reviewAction->setVisible(false);
    addAction(d->reviewAction);

    d->expandToolBarAction = new QAction;
    d->expandToolBarAction->setIconText("\uf1d9");
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

    designSystemChangeEvent(nullptr);
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

void ScreenplayTextEditToolBar::updateTranslations()
{

}

void ScreenplayTextEditToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    const qreal additionalWidth = [this] {
        const QFontMetricsF fontMetrics(Ui::DesignSystem::font().body1());
        qreal width = 0.0;
        for (const auto action : actions()) {
            if (action->text().length() > 1) {
                width += fontMetrics.horizontalAdvance(action->text());
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
}

} // namespace Ui
