#include "task_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/logging.h>

#include <QEvent>
#include <QPainter>


class TaskBar::Implementation
{
public:
    /**
     * @brief Скорректировать геометрию панели
     */
    void correctGeometry(QWidget* _taskBar);


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
    };

    /**
     * @brief Список выполняющихся фоновых процессов
     */
    QVector<Task> tasks;
};

TaskBar* TaskBar::Implementation::instance = nullptr;

void TaskBar::Implementation::correctGeometry(QWidget* _taskBar)
{
    _taskBar->setFixedSize(Ui::DesignSystem::taskBar().width(),
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


// ****


void TaskBar::registerTaskBar(QWidget* _parent, const QColor& _backgroundColor,
                              const QColor& _textColor, const QColor& _barColor)
{
    Q_ASSERT(_parent);

    if (Implementation::instance == nullptr) {
        Implementation::instance = new TaskBar(_parent);
    } else {
        Implementation::instance->setParent(_parent);
    }

    _parent->installEventFilter(Implementation::instance);

    Implementation::instance->setBackgroundColor(_backgroundColor);
    Implementation::instance->setTextColor(_textColor);
    Implementation::instance->setBarColor(_barColor);
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

    Implementation::instance->update();
}

void TaskBar::setTaskProgress(const QString& _taskId, qreal _progress)
{
    Q_ASSERT(Implementation::instance);

    auto& tasks = Implementation::instance->d->tasks;
    for (auto& task : tasks) {
        if (task.id == _taskId) {
            task.progress = _progress;
            break;
        }
    }

    Implementation::instance->update();
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

    if (tasks.isEmpty()) {
        Implementation::instance->hide();
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
        const QRectF progressRect(
            progressBackgroundRect.left()
                + (isLeftToRight() ? 0.0
                                   : (progressBackgroundRect.width()
                                      - progressBackgroundRect.width() * task.progress / 100.0)),
            progressBackgroundRect.top(), progressBackgroundRect.width() * task.progress / 100.0,
            progressBackgroundRect.height());
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
