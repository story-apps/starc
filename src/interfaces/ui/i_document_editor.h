#pragma once

#include <QtPlugin>

class QObject;
class QString;

namespace Ui
{

/**
 * @brief Интерфейс редактора документа
 */
class IDocumentEditor
{
public:
    virtual ~IDocumentEditor() = default;
};

} // namespace Ui

Q_DECLARE_INTERFACE(Ui::IDocumentEditor, "app.starc.Ui.IDocumentEditor")
