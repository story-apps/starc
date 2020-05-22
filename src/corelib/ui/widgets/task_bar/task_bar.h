#pragma once

#include "../widget/widget.h"


/**
 * @brief Виджет уведомления о фоновых процессах
 */
class CORE_LIBRARY_EXPORT TaskBar : public Widget
{
    Q_OBJECT

public:
    /**
     * @brief Зарегистрировать панель уведомлений в заданном виджете
     */
    static void registerTaskBar(QWidget* _parent);

    /**
     * @brief Добавить процесс в список активных процессов
     */
    static void addTask(const QString& _taskId);

    /**
     * @brief Задать название процесса с заданным идентификатором
     */
    static void setTaskTitle(const QString& _taskId, const QString& _title);

    /**
     * @brief Задать прогресс выполнения процесса с заданным идентификатором в интервале [0.0, 100.0]
     */
    static void setTaskProgress(const QString& _taskId, qreal _progress);

    /**
     * @brief Завершить заданный процесс
     */
    static void finishTask(const QString& _taskId);

public:
    ~TaskBar() override;

protected:
    /**
     * @brief Собственная реализация рисования
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    explicit TaskBar(QWidget* _parent = nullptr);

    class Implementation;
    QScopedPointer<Implementation> d;
};
