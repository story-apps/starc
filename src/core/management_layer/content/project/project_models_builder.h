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
 * @brief Билдер для работы с моделями документов проекта
 */
class ProjectModelsBuilder : public QObject
{
    Q_OBJECT

public:
    explicit ProjectModelsBuilder(BusinessLayer::AbstractImageWrapper* _imageWrapper, QObject* _parent = nullptr);
    ~ProjectModelsBuilder() override;

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

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
