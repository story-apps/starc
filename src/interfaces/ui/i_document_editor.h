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

    /**
     * @brief Активировать плагин
     */
    virtual void activatePlugin(const QString& _email) = 0;

    /**
     * @brief Получить майм-тип редактора
     */
    virtual QString mimeType() const = 0;

    /**
     * @brief Получить майм-тип документа, который может быть отредактирован данным редактором
     */
    virtual QString documentMimeType() const = 0;

    /**
     * @brief Получить майм-тип навигатора, который может использоваться совместно с редактором
     */
    virtual QString navigatorMimeType() const = 0;

    /**
     * @brief Связать редактор с заданным навигатором
     */
    virtual void setNavigator(QObject* _navigator) = 0;
};

} // namespace Ui

Q_DECLARE_INTERFACE(Ui::IDocumentEditor, "app.starc.Ui.IDocumentEditor")
