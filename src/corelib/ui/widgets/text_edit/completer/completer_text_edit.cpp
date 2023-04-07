#include "completer_text_edit.h"

#include "completer.h"

#include <ui/design_system/design_system.h>

#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QScrollBar>
#include <QTextBlock>


class CompleterTextEdit::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Использовать ли подстановщик
     */
    bool isCompleterActive = true;

    /**
     * @brief Активно ли автодополнение
     */
    bool isAutoCompleteEnabled = true;

    /**
     * @brief Подстановщик для завершения текста
     */
    Completer* completer = nullptr;
};

CompleterTextEdit::Implementation::Implementation(QWidget* _parent)
    : completer(new Completer(_parent))
{
    completer->setWidget(_parent);
    completer->setMaxVisibleItems(10);
}


// ****


CompleterTextEdit::CompleterTextEdit(QWidget* _parent)
    : SpellCheckTextEdit(_parent)
    , d(new Implementation(this))
{
    connect(d->completer, qOverload<const QModelIndex&>(&QCompleter::activated), this,
            qOverload<const QModelIndex&>(&CompleterTextEdit::applyCompletion));
}

CompleterTextEdit::~CompleterTextEdit() = default;

bool CompleterTextEdit::isCompleterActive() const
{
    return d->isCompleterActive;
}

void CompleterTextEdit::setCompleterActive(bool _use)
{
    d->isCompleterActive = _use;
}

bool CompleterTextEdit::isAutoCompleteEnabled() const
{
    return d->isAutoCompleteEnabled;
}

void CompleterTextEdit::setAutoCompleteEnabled(bool _enabled)
{
    d->isAutoCompleteEnabled = _enabled;
}

void CompleterTextEdit::setCompleterItemDelegate(QAbstractItemDelegate* _delegate)
{
    d->completer->setItemDelegate(_delegate);
}

Completer* CompleterTextEdit::completer() const
{
    return d->completer;
}

bool CompleterTextEdit::isCompleterVisible() const
{
    return d->completer->popup()->isVisible();
}

bool CompleterTextEdit::complete(QAbstractItemModel* _model, const QString& _completionPrefix)
{
    return complete(_model, _completionPrefix, _completionPrefix.length());
}

bool CompleterTextEdit::complete(QAbstractItemModel* _model, const QString& _completionPrefix,
                                 int _cursorMovement)
{
    return complete(_model, _completionPrefix, _cursorMovement, Qt::MatchStartsWith);
}

bool CompleterTextEdit::complete(QAbstractItemModel* _model, const QString& _completionPrefix,
                                 int _cursorMovement, Qt::MatchFlags _filterMode)
{
    if (!d->isCompleterActive) {
        return false;
    }

    if (_model == nullptr) {
        return false;
    }

    //
    // Настроим завершателя
    //
    d->completer->setModel(_model);
    d->completer->setModelSorting(QCompleter::UnsortedModel);
    d->completer->setCaseSensitivity(Qt::CaseInsensitive);
    d->completer->setCompletionPrefix(_completionPrefix);
    d->completer->setFilterMode(_filterMode);

    //
    // Если в модели для дополнения нет элементов, или она уже полностью дополнена
    //
    const bool hasCompletions = d->completer->completionModel()->rowCount() > 0;
    const bool alreadyComplete
        = _completionPrefix.endsWith(d->completer->currentCompletion(), Qt::CaseInsensitive);
    if (!hasCompletions || alreadyComplete) {
        //
        // ... скроем завершателя, если был показан
        //
        closeCompleter();
        return false;
    }

    //
    // Отобразим завершателя
    //
    d->completer->popup()->setCurrentIndex(d->completer->completionModel()->index(0, 0));
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, _cursorMovement);
    QRect rect = cursorRect(cursor);
    rect.moveLeft(rect.left()
                  + (verticalScrollBar()->isVisible() ? verticalScrollBar()->width() : 0)
                  + (viewportMargins().isNull() ? 0 : viewportMargins().left()));
    const int heightDelta
        = cursor.block().layout()->boundingRect().height() + Ui::DesignSystem::layout().px16();
    rect.moveTop(rect.top() + heightDelta);
    rect.setWidth(Ui::DesignSystem::treeOneLineItem().margins().left()
                  + Ui::DesignSystem::treeOneLineItem().spacing()
                  + d->completer->popup()->sizeHintForColumn(0)
                  + Ui::DesignSystem::treeOneLineItem().margins().right());
    rect.setHeight(heightDelta);
    d->completer->showCompleter(rect);
    emit popupShown();
    return true;
}

void CompleterTextEdit::applyCompletion()
{
    if (!isCompleterVisible()) {
        return;
    }

    //
    // Получим выбранный из списка дополнений элемент
    //
    const QModelIndex currentIndex = d->completer->popup()->currentIndex();
    applyCompletion(currentIndex);

    closeCompleter();
}

void CompleterTextEdit::applyCompletion(const QModelIndex& _completionIndex)
{
    auto completionModel = qobject_cast<QAbstractProxyModel*>(d->completer->completionModel());
    const auto sourceCompletionIndex = completionModel->mapToSource(_completionIndex);

    //
    // Вставим дополнение в текст
    //
    const auto completionPrefix = d->completer->completionPrefix();
    const auto completion = d->completer->popup()->model()->data(_completionIndex).toString();

    //
    // Определяем позицию вставки текста дополнения и помещаем туда курсор редактора
    //
    QTextCursor cursor = textCursor();
    if (!completionPrefix.isEmpty()) {
        cursor.movePosition(QTextCursor::EndOfBlock);
        while (!cursor.atBlockStart()
               && cursor.selectedText().compare(completionPrefix, Qt::CaseInsensitive) != 0) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            if (!completionPrefix.endsWith(cursor.selectedText(), Qt::CaseInsensitive)) {
                cursor.clearSelection();
            }
        }
    }

    //
    // Собственно дополняем текст
    //
    cursor.insertText(completion);

    //
    // Уведомим об успешном дополнении
    //
    emit completed(sourceCompletionIndex);
}

void CompleterTextEdit::closeCompleter()
{
    d->completer->closeCompleter();
}
