#pragma once

#include <business_layer/model/abstract_model_item.h>

#include <QModelIndex>


namespace BusinessLayer {

/**
 * @brief Элемент модели разбивки сценария
 */
class ScreenplayBreakdownStructureModelItem : public AbstractModelItem
{
public:
    enum {
        DurationRole = Qt::UserRole + 1,
        HighlightedRole,
        ScreenplayIndexRole,
    };

public:
    explicit ScreenplayBreakdownStructureModelItem(const QString& _name,
                                                   const QModelIndex& _screenplayItemIndex = {});

    /**
     * @brief Переопределяем интерфейс для возврата элемента собственного класса
     */
    ScreenplayBreakdownStructureModelItem* parent() const override;
    ScreenplayBreakdownStructureModelItem* childAt(int _index) const override;

    /**
     * @brief Определяем интерфейс получения данных элемента
     */
    QVariant data(int _role) const override;

    /**
     * @brief Перекрываем метод добавления элемента, чтобы обновлять хронометраж
     */
    void appendItem(ScreenplayBreakdownStructureModelItem* _item);

    /**
     * @brief Является ли элемент сценой
     */
    bool isScene() const;

    /**
     * @brief Обновить хронометраж
     */
    void updateDuration();

    /**
     * @brief Задать необходимость выделения элемента
     */
    void setHighlighted(bool _highlighted);


    /**
     * @brief Название элемента
     */
    QString name;

    /**
     * @brief Хронометраж элемента
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };

    /**
     * @brief Выделен ли элемент
     */
    bool isHighlighted = false;

    /**
     * @brief Индекс элемента из модели сценария
     */
    QModelIndex screenplayItemIndex;
};

} // namespace BusinessLayer
