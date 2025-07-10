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

    //
    // Очистим опции модулей
    //
    clearOptions(AppBarOptionsLevel::Modules);

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

    QAction* viewOptionAction = new QAction(this);
    viewOptionAction->setIconText(_icon);
    viewOptionAction->setText(_tooltip);
    viewOptionAction->setCheckable(true);
    viewOptionAction->setChecked(_isActive);
    viewOptionAction->setData(_mimeType);
    connect(viewOptionAction, &QAction::toggled, viewAction,
            [this, viewAction, viewOptionAction](bool _isChecked) {
                //
                // Если пользователь нажимает на кнопку текущего активного модуля
                //
                const auto modulesOptions = options(AppBarOptionsLevel::Modules);
                if (!_isChecked
                    && std::count_if(modulesOptions.begin(), modulesOptions.end(),
                                     [](const QAction* _option) { return _option->isChecked(); })
                        == 0) {
                    //
                    // ... не даём ему выключиться
                    //
                    viewOptionAction->setChecked(true);
                    return;
                }

                viewAction->setChecked(_isChecked);
                if (!_isChecked) {
                    return;
                }

                for (auto action : modulesOptions) {
                    if (action->isCheckable() && action != viewOptionAction) {
                        action->setChecked(false);
                    }
                }

                update();
            });
    connect(viewAction, &QAction::toggled, viewOptionAction, [viewOptionAction](bool _isChecked) {
        QSignalBlocker signalBlocker(viewOptionAction);
        viewOptionAction->setChecked(_isChecked);
    });
    auto modulesOptions = options(AppBarOptionsLevel::Modules);
    modulesOptions.append(viewOptionAction);
    setOptions(modulesOptions, AppBarOptionsLevel::Modules);

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
    for (auto action : options(AppBarOptionsLevel::Modules)) {
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
    AppBar::updateTranslations();

    actions().at(0)->setToolTip(tr("Show main menu"));
}

void ProjectToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui
