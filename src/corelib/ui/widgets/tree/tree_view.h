#pragma once

#include <corelib_global.h>

#include <QTreeView>


/**
 * @brief Докрученный виджет дерева с эффектом клика на элементе
 */
class CORE_LIBRARY_EXPORT TreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit TreeView(QWidget* _parent = nullptr);
    ~TreeView() override;

    /**
     * @brief Загрузить состояние дерева
     */
    void restoreState(const QVariant& _state);

    /**
     * @brief Сохранить состояние дерева
     */
    QVariant saveState() const;

    /**
     * @brief Делаем возможность узнать необходимую ширину столбца публичным
     */
    int sizeHintForColumn(int _column) const override;

signals:
    /**
     * @brief Пользователь навёл мышь на элемент с заданным индексом
     */
    void hoverIndexChanged(const QModelIndex& _index);

protected:
    /**
     * @brief Фильтруем события клика вьюпорта, чтобы анимировать их
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Декорируем вьюпорт после нажатия клавиши
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы раскрывать элемент в который кидаются данные, если элемент свёрнут
     */
    void dropEvent(QDropEvent* _event) override;

    /**
     * @brief Переопределяем для реализации уведомлений об изменении элемента под курсором
     */
    void mouseMoveEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
