#pragma once

#include <ui/modules/cards/cards_graphics_view.h>

namespace BusinessLayer {
class ProjectsModelProjectItem;
} // namespace BusinessLayer


namespace Ui {

/**
 * @brief Представление модели со списком проектов
 */
class ProjectsCardsGraphicsView : public CardsGraphicsView
{
    Q_OBJECT

public:
    explicit ProjectsCardsGraphicsView(QWidget* _parent = nullptr);

signals:
    /**
     * @brief Пользователь хочет открыть выбранный проект
     */
    void openProjectRequested(BusinessLayer::ProjectsModelProjectItem* _project);

    /**
     * @brief Запрос на отображение контекстного меню проекта
     */
    void projectContextMenuRequested(BusinessLayer::ProjectsModelProjectItem* _project);

protected:
    /**
     * @brief Исключить ли заданный индекс при рассчёте плоского индекса элемента дерева
     */
    bool excludeFromFlatIndex(const QModelIndex& _index) const override;

    /**
     * @brief Создать новую карточку на базе элемента модели
     */
    AbstractCardItem* createCardFor(const QModelIndex& _index) const override;
};


} // namespace Ui
