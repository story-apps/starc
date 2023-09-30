#include "project_tool_bar.h"

#include <ui/design_system/design_system.h>

#include <QAction>


namespace Ui {

ProjectToolBar::ProjectToolBar(QWidget* _parent)
    : AppBar(_parent)
{
    QAction* menuAction = new QAction(this);
    menuAction->setText(u8"\U000f035c");
    addAction(menuAction);
    connect(menuAction, &QAction::triggered, this, &ProjectToolBar::menuPressed);
}

void ProjectToolBar::clearViews()
{
    //
    // Оставляем только кнопку меню
    //
    auto actions = this->actions();
    while (actions.size() > 1) {
        auto action = actions.takeLast();
        removeAction(action);
        action->deleteLater();
    }

    update();
}

void ProjectToolBar::addView(const QString& _mimeType, const QString& _icon,
                             const QString& _tooltip, bool _isActive)
{
    QAction* viewAction = new QAction(this);
    viewAction->setText(_icon);
    viewAction->setToolTip(_tooltip);
    viewAction->setCheckable(true);
    viewAction->setChecked(_isActive);
    viewAction->setData(_mimeType);
    addAction(viewAction);
    connect(viewAction, &QAction::toggled, this,
            [this, _mimeType] { emit viewPressed(_mimeType); });

    update();
}

QString ProjectToolBar::currentViewMimeType() const
{
    for (int actionIndex = 1; actionIndex < actions().size(); ++actionIndex) {
        const auto action = actions().at(actionIndex);
        if (action->isChecked()) {
            return action->data().toString();
        }
    }

    return {};
}

void ProjectToolBar::setCurrentViewMimeType(const QString& _mimeType)
{
    for (int actionIndex = 1; actionIndex < actions().size(); ++actionIndex) {
        const auto action = actions().at(actionIndex);
        QSignalBlocker signalBlocker(action);

        if (action->data().toString() == _mimeType) {
            if (action->isChecked()) {
                continue;
            }

            action->setChecked(true);
        } else {
            action->setChecked(false);
        }
    }
}

void ProjectToolBar::updateTranslations()
{
    actions().at(0)->setToolTip(tr("Show main menu"));
}

void ProjectToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui
