#pragma once

#include "screenplay_text_model_item.h"

class QDomElement;


namespace BusinessLayer
{

/**
 * @brief Класс элементов папок модели сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayTextModelFolderItem : public ScreenplayTextModelItem
{
public:
    ScreenplayTextModelFolderItem();
    explicit ScreenplayTextModelFolderItem(const QDomElement& _node);
    ~ScreenplayTextModelFolderItem() override;

    /**
     * @brief Определяем интерфейс получения данных сцены
     */
    QVariant data(int _role) const override;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QString toXml() const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
