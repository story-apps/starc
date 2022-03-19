#pragma once

#include <QVector>

class QAction;
class QWidget;

namespace Ui {

/**
 * @brief Интерфейс редактора документа
 */
class IDocumentView
{
public:
    virtual ~IDocumentView() = default;

    /**
     * @brief Вернуть указатель на себя как QWidget, если представление является наследником QWidget
     */
    virtual QWidget* asQWidget() = 0;

    /**
     * @brief Включить/отключить полноэкранный режим
     */
    virtual void toggleFullScreen(bool)
    {
    }

    /**
     * @brief Список опций редактора
     */
    virtual QVector<QAction*> options() const
    {
        return {};
    }
};

} // namespace Ui
