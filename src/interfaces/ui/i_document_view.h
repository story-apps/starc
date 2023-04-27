#pragma once

#include <QVector>

class QAction;
class QModelIndex;
class QWidget;

namespace Domain {
struct CursorInfo;
}

namespace ManagementLayer {
enum class DocumentEditingMode;
}

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

    /**
     * @brief Задать параметры проекта
     */
    virtual void setProjectInfo(bool _isRemote, bool _isOwner)
    {
    }

    /**
     * @brief Задать режим редактирования
     */
    virtual void setEditingMode(ManagementLayer::DocumentEditingMode _editingMode)
    {
    }

    /**
     * @brief Задать список курсоров соавторов в редакторе
     */
    virtual void setCursors(const QVector<Domain::CursorInfo>& _cursors)
    {
    }

    /**
     * @brief Сделать элемент модели с заданным индексом текущим
     */
    virtual void setCurrentModelIndex(const QModelIndex& _index)
    {
    }

    /**
     * @brief Задать доступное кол-во кредитов для использования ИИ инструментов
     */
    virtual void setAvailableCredits(int _credits)
    {
    }

    /**
     * @brief Задать сгенерированный текст
     */
    virtual void setGeneratedText(const QString& _text)
    {
    }

    /**
     * @brief Задать сгенерированный текст
     */
    virtual void setRephrasedText(const QString& _text)
    {
    }
};

} // namespace Ui
