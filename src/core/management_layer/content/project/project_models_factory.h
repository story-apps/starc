#pragma once

#include <QScopedPointer>

namespace BusinessLayer {
class AbstractModel;
}

namespace Domain {
class DocumentObject;
}


namespace ManagementLayer
{

/**
 * @brief Фабрика для работы с моделями документов проекта
 */
class ProjectModelsFactory
{
public:
    ProjectModelsFactory();
    ~ProjectModelsFactory();

    /**
     * @brief Сбросить все загруженные модели
     */
    void clear();

    /**
     * @brief Получить модель для заданного документа
     */
    BusinessLayer::AbstractModel* modelFor(Domain::DocumentObject* _document);

    /**
     * @brief Получить список загруженных моделей документов
     */
    QVector<BusinessLayer::AbstractModel*> models()  const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
