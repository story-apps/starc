#pragma once

#include "card_popup.h"

class QAbstractItemModel;


/**
 * @brief Попап для реализации выпадающих списков элементов
 */
class CORE_LIBRARY_EXPORT CardPopupWithTree : public CardPopup
{
    Q_OBJECT

public:
    explicit CardPopupWithTree(QWidget* _parent = nullptr);
    ~CardPopupWithTree() override;

    /**
     * @brief Модель элементов для отображения в попапе
     */
    QAbstractItemModel* contentModel() const;
    void setContentModel(QAbstractItemModel* _model);

    /**
     * @brief Текущий индекс модели
     */
    QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex& _index);

    /**
     * @brief Получить ширину заданной колонки
     */
    int sizeHintForColumn(int _column) const;

    /**
     * @brief Показать попап в заданном положении с заданной шириной
     */
    void showPopup(const QPoint& _position, int _parentHeight, int _width, int _showMaxItems = 5);

signals:
    /**
     * @brief Изменился текущий индекс
     */
    void currentIndexChanged(const QModelIndex& _index);

protected:
    /**
     * @brief Настраиваем внешний вид дерева элементов самого попапа
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
