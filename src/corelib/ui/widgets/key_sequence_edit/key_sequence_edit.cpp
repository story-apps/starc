#include "key_sequence_edit.h"

#include <private/qkeymapper_p.h>

#include <QKeyEvent>

namespace {
constexpr int kMaxKeyCount = 4;
}


class KeySequenceEdit::Implementation
{
public:
    explicit Implementation(KeySequenceEdit* _q);
    int translateModifiers(Qt::KeyboardModifiers _state);
    void resetState();
    void finishEditing();


    KeySequenceEdit* q = nullptr;
    QKeySequence keySequence;
    QKeySequence lastKeySequence;
    int keyNum = 0;
    int key[kMaxKeyCount];
    int prevKey = -1;
    int releaseTimer = 0;
};

KeySequenceEdit::Implementation::Implementation(KeySequenceEdit* _q)
    : q(_q)
{
    key[0] = key[1] = key[2] = key[3] = 0;
}

int KeySequenceEdit::Implementation::translateModifiers(Qt::KeyboardModifiers _state)
{
    int result = 0;
    if (_state & Qt::ControlModifier) {
        result |= Qt::CTRL;
    }
    if (_state & Qt::MetaModifier) {
        result |= Qt::META;
    }
    if (_state & Qt::AltModifier) {
        result |= Qt::ALT;
    }
    return result;
}

void KeySequenceEdit::Implementation::resetState()
{
    if (releaseTimer) {
        q->killTimer(releaseTimer);
        releaseTimer = 0;
    }
    prevKey = -1;
    q->setText(keySequence.toString(QKeySequence::NativeText));
}

void KeySequenceEdit::Implementation::finishEditing()
{
    resetState();
    emit q->keySequenceChanged(keySequence);
    emit q->editingFinished();
}


// ****


KeySequenceEdit::KeySequenceEdit(QWidget* _parent)
    : TextField(_parent)
    , d(new Implementation(this))
{
    setReadOnly(true);
}

KeySequenceEdit::~KeySequenceEdit() = default;

QKeySequence KeySequenceEdit::keySequence() const
{
    return !d->keySequence.isEmpty() ? d->keySequence : d->lastKeySequence;
}

void KeySequenceEdit::setKeySequence(const QKeySequence& keySequence)
{
    d->resetState();
    if (d->keySequence == keySequence) {
        return;
    }

    d->lastKeySequence = d->keySequence;
    d->keySequence = keySequence;
    d->key[0] = d->key[1] = d->key[2] = d->key[3] = 0;
    d->keyNum = keySequence.count();
    for (int i = 0; i < d->keyNum; ++i) {
        d->key[i] = keySequence[i];
    }
    setText(keySequence.isEmpty() ? "..." : keySequence.toString(QKeySequence::NativeText));
    emit keySequenceChanged(keySequence);
}

void KeySequenceEdit::clear()
{
    setKeySequence({});
}

bool KeySequenceEdit::event(QEvent* e)
{
    switch (e->type()) {
    case QEvent::Shortcut: {
        return true;
    }

    case QEvent::ShortcutOverride: {
        e->accept();
        return true;
    }

    default: {
        break;
    }
    }

    return TextField::event(e);
}

void KeySequenceEdit::keyPressEvent(QKeyEvent* e)
{
    int nextKey = e->key();
    if (d->prevKey == -1) {
        clear();
        d->prevKey = nextKey;
    }
    setPlaceholderText({});
    if (nextKey == Qt::Key_Control || nextKey == Qt::Key_Shift || nextKey == Qt::Key_Meta
        || nextKey == Qt::Key_Alt || nextKey == Qt::Key_unknown) {
        return;
    }
    QString selectedText = this->selectedText();
    if (!selectedText.isEmpty() && selectedText == this->text()) {
        clear();
        if (nextKey == Qt::Key_Backspace) {
            return;
        }
    }
    if (d->keyNum >= kMaxKeyCount) {
        return;
    }
    if (e->modifiers() & Qt::ShiftModifier) {
        const auto possibleKeys = QKeyMapper::possibleKeys(e);
        int pkTotal = possibleKeys.count();
        if (!pkTotal) {
            return;
        }
        bool found = false;
        for (int i = 0; i < possibleKeys.size(); ++i) {
            if (possibleKeys.at(i) - nextKey == int(e->modifiers())
                || (possibleKeys.at(i) == nextKey && e->modifiers() == Qt::ShiftModifier)) {
                nextKey = possibleKeys.at(i);
                found = true;
                break;
            }
        }
        // Use as fallback
        if (!found) {
            nextKey = possibleKeys.first();
        }
    } else {
        nextKey |= d->translateModifiers(e->modifiers());
    }
    d->key[d->keyNum] = nextKey;
    d->keyNum++;
    QKeySequence key(d->key[0], d->key[1], d->key[2], d->key[3]);
    d->keySequence = key;
    QString text = key.toString(QKeySequence::NativeText);
    if (d->keyNum < kMaxKeyCount) {
        text = QString("%1, ...").arg(text);
    }
    setText(text);
    e->accept();
}

void KeySequenceEdit::keyReleaseEvent(QKeyEvent* e)
{
    if (d->prevKey == e->key()) {
        if (d->keyNum < kMaxKeyCount) {
            d->releaseTimer = startTimer(1000);
        } else {
            d->finishEditing();
        }
    }
    e->accept();
}

void KeySequenceEdit::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == d->releaseTimer) {
        d->finishEditing();
        return;
    }
    TextField::timerEvent(e);
}
