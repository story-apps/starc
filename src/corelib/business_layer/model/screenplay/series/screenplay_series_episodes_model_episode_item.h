#pragma once

#include "screenplay_series_episodes_model_item.h"

class QColor;


namespace BusinessLayer {

/**
 * @brief Эпизод
 */
class CORE_LIBRARY_EXPORT ScreenplaySeriesEpisodesModelEpisodeItem
    : public ScreenplaySeriesEpisodesModelItem
{
public:
    ScreenplaySeriesEpisodesModelEpisodeItem();
    ~ScreenplaySeriesEpisodesModelEpisodeItem() override;

    /**
     * @brief Тип элемента модели
     */
    ScreenplaySeriesEpisodesModelItemType type() const override;

    /**
     * @brief Является ли эпизод контейнером заголовков сюжетных линий
     */
    bool isStoryLinesContainer() const;
    void setStoryLinesContainer(bool _container);

    /**
     * @brief Название эпизода
     */
    QString name() const;
    void setName(const QString& _name);

    /**
     * @brief Цвет эпизода
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

} // namespace BusinessLayer
