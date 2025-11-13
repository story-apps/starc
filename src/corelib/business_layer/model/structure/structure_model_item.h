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
                                bool _readOnly, bool _comparison);
    explicit StructureModelItem(const StructureModelItem& _other);
    ~StructureModelItem() override;

    /**
     * @brief Уникальный идентификатор элемента
     */
    const QUuid& uuid() const;

    /**
     * @brief Тип элемента
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
     * @brief Является ли элемент результатом сравнения документов
     */
    bool isComparison() const;
    void setComparison(bool _comparison);

    /**
     * @brief Драфты документа (добавляются в начало списка, т.е. сверху находятся наиболее свежие)
     */
    const QVector<StructureModelItem*>& drafts() const;
    StructureModelItem* addDraft(StructureModelItem* _draft);
    StructureModelItem* addDraft(const QString& _name, const QColor& _color, bool _readOnly,
                                 bool _comparison);
    void removeDraft(int _draftIndex);

    /**
     * @brief Переопределяем интерфейс для получения данных модели по роли
     */
    QVariant data(int _role) const override;

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    StructureModelItem* parent() const override;
    StructureModelItem* childAt(int _index) const override;

    /**
     * @brief Методы необходимые для того, чтобы работало определение списка изменений модели при
     *        наложении патчей, когда надо сравнить две версии xml-документа
     */
    /** @{ */
    int subtype() const;
    QByteArray toXml() const;
    bool isEqual(const StructureModelItem* _other) const;
    void copyFrom(const StructureModelItem* _other) const;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
