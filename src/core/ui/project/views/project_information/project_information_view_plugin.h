#pragma once

#include "../view_plugin_global.h"

#include <interfaces/ui/i_document_plugin.h>

#include <QObject>


namespace Ui
{

class VIEW_PLUGIN_EXPORT ProjectInformationViewPlugin : public QObject, public IDocumentPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.Ui.IDocumentPlugin")
    Q_INTERFACES(Ui::IDocumentPlugin)

public:
    explicit ProjectInformationViewPlugin(QObject* _parent = nullptr);

    QWidget* createWidget() override;
};

} // namespace Ui
