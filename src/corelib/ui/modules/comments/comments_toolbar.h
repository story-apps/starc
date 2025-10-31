#pragma once

#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>


namespace Ui {

/**
 * @brief Панель инструментов рецензирования
 */
class CORE_LIBRARY_EXPORT CommentsToolbar : public FloatingToolBar
{
    Q_OBJECT

public:
    /**
     * @brief Режим работы рецензирования (комменты, изменения, ревизии)
     */
    enum class CommentsType {
        Review,
        Changes,
        Revision,
    };

    /**
     * @brief Режим работы панели инструментов
     */
    enum class Mode {
        AddReview,
        EditReview,
    };

public:
    explicit CommentsToolbar(QWidget* _parent = nullptr);
    ~CommentsToolbar() override;

    /**
     * @brief Текущий режим работы панели
     */
    CommentsType commentsType() const;

    /**
     * @brief Текущий цвет режима
     */
    QColor color() const;

    /**
     * @brief Настроить режим работы панели инструментов
     */
    void setMode(Mode _mode);

    /**
     * @brief Задать возможность добавления редакторских заметок
     */
    void setAddingAvailable(bool _available);

    /**
     * @brief Установить состояние текущего комментария
     */
    void setCurrentCommentState(bool _isDone, bool _isChange, bool _isRevision);

    /**
     * @brief Отобразить тулбар
     */
    void showToolbar();

    /**
     * @brief Скрыть тулбар
     */
    void hideToolbar();

    /**
     * @brief Сместить тулбар в заданную точку
     */
    void moveToolbar(const QPoint& _position, bool _force = false);

signals:
    /**
     * @brief Изменился режим работы рецензирования
     */
    void commentsTypeChanged(Ui::CommentsToolbar::CommentsType _type);

    /**
     * @brief Изменился цвет заметки
     */
    void colorChanged(const QColor& _color);

    /**
     * @brief Пользователь хочет изменить цвет текста
     */
    void textColorChangeRequested(const QColor& _color);

    /**
     * @brief Пользователь хочет изменить цвет фона текста
     */
    void textBackgoundColorChangeRequested(const QColor& _color);

    /**
     * @brief Пользователь хочет добавить комментарий с заданным цветом
     */
    void commentAddRequested(const QColor& _color);

    /**
     * @brief Пользователь хочет добавить пометку добавленного текста
     */
    void changeAdditionAddRequested(const QColor& _color);

    /**
     * @brief Пользователь хочет добавить пометку удалённого текста
     */
    void changeRemovalAddRequested(const QColor& _color);

    /**
     * @brief Пользователь хочет добавить пометку ревизии
     */
    void revisionMarkAddRequested(const QColor& _color);

    /**
     * @brief Пользователь хочет пометить текущий комментарий как решённый
     */
    void markAsDoneRequested(bool _isDone);

    /**
     * @brief Пользователь хочет удалить текущий комеентарий
     */
    void removeRequested();

protected:
    /**
     * @brief Скрыть панель типов при потере фокуса и нажатии эксейп в моменте, когда она открыта
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Добавим возможность анимированного отображения
     */
    bool paintEventPreprocess(QPainter& _painter) override;

    /**
     * @brief При смещении панели инструментов, сдвигаем панель режимов, если она была открыта
     */
    void moveEvent(QMoveEvent* _event) override;

    /**
     * @brief Скрываем попап, когда фокус ушёл из виджета
     */
    void focusOutEvent(QFocusEvent* _event) override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
