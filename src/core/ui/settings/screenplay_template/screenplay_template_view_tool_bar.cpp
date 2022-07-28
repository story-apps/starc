#include "screenplay_template_view_tool_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QAction>


namespace Ui {

class ScreenplayTemplateViewToolBar::Implementation
{
public:
    explicit Implementation(QObject* _parent);


    QAction* save = nullptr;
    QAction* exportToFile = nullptr;
};

ScreenplayTemplateViewToolBar::Implementation::Implementation(QObject* _parent)
    : save(new QAction(_parent))
    , exportToFile(new QAction(_parent))
{
    save->setIconText(u8"\U000f0193");
    exportToFile->setIconText(u8"\U000f0207");
}


// ****


ScreenplayTemplateViewToolBar::ScreenplayTemplateViewToolBar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    setCurtain(true);

    connect(d->save, &QAction::triggered, this, &ScreenplayTemplateViewToolBar::savePressed);

    addActions({ d->save /*, d->exportToFile*/ });
}

ScreenplayTemplateViewToolBar::~ScreenplayTemplateViewToolBar() = default;

void ScreenplayTemplateViewToolBar::updateTranslations()
{
    d->save->setToolTip(tr("Save template"));
    d->exportToFile->setToolTip(tr("Export template to the file"));
}

void ScreenplayTemplateViewToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    resize(sizeHint());
    setBackgroundColor(ColorHelper::nearby(DesignSystem::color().background()));
    setTextColor(DesignSystem::color().onBackground());
    raise();

    move(QPointF(isLeftToRight()
                     ? Ui::DesignSystem::layout().px24()
                     : parentWidget()->width() - width() - Ui::DesignSystem::layout().px24(),
                 -Ui::DesignSystem::card().shadowMargins().top())
             .toPoint());
}

} // namespace Ui
