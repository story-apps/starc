#include "task_bar.h"


class TaskBar::Implementation
{
public:
    static TaskBar* instance;
};

TaskBar* TaskBar::Implementation::instance = nullptr;


// ****


void TaskBar::registerTaskBar(QWidget* _parent)
{
    if (Implementation::instance == nullptr) {
        Implementation::instance = new TaskBar(_parent);
    } else {
        Implementation::instance->setParent(_parent);
    }
}

void TaskBar::addTask(const QString& _taskId)
{

}

void TaskBar::setTaskTitle(const QString& _taskId, const QString& _title)
{

}

void TaskBar::setTaskProgress(const QString& _taskId, qreal _progress)
{

}

void TaskBar::finishTask(const QString& _taskId)
{

}

TaskBar::~TaskBar() = default;

TaskBar::TaskBar(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
}

void TaskBar::paintEvent(QPaintEvent* _event)
{

}
