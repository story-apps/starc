#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
struct CursorInfo;
}


namespace Ui {

/**
 * @brief Панель со списком активных соавторов
 */
class CollaboratorsToolBar : public Widget
{
    Q_OBJECT

public:
    explicit CollaboratorsToolBar(QWidget* _parent = nullptr);
    ~CollaboratorsToolBar() override;

    /**
     * @brief Задать список соавторов
     */
    void setCollaborators(const QVector<Domain::CursorInfo>& _collaborators);

    /**
     * @brief Определим идеальный размер в зависимости от количества соавторов
     */
    QSize sizeHint() const override;

signals:
    /**
     * @brief Пользователь кликнул на соавтора
     */
    void collaboratorClicked(const QString& _cursorId);

protected:
    /**
     * @brief Переопределяем для корректировки положения тулбара при изменении рамера родителя
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Определим хочет ли пользователь посмотреть где сидит какой соавтор
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Корректируем размер при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
} // namespace Ui
