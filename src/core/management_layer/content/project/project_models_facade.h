#pragma once

#include <QObject>

namespace BusinessLayer {
class AbstractImageWrapper;
class AbstractModel;
}

namespace Domain {
class DocumentObject;
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
    explicit ProjectModelsFacade(BusinessLayer::AbstractImageWrapper* _imageWrapper, QObject* _parent = nullptr);
    ~ProjectModelsFacade() override;

    /**
     * @brief Сбросить все загруженные модели
     */
    void clear();

    /**
     * @brief Получить модель для заданного документа
     */
    BusinessLayer::AbstractModel* modelFor(Domain::DocumentObject* _document);

    /**
     * @brief Удалить модель для заданного документа
     */
    void removeModelFor(Domain::DocumentObject* _document);

    /**
     * @brief Получить список загруженных моделей документов
     */
    QVector<BusinessLayer::AbstractModel*> models()  const;

signals:
    /**
     * @brief Изменился контент модели
     */
    void modelContentChanged(BusinessLayer::AbstractModel* _model, const QByteArray& _undo, const QByteArray& _redo);

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
     * @brief Неоходимо создать локацию с заданным именем
     */
    void createLocationRequested(const QString& _name);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
