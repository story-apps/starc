#pragma once

#include "text_model_item.h"

#include <Qt>

class QColor;
class QXmlStreamReader;


namespace BusinessLayer {

/**
 * @brief Класс элементов папок модели текста
 */
class TextModelFolderItem : public TextModelItem
{
public:
    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        FolderNameRole = Qt::UserRole + 1,
        FolderColorRole,
    };

public:
    explicit TextModelFolderItem(const TextModel* _model);
    TextModelFolderItem(const TextModel* _model, QXmlStreamReader& _contentReader);
    ~TextModelFolderItem() override;

    /**
     * @brief Цвет папки
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Определяем интерфейс получения данных папки
     */
    QVariant data(int _role) const override;

    /**
     * @brief Определяем интерфейс для получения XML блока
     */
    QByteArray toXml() const override;
    QByteArray toXml(TextModelItem* _from, int _fromPosition, TextModelItem* _to,
                     int _toPosition, bool _clearUuid) const;
    QByteArray xmlHeader(bool _clearUuid = false) const;

    /**
     * @brief Скопировать контент с заданного элемента
     */
    void copyFrom(TextModelItem* _item) override;

    /**
     * @brief Проверить равен ли текущий элемент заданному
     */
    bool isEqual(TextModelItem* _item) const override;

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
