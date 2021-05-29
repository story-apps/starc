#pragma once

#include <QObject>
#include <QUuid>

namespace BusinessLayer {
class AbstractImageWrapper;
class AbstractModel;
class StructureModel;
}

namespace Domain {
class DocumentObject;
enum class DocumentObjectType;
}


namespace ManagementLayer
{

/**
 * @brief Фасад для работы с моделями документов проекта
 */
class ProjectModelsFacade : public QObject
{
    Q_OBJECT

public:
    explicit ProjectModelsFacade(BusinessLayer::StructureModel* _projectStructureModel,
        BusinessLayer::AbstractImageWrapper* _imageWrapper, QObject* _parent = nullptr);
    ~ProjectModelsFacade() override;

    /**
     * @brief Сбросить все загруженные модели
     */
    void clear();

    /**
     * @brief Получить модель для заданного документа
     */
    BusinessLayer::AbstractModel* modelFor(const QUuid& _uuid);
    BusinessLayer::AbstractModel* modelFor(Domain::DocumentObjectType _type);
    BusinessLayer::AbstractModel* modelFor(Domain::DocumentObject* _document);

    /**
     * @brief Получить список всех моделей заданного типа
     */
    QVector<BusinessLayer::AbstractModel*> modelsFor(Domain::DocumentObjectType _type);

    /**
     * @brief Удалить модель для заданного документа
     */
    void removeModelFor(Domain::DocumentObject* _document);

    /**
     * @brief Получить список загруженных моделей документов
     */
    QVector<BusinessLayer::AbstractModel*> loadedModels()  const;

signals:
    /**
     * @brief Изменилось название модели
     */
    void modelNameChanged(BusinessLayer::AbstractModel* _model, const QString& _name);

    /**
     * @brief Изменился контент модели
     */
    void modelContentChanged(BusinessLayer::AbstractModel* _model, const QByteArray& _undo, const QByteArray& _redo);

    /**
     * @brief Запрос на отмену последнего действия в модели
     */
    void modelUndoRequested(BusinessLayer::AbstractModel* _model, int _undoStep);

    /**
     * @brief Изменилось название проекта
     */
    void projectNameChanged(const QString& _name);

    /**
     * @brief Изменилось короткое описание проекта
     */
    void projectLoglineChanged(const QString& _logline);

    /**
     * @brief Изменилась обложка проекта
     */
    void projectCoverChanged(const QPixmap& _cover);

    /**
     * @brief Сменилась видимость элемента сценария
     */
    /** @{ */
    void screenplayTitlePageVisibilityChanged(BusinessLayer::AbstractModel* _screenplayModel, bool _visible);
    void screenplaySynopsisVisibilityChanged(BusinessLayer::AbstractModel* _screenplayModel, bool _visible);
    void screenplayTreatmentVisibilityChanged(BusinessLayer::AbstractModel* _screenplayModel, bool _visible);
    void screenplayTextVisibilityChanged(BusinessLayer::AbstractModel* _screenplayModel, bool _visible);
    void screenplayStatisticsVisibilityChanged(BusinessLayer::AbstractModel* _screenplayModel, bool _visible);
    /** @} */

    /**
     * @brief Необходимо создать персонажа с заданным именем
     */
    void createCharacterRequested(const QString& _name, const QByteArray& _content);

    /**
     * @brief Изменилось имя персонажа
     */
    void characterNameChanged(const QString& _oldName, const QString& _newName);

    /**
     * @brief Неоходимо создать локацию с заданным именем
     */
    void createLocationRequested(const QString& _name, const QByteArray& _content);

    /**
     * @brief Изменилось название локации
     */
    void locationNameChanged(const QString& _oldName, const QString& _newName);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
