#pragma once

#include <QtContainerFwd>
#include <QtPlugin>

namespace BusinessLayer {
class AbstractModel;
}

namespace Ui {
class IDocumentView;
}

class QObject;
class QUuid;
class QWidget;

namespace ManagementLayer {

/**
 * @brief Режим редактирования документа
 */
enum class DocumentEditingMode {
    Edit = 0,
    Comment,
    Read,
    Mixed,
};

/**
 * @brief Интерфейс менеджера документа
 */
class IDocumentManager
{
public:
    virtual ~IDocumentManager() = default;

    /**
     * @brief Вернуть указатель на себя как QObject, если менеджер является наследником  QObject
     */
    virtual QObject* asQObject()
    {
        return nullptr;
    }

    /**
     * @brief Является ли менеджер управляющим навигатора
     */
    virtual bool isNavigationManager() const
    {
        return false;
    }

    /**
     * @brief Добавить модель в плагин для необходимой внутренней обработки
     */
    virtual void configureModelProcessing(BusinessLayer::AbstractModel*)
    {
    }

    /**
     * @brief Представление документа для основного окна приложения
     */
    virtual Ui::IDocumentView* view() = 0;
    virtual Ui::IDocumentView* view(BusinessLayer::AbstractModel*) = 0;
    virtual Ui::IDocumentView* secondaryView() = 0;
    virtual Ui::IDocumentView* secondaryView(BusinessLayer::AbstractModel*) = 0;

    /**
     * @brief Представление документа для отдельного окна с заданной моделью
     */
    virtual Ui::IDocumentView* createView(BusinessLayer::AbstractModel*) = 0;

    /**
     * @brief Сбросить все модели данных, к которым подсоединены представления
     */
    virtual void resetModels() = 0;

    /**
     * @brief Перенастроить редактор, которым управляет менеджер,
     *        в соответствии с заданным списком изменившихся параметров
     */
    virtual void reconfigure(const QStringList&)
    {
    }

    /**
     * @brief Связать с другим менеджером
     */
    virtual void bind(IDocumentManager*)
    {
    }

    /**
     * @brief Сохранить параметры плагина
     */
    virtual void saveSettings()
    {
    }

    /**
     * @brief Перепроверить воможность редактирования
     * @note Актуально для платных плагинов
     */
    virtual void checkAvailabilityToEdit(bool)
    {
    }

    /**
     * @brief Задать режим редактирования документа
     */
    virtual void setEditingMode(DocumentEditingMode)
    {
    }

    /**
     * @brief Задать режим редактирования конкретных документов
     */
    virtual void setEditingPermissions(const QHash<QUuid, DocumentEditingMode>&)
    {
    }

    /**
     * @brief Задать доступное кол-во кредитов для использования ИИ инструментов
     */
    virtual void setAvailableCredits(int)
    {
    }


    //
    // Сигналы
    //


    // /**
    //  * @brief Пользователь хочет проапгрейдить свой аккаунт
    //  */
    // void upgradeToProRequested();

    // /**
    //  * @brief Пользователь хочет проапгрейдить свой аккаунт
    //  */
    // void upgradeToCreatorRequested();

    // /**
    //  * @brief Запрос на создание документа заданного типа
    //  */
    // void createDocumentRequested(Domain::DocumentObjectType _type);


    // /**
    //  * @brief Запрос отправки документа на проверку
    //  */
    // void sendDocumentToReviewRequested(const QUuid& _documentUuid, const QString& _comment);
};

} // namespace ManagementLayer

Q_DECLARE_INTERFACE(ManagementLayer::IDocumentManager, "app.starc.ManagementLayer.IDocumentManager")
