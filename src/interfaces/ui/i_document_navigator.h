#pragma once

#include <QtPlugin>

class QString;

namespace Ui
{

/**
 * @brief Интерфейс навигатора документа
 */
class IDocumentNavigator
{
public:
    virtual ~IDocumentNavigator() = default;

    /**
     * @brief Активировать плагин
     */
    virtual void activatePlugin(const QString& _email) = 0;

    /**
     * @brief Получить майм-тип навигатора
     */
    virtual QString mimeType() const = 0;

    /**
     * @brief Получить майм-тип документа, с которым может работать данный навигатор
     */
    virtual QString documentMimeType() const = 0;
};

} // namespace Ui

Q_DECLARE_INTERFACE(Ui::IDocumentNavigator, "app.starc.Ui.IDocumentNavigator")
