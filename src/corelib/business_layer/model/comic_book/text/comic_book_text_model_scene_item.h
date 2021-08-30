#pragma once

#include "comic_book_text_model_item.h"

#include <QString>

#include <chrono>

class QColor;
class QXmlStreamReader;


namespace BusinessLayer {

/**
 * @brief Класс элементов сцен модели сценария
 */
class CORE_LIBRARY_EXPORT ComicBookTextModelSceneItem : public ComicBookTextModelItem
{
public:
    /**
     * @brief Номер сцены
     */
    struct Number {
        QString value;

        bool operator==(const Number& _other) const;
    };

    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        SceneNumberRole = Qt::UserRole + 1,
        SceneHeadingRole,
        SceneTextRole,
        SceneColorRole,
        SceneInlineNotesSizeRole,
        SceneReviewMarksSizeRole,
        SceneDurationRole,
    };

public:
    ComicBookTextModelSceneItem();
    explicit ComicBookTextModelSceneItem(QXmlStreamReader& _contentReader);
    ~ComicBookTextModelSceneItem() override;

    /**
     * @brief Номер сцены
     */
    Number number() const;
    bool setNumber(int _number, const QString& _prefix);

    /**
     * @brief Цвет сцены
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Длительность сцены
     */
    std::chrono::milliseconds duration() const;

    /**
     * @brief Определяем интерфейс получения данных сцены
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
     * @brief Обновляем текст сцены при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
