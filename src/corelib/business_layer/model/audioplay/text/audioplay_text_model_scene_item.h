#pragma once

#include <business_layer/model/text/text_model_group_item.h>

#include <chrono>


namespace BusinessLayer {

class AudioplayTextModel;

/**
 * @brief Класс элементов сцен модели аудиопостановки
 */
class CORE_LIBRARY_EXPORT AudioplayTextModelSceneItem : public TextModelGroupItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum {
        SceneDurationRole = TextModelGroupItem::GroupUserRole + 1,
    };

public:
    explicit AudioplayTextModelSceneItem(const AudioplayTextModel* _model);
    ~AudioplayTextModelSceneItem() override;

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration() const;

    /**
     * @brief Определяем интерфейс получения данных сцены
     */
    QVariant data(int _role) const override;

protected:
    /**
     * @brief Считываем дополнительный контент
     */
    QStringRef readCustomContent(QXmlStreamReader& _contentReader) override;

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
