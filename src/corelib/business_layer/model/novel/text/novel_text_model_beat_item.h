#pragma once

#include <business_layer/model/text/text_model_group_item.h>

#include <chrono>


namespace BusinessLayer {

class NovelTextModel;

/**
 * @brief Класс элементов битов модели сценария
 */
class CORE_LIBRARY_EXPORT NovelTextModelBeatItem : public TextModelGroupItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum {
        BeatDurationRole = TextModelGroupItem::GroupUserRole + 1,
    };

public:
    explicit NovelTextModelBeatItem(const NovelTextModel* _model);
    ~NovelTextModelBeatItem() override;

    /**
     * @brief Количество слов
     */
    int wordsCount() const;

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount() const;

    /**
     * @brief Определяем интерфейс получения данных бита
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
     * @brief Обновляем текст бита при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
