#pragma once

#include "comic_book_text_model_item.h"

#include <Qt>

#include <chrono>

class QColor;
class QXmlStreamReader;


namespace BusinessLayer {

/**
 * @brief Класс элементов страниц модели комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModelPageItem : public ComicBookTextModelItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        PageNameRole = Qt::UserRole + 1,
        PageColorRole,
    };

public:
    ComicBookTextModelPageItem();
    explicit ComicBookTextModelPageItem(QXmlStreamReader& _contentReader);
    ~ComicBookTextModelPageItem() override;

    /**
     * @brief Цвет страницы
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Определяем интерфейс получения данных страницы
     */
    QVariant data(int _role) const override;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;
    QByteArray toXml(ComicBookTextModelItem* _from, int _fromPosition, ComicBookTextModelItem* _to,
                     int _toPosition, bool _clearUuid) const;
    QByteArray xmlHeader(bool _clearUuid = false) const;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(ComicBookTextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(ComicBookTextModelItem* _item) const override;

protected:
    /**
     * @brief Обновляем текст страницы при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
