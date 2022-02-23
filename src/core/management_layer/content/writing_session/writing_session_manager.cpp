#include "writing_session_manager.h"

#include <ui/writing_session/writing_sprint_panel.h>

#include <QApplication>
#include <QDateTime>
#include <QKeyEvent>
#include <QWidget>


namespace ManagementLayer {

class WritingSessionManager::Implementation
{
public:
    explicit Implementation(QWidget* _parentWidget);

    QDateTime sessionStartedAt;
    QDateTime sessionEndedAt;


    int words = 0;
    int characters = 0;
    bool isLastCharacterSpace = true;

    Ui::WritingSprintPanel* writingSprintPanel = nullptr;
};

WritingSessionManager::Implementation::Implementation(QWidget* _parentWidget)
    : writingSprintPanel(new Ui::WritingSprintPanel(_parentWidget))
{
    writingSprintPanel->hide();
}


// ****


WritingSessionManager::WritingSessionManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(_parentWidget))
{
    connect(d->writingSprintPanel, &Ui::WritingSprintPanel::sprintStarted, this, [this] {
        d->words = 0;
        d->characters = 0;
        d->isLastCharacterSpace = true;
    });
    connect(d->writingSprintPanel, &Ui::WritingSprintPanel::sprintFinished, this,
            [this] { d->writingSprintPanel->setResult(d->words); });
}

WritingSessionManager::~WritingSessionManager() = default;

void WritingSessionManager::addKeyPressEvent(QKeyEvent* _event)
{
    //
    // Обрабатываем событие только в случае, если в виджет в фокусе можно вводить текст
    //
    if (QApplication::focusWidget()
        && !QApplication::focusWidget()->testAttribute(Qt::WA_InputMethodEnabled)) {
        return;
    }

    //
    // Пробел и переносы строк интерпретируем, как разрыв слова
    //
    if (_event->key() == Qt::Key_Space || _event->key() == Qt::Key_Tab
        || _event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return) {
        if (!d->isLastCharacterSpace) {
            d->isLastCharacterSpace = true;
            ++d->words;
        }
    }
    //
    // В остальных случаях, если есть текст, накручиваем счётчики
    //
    else if (!_event->text().isEmpty()) {
        if (d->isLastCharacterSpace) {
            d->isLastCharacterSpace = false;
        }
        d->characters += _event->text().length();
    }
}

void WritingSessionManager::showSprintPanel()
{
    d->writingSprintPanel->showPanel();
}

} // namespace ManagementLayer
