#pragma once

#include "screenplay_series_episodes_model_item.h"

class QColor;


namespace BusinessLayer {

/**
 * @brief Файл проекта
 */
class ScreenplaySeriesEpisodesModelStoryLineItem : public ScreenplaySeriesEpisodesModelItem
{
public:
    ScreenplaySeriesEpisodesModelStoryLineItem();
    ScreenplaySeriesEpisodesModelStoryLineItem(
        const ScreenplaySeriesEpisodesModelStoryLineItem& _other);
    const ScreenplaySeriesEpisodesModelStoryLineItem& operator=(
        const ScreenplaySeriesEpisodesModelStoryLineItem& _other);
    ~ScreenplaySeriesEpisodesModelStoryLineItem();

    /**
     * @brief Тип элемента модели
     */
    ScreenplaySeriesEpisodesModelItemType type() const override;

    /**
     * @brief Является ли сюжетная линия заголовком группы для эпизодов
     */
    bool isStoryLineTitle() const;
    void setStoryLineTitle(bool _title);

    /**
     * @brief Название проекта
     */
    QString name() const;
    void setName(const QString& _name);

    /**
     * @brief Цвет состояния
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Подходит ли элемент под условия заданного фильтра
     */
    bool isFilterAccepted(const QString& _text, bool _isCaseSensitive,
                          int _filterType) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

bool operator==(const ScreenplaySeriesEpisodesModelStoryLineItem& _lhs,
                const ScreenplaySeriesEpisodesModelStoryLineItem& _rhs);

} // namespace BusinessLayer
