#pragma once

#include <business_layer/model/text/text_model_group_item.h>


namespace BusinessLayer {

class ComicBookTextModel;

/**
 * @brief Класс элементов страниц модели комикса
 */
class CORE_LIBRARY_EXPORT ComicBookTextModelPageItem : public TextModelGroupItem
{
public:
    /**
     * @brief Номер страницы
     */
    struct PageNumber {
        int fromPage = 0;
        int pageCount = 0;
        QString text;
    };

    /**
     * @brief Роли данных из модели
     */
    enum {
        PagePanelsCountRole = TextModelGroupItem::GroupUserRole + 1,
        PageDialoguesWordsCountRole,
        PageHasNumberingErrorRole,
    };

public:
    explicit ComicBookTextModelPageItem(const ComicBookTextModel* _model);
    ~ComicBookTextModelPageItem() override;

    /**
     * @brief Номер страницы
     */
    std::optional<PageNumber> pageNumber() const;
    void setPageNumber(int& _fromNumber, const QVector<QString>& _singlePageIntros,
                       const QVector<QString>& _multiplePageIntros);

    /**
     * @brief Получить количество панелей на странице
     */
    int panelsCount() const;

    /**
     * @brief Получить количество слов
     */
    int dialoguesWordsCount() const;

    /**
     * @brief Определяем интерфейс получения данных страницы
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
     * @brief Обновляем текст страницы при изменении кого-то из детей
     */
    void handleChange() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
