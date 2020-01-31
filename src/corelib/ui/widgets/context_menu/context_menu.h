#pragma once

#include <ui/widgets/card/card.h>

class QAbstractItemModel;


/**
 * @brief Виджет контекстного меню
 */
class ContextMenu : public Card
{
    Q_OBJECT

public:
    explicit ContextMenu(QWidget* _parent = nullptr);
    ~ContextMenu() override;

    /**
     * @brief Задать модель действий контекстного меню
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Отобразить контекстное меню
     */
    void showContextMenu(const QPoint& _pos);

    /**
     * @brief Скрыть контекстное меню
     */
    void hideContextMenu();

signals:
    /**
     * @brief Пользователь кликнул на пункт контекстного меню
     */
    void clicked(const QModelIndex& _index);

protected:
    /**
     * @brief Обновляем цвет фона списка действий меню при смене
     */
    void processBackgroundColorChange() override;

    /**
     * @brief Обновляем цвет текста списка действий меню при смене
     */
    void processTextColorChange() override;

    /**
     * @brief Переопределяем, чтобы скрывать контекстное меню при деактивации виджета
     */
    bool event(QEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
