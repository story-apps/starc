#include "task_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/text_helper.h>
#include <utils/logging.h>

#include <QEvent>
#include <QPainter>
#include <QTimer>


class TaskBar::Implementation
{
public:
    /**
     * @brief Скорректировать геометрию панели
     */
    void correctGeometry(QWidget* _taskBar);

    /**
     * @brief Обновить таймер неопределённых задач в зависимости от их наличия
     */
    void updateIndeterminateTimer();


    /**
     * @brief Синглтон панели для отображения фоновых процессов
     */
    static TaskBar* instance;

    /**
     * @brief Цвет фона прогрессбара
     */
    QColor barColor;

    /**
     * @brief Вспомогательная структура для хранения информации о процессах
     */
    struct Task {
        QString id;
        QString title;
        qreal progress = 0.0;
        bool isIndeterminate = false;
    };

    /**
     * @brief Список выполняющихся фоновых процессов
     */
    QVector<Task> tasks;

    /**
     * @brief Анимация прогресса для задач в неопределённом состоянии
     */
    QTimer indeterminateTimer;
};

TaskBar* TaskBar::Implementation::instance = nullptr;

void TaskBar::Implementation::correctGeometry(QWidget* _taskBar)
{
    qreal width = Ui::DesignSystem::taskBar().minimumWidth();
    for (const auto& task : std::as_const(tasks)) {
        const auto taskWidth = std::min(
            Ui::DesignSystem::taskBar().margins().left()
                + TextHelper::fineTextWidthF(task.title, Ui::DesignSystem::font().caption())
                + Ui::DesignSystem::taskBar().margins().right(),
            Ui::DesignSystem::taskBar().maximumWidth());
        if (width < taskWidth) {
            width = taskWidth;
        }
    }
    _taskBar->setFixedSize(width,
                           Ui::DesignSystem::taskBar().margins().top()
                               + Ui::DesignSystem::taskBar().taskHeight() * tasks.size()
                               + Ui::DesignSystem::taskBar().margins().bottom());

    const int x = _taskBar->isLeftToRight()
        ? (_taskBar->parentWidget()->width() - _taskBar->width() - Ui::DesignSystem::layout().px4())
        : Ui::DesignSystem::layout().px4();
    const int y = _taskBar->parentWidget()->height() - _taskBar->height()
        - Ui::DesignSystem::layout().px4();
    _taskBar->move(x, y);
}

void TaskBar::Implementation::updateIndeterminateTimer()
{
    bool hasIndeteminateTasks = false;
    for (const auto& task : std::as_const(tasks)) {
        if (task.isIndeterminate) {
            hasIndeteminateTasks = true;
            break;
        }
    }

    if (hasIndeteminateTasks) {
        if (!indeterminateTimer.isActive()) {
            indeterminateTimer.start();
        }
    } else {
        indeterminateTimer.stop();
    }
}


// ****


void TaskBar::registerTaskBar(QWidget* _parent, const QColor& _backgroundColor,
                              const QColor& _textColor, const QColor& _barColor)
{
    Q_ASSERT(_parent);

    TaskBar* taskBar = Implementation::instance;
    if (taskBar == nullptr) {
        taskBar = new TaskBar(_parent);
        Implementation::instance = taskBar;
    } else {
        if (taskBar->parent() != _parent) {
            taskBar->parent()->removeEventFilter(taskBar);
            Implementation::instance->setParent(_parent);
        }
    }
    _parent->installEventFilter(taskBar);


    taskBar->setBackgroundColor(_backgroundColor);
    taskBar->setTextColor(_textColor);
    taskBar->setBarColor(_barColor);

    auto& animationTimer = taskBar->d->indeterminateTimer;
    animationTimer.setInterval(40);
    connect(&animationTimer, &QTimer::timeout, taskBar, [taskBar] {
        for (auto& task : taskBar->d->tasks) {
            if (task.isIndeterminate) {
                task.progress += 0.5;
                //
                // Тут немного хитрим, чтобы прогрессбар заполнялся чуть долше, т.к. там рисуется
                // полоса, которая проходит как бы насквозь
                //
                if (task.progress > 150.0) {
                    task.progress = 0;
                }
            }
        }
        taskBar->update();
    });
}

void TaskBar::addTask(const QString& _taskId)
{
    Q_ASSERT(Implementation::instance);

    auto& tasks = Implementation::instance->d->tasks;
    if (std::find_if(tasks.begin(), tasks.end(),
                     [_taskId](const auto& _task) { return _task.id == _taskId; })
        != tasks.end()) {
        return;
    }

    tasks.append({ _taskId, {}, {} });
    Implementation::instance->d->correctGeometry(Implementation::instance);
    Implementation::instance->raise();
    Implementation::instance->show();
}

void TaskBar::setTaskTitle(const QString& _taskId, const QString& _title)
{
    Q_ASSERT(Implementation::instance);

    auto& tasks = Implementation::instance->d->tasks;
    for (auto& task : tasks) {
        if (task.id == _taskId) {
            task.title = _title;
            break;
        }
    }

    Implementation::instance->d->correctGeometry(Implementation::instance);
    Implementation::instance->update();
}

qreal TaskBar::taskProgress(const QString& _taskId)
{
    Q_ASSERT(Implementation::instance);

    auto& tasks = Implementation::instance->d->tasks;
    for (auto& task : tasks) {
        if (task.id == _taskId) {
            return task.progress;
        }
    }

    return std::numeric_limits<qreal>::quiet_NaN();
}

void TaskBar::setTaskProgress(const QString& _taskId, qreal _progress)
{
    Q_ASSERT(Implementation::instance);

    auto& tasks = Implementation::instance->d->tasks;
    for (auto& task : tasks) {
        if (task.id == _taskId) {
            task.progress = std::min(std::max(0.0, _progress), 100.0);
            break;
        }
    }

    Implementation::instance->update();
}

void TaskBar::setIndeterminate(const QString& _taskId, bool _indeterminate)
{
    Q_ASSERT(Implementation::instance);

    auto& tasks = Implementation::instance->d->tasks;
    for (auto& task : tasks) {
        if (task.id == _taskId) {
            task.isIndeterminate = _indeterminate;
            break;
        }
    }

    Implementation::instance->d->updateIndeterminateTimer();
}

void TaskBar::finishTask(const QString& _taskId)
{
    Q_ASSERT(Implementation::instance);

    auto& tasks = Implementation::instance->d->tasks;
    const auto taskIter = std::find_if(
        tasks.begin(), tasks.end(), [_taskId](const auto& _task) { return _task.id == _taskId; });
    if (taskIter == tasks.end()) {
        Log::warning("Task with id %1 not found. Skip task finishing.", _taskId);
    } else {
        tasks.erase(taskIter);
    }

    Implementation::instance->d->correctGeometry(Implementation::instance);
    Implementation::instance->d->updateIndeterminateTimer();

    if (tasks.isEmpty()) {
        Implementation::instance->hide();
    }
}

bool TaskBar::isTaskFinished(const QString& _taskId)
{
    Q_ASSERT(Implementation::instance);

    const auto& tasks = Implementation::instance->d->tasks;
    const auto taskIter = std::find_if(
        tasks.begin(), tasks.end(), [_taskId](const auto& _task) { return _task.id == _taskId; });
    return taskIter == tasks.end();
}

void TaskBar::finishAllTasks()
{
    Q_ASSERT(Implementation::instance);

    QVector<QString> taskIds;
    for (const auto& task : std::as_const(Implementation::instance->d->tasks)) {
        taskIds.append(task.id);
    }
    for (const auto& taskId : std::as_const(taskIds)) {
        finishTask(taskId);
    }
}

void TaskBar::setBarColor(const QColor& _color)
{
    if (d->barColor == _color) {
        return;
    }

    d->barColor = _color;
    update();
}

bool TaskBar::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == parentWidget() && _event->type() == QEvent::Resize) {
        d->correctGeometry(this);
    }

    return Card::eventFilter(_watched, _event);
}

TaskBar::~TaskBar() = default;

TaskBar::TaskBar(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation)
{
    hide();
}

void TaskBar::paintEvent(QPaintEvent* _event)
{
    Card::paintEvent(_event);

    QPainter painter(this);
    painter.setFont(Ui::DesignSystem::font().caption());

    const auto progressRadius = Ui::DesignSystem::progressBar().linearTrackHeight() / 2.0;
    qreal lastTop = Ui::DesignSystem::taskBar().margins().top();
    for (const auto& task : std::as_const(d->tasks)) {
        //
        // Заголовок
        //
        painter.setPen(textColor());
        const QRectF titleRect(Ui::DesignSystem::taskBar().margins().left(), lastTop,
                               width() - Ui::DesignSystem::taskBar().margins().left()
                                   - Ui::DesignSystem::taskBar().margins().right(),
                               Ui::DesignSystem::taskBar().taskTitleHeight());
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, task.title);

        //
        // Прогресс
        //
        painter.setPen(Qt::NoPen);
        painter.setBrush(d->barColor);
        //
        // ... подложка
        //
        const QRectF progressBackgroundRect(
            titleRect.left(), titleRect.bottom() + Ui::DesignSystem::taskBar().spacing(),
            titleRect.width(), Ui::DesignSystem::progressBar().linearTrackHeight());
        painter.setOpacity(Ui::DesignSystem::progressBar().unfilledPartOpacity());
        painter.drawRoundedRect(progressBackgroundRect, progressRadius, progressRadius);
        //
        // ... заполненная часть
        //
        QRectF progressRect;
        if (!task.isIndeterminate) {
            progressRect
                = QRectF(progressBackgroundRect.left()
                             + (isLeftToRight()
                                    ? 0.0
                                    : (progressBackgroundRect.width()
                                       - progressBackgroundRect.width() * task.progress / 100.0)),
                         progressBackgroundRect.top(),
                         progressBackgroundRect.width() * task.progress / 100.0,
                         progressBackgroundRect.height());
        } else {
            const auto left
                = progressBackgroundRect.width() * std::max(0.0, task.progress - 50.0) / 100.0;
            progressRect
                = QRectF(progressBackgroundRect.left()
                             + (isLeftToRight()
                                    ? left
                                    : (progressBackgroundRect.width()
                                       - progressBackgroundRect.width() * task.progress / 100.0)),
                         progressBackgroundRect.top(),
                         progressBackgroundRect.width() * task.progress / 100.0 - left,
                         progressBackgroundRect.height());
            progressRect.setLeft(std::max(0.0, progressRect.left()));
            progressRect.setRight(std::min(progressRect.right(), progressBackgroundRect.right()));
        }
        painter.setOpacity(1.0);
        painter.drawRoundedRect(progressRect, progressRadius, progressRadius);

        lastTop += Ui::DesignSystem::taskBar().taskHeight();
    }
}

void TaskBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    d->correctGeometry(this);
}
