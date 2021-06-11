#pragma once

#include <business_layer/model/abstract_model_item.h>

#include <corelib_global.h>

class QColor;
class QUuid;

namespace Domain {
enum class DocumentObjectType;
}


namespace BusinessLayer {

/**
 * @brief Элемент структуры проекта
 */
class CORE_LIBRARY_EXPORT StructureModelItem : public AbstractModelItem
{
public:
    explicit StructureModelItem(const QUuid& _uuid, Domain::DocumentObjectType _type,
                                const QString& _name, const QColor& _color, bool _visible);
    explicit StructureModelItem(const StructureModelItem& _other);
    ~StructureModelItem() override;

    /**
     * @brief Уникальный идентификатор элемента
     */
    const QUuid& uuid() const;

    /**
     * @brief Иконка элемента
     */
    Domain::DocumentObjectType type() const;

    /**
     * @brief Название элемента
     */
    const QString& name() const;
    void setName(const QString& _name);

    /**
     * @brief Цвет элемента
     */
    const QColor& color() const;

    /**
     * @brief Должен ли быть виден элемент модели
     */
    bool visible() const;
    void setVisible(bool _visible);

    /**
     * @brief Переопределяем интерфейс для получения данных модели по роли
     */
    QVariant data(int _role) const override;

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    StructureModelItem* parent() const override;
    StructureModelItem* childAt(int _index) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
