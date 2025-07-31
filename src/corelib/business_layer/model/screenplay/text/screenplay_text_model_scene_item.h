#pragma once

#include "../screenplay_dictionaries_model.h"

#include <business_layer/model/text/text_model_group_item.h>

#include <chrono>


namespace BusinessLayer {

class ScreenplayTextModel;

/**
 * @brief Класс элементов сцен модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelSceneItem : public TextModelGroupItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum {
        SceneDurationRole = TextModelGroupItem::GroupUserRole + 1,
        SceneDescriptionRole,
    };

public:
    explicit ScreenplayTextModelSceneItem(const ScreenplayTextModel* _model);
    ~ScreenplayTextModelSceneItem() override;

    /**
     * @brief Ресурсы сцены
     */
    QVector<BreakdownSceneResource> resources() const;
    void storeResource(const QUuid& _uuid, int _qty, const QString& _descriptionForScene);
    void removeResource(const QUuid& _uuid);

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration() const;

    /**
     * @brief Список битов
     */
    QVector<QString> beats() const;

    /**
     * @brief Описание сцены (объединённые биты)
     */
    QString description(const QString& _separator = " ") const;

    /**
     * @brief Определяем интерфейс получения данных сцены
     */
    QVariant data(int _role) const override;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(TextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(TextModelItem* _item) const override;

    /**
     * @brief Подходит ли элемент под условия заданного фильтра
     */
    bool isFilterAccepted(const QString& _text, bool _isCaseSensitive,
                          int _filterType) const override;

protected:
/**
 * @brief Считываем дополнительный контент
 */
#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
    QStringView readCustomContent(QXmlStreamReader& _contentReader) override;
#else
    QStringRef readCustomContent(QXmlStreamReader& _contentReader) override;
#endif

    /**
     * @brief Сформировать xml-блок с кастомными данными элемента
     */
    QByteArray customContent() const override;

    /**
     * @brief Обновляем текст сцены при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
