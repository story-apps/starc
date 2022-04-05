#pragma once

#include <business_layer/model/text/text_model_folder_item.h>

#include <chrono>


namespace BusinessLayer {

class AudioplayTextModel;

/**
 * @brief Класс элементов папок модели аудиопьесы
 */
class CORE_LIBRARY_EXPORT AudioplayTextModelFolderItem : public TextModelFolderItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum {
        FolderDurationRole = TextModelFolderItem::FolderUserRole + 1,
    };

public:
    explicit AudioplayTextModelFolderItem(const AudioplayTextModel* _model);
    ~AudioplayTextModelFolderItem() override;

    /**
     * @brief Длительность папки
     */
    std::chrono::milliseconds duration() const;

    /**
     * @brief Определяем интерфейс получения данных папки
     */
    QVariant data(int _role) const override;

protected:
    /**
     * @brief Обновляем текст папки при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
