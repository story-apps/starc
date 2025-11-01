#pragma once

#include "../card/card.h"


/**
 * @brief Виджет уведомления о фоновых процессах
 */
class CORE_LIBRARY_EXPORT TaskBar : public Card
{
    Q_OBJECT

public:
    /**
     * @brief Зарегистрировать панель уведомлений в заданном виджете
     */
    static void registerTaskBar(QWidget* _parent, const QColor& _backgroundColor,
                                const QColor& _textColor, const QColor& _barColor);

    /**
     * @brief Добавить процесс в список активных процессов
     */
    static void addTask(const QString& _taskId);

    /**
     * @brief Задать название процесса с заданным идентификатором
     */
    static void setTaskTitle(const QString& _taskId, const QString& _title);

    /**
     * @brief Прогресс выполнения процесса с заданным идентификатором в интервале [0.0, 100.0]
     */
    static qreal taskProgress(const QString& _taskId);
    static void setTaskProgress(const QString& _taskId, qreal _progress);

    /**
     * @brief Установить неопределённое состояние задачи
     */
    static void setIndeterminate(const QString& _taskId, bool _indeterminate);

    /**
     * @brief Завершить заданный процесс
     */
    static void finishTask(const QString& _taskId);

    /**
     * @brief Завершён ли заданный процесс
     */
    static bool isTaskFinished(const QString& _taskId);

    /**
     * @brief Завершить все процессы
     */
    static void finishAllTasks();

public:
    ~TaskBar() override;

    /**
     * @brief Задать цвет полосы прогресса
     */
    void setBarColor(const QColor& _color);

protected:
    /**
     * @brief Переопределяем, чтобы отлавливать изменение размера
     *        родительского виджета и корректировать свою позицию
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Собственная реализация рисования
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для обработки события смены дизайн-системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    explicit TaskBar(QWidget* _parent = nullptr);

    class Implementation;
    QScopedPointer<Implementation> d;
};
