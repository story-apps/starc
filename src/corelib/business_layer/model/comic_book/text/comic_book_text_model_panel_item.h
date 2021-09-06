#pragma once

#include "comic_book_text_model_item.h"

#include <QString>

#include <chrono>

class QColor;
class QXmlStreamReader;


namespace BusinessLayer {

/**
 * @brief Класс элементов панелей модели комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModelPanelItem : public ComicBookTextModelItem
{
public:
    /**
     * @brief Номер панели
     */
    struct Number {
        QString value;

        bool operator==(const Number& _other) const;
    };

    /**
     * @brief Роли данных из модели
     */
    enum DataRole {
        PanelNumberRole = Qt::UserRole + 1,
        PanelHeadingRole,
        PanelTextRole,
        PanelColorRole,
        PanelInlineNotesSizeRole,
        PanelReviewMarksSizeRole,
        PanelDialoguesWordsSizeRole,
    };

public:
    ComicBookTextModelPanelItem();
    explicit ComicBookTextModelPanelItem(QXmlStreamReader& _contentReader);
    ~ComicBookTextModelPanelItem() override;

    /**
     * @brief Номер панели
     */
    Number number() const;
    bool setNumber(int _number);

    /**
     * @brief Цвет панели
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Получить количество слов в репликах
     */
    int dialoguesWordsCount() const;

    /**
     * @brief Определяем интерфейс получения данных панели
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
     * @brief Обновляем текст панели при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
