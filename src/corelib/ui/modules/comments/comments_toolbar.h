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
     * @brief Режим работы панели инструментов
     */
    enum class Mode {
        AddNewComment,
        EditComment,
    };

public:
    explicit CommentsToolbar(QWidget* _parent = nullptr);
    ~CommentsToolbar() override;

    /**
     * @brief Настроить режим работы панели инструментов
     */
    void setMode(Mode _mode);

    /**
     * @brief Установить решённость текущего комментария
     */
    void setCurrentCommentIsDone(bool _isDone);

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
    void moveToolbar(const QPoint& _position);

signals:
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
     * @brief Пользователь хочет пометить текущий комментарий как решённый
     */
    void markAsDoneRequested(bool _isDone);

    /**
     * @brief Пользователь хочет удалить текущий комеентарий
     */
    void removeRequested();

protected:
    /**
     * @brief Добавим возможность анимированного отображения
     */
    void paintEvent(QPaintEvent* _event) override;

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
