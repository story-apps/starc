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
                                const QString& _name, const QColor& _color, bool _visible,
                                bool _readOnly);
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
    void setColor(const QColor& _color);

    /**
     * @brief Должен ли быть виден элемент модели
     */
    bool isVisible() const;
    void setVisible(bool _visible);

    /**
     * @brief Доступен ли элемент для редактирования
     */
    bool isReadOnly() const;
    void setReadOnly(bool _readOnly);

    /**
     * @brief Версии документа (добавляются в начало списка, т.е. сверху находятся наиболее свежие
     *        версии)
     */
    const QVector<StructureModelItem*>& versions() const;
    StructureModelItem* addVersion(StructureModelItem* _version);
    StructureModelItem* addVersion(const QString& _name, const QColor& _color, bool _readOnly);
    void setVersions(const QVector<StructureModelItem*>& _versions);
    void removeVersion(int _versionIndex);

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
