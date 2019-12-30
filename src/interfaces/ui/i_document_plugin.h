#pragma once

#include <QtPlugin>

class QWidget;

namespace Ui
{

/**
 * @brief Интерфейс плагина представления документа
 */
class IDocumentPlugin
{
public:
    virtual ~IDocumentPlugin() = default;

    /**
     * @brief Создать виджет представления
     */
    virtual QWidget* createWidget() = 0;
};

} // namespace Ui

Q_DECLARE_INTERFACE(Ui::IDocumentPlugin, "app.starc.Ui.IDocumentPlugin")
