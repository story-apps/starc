#include "simple_text_search_toolbar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/text_field/text_field.h>

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QStringListModel>
#include <QVariantAnimation>


namespace Ui {

class SimpleTextSearchToolbar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QAction* closeAction = nullptr;

    QAction* searchTextAction = nullptr;
    TextField* searchText = nullptr;
    QString lastSearchText; // чтобы защититься от флуда при активации окна
    QAction* goToNextAction = nullptr;
    QAction* goToPreviousAction = nullptr;
    QAction* matchCaseAction = nullptr;

    QAction* replaceTextAction = nullptr;
    TextField* replaceText = nullptr;
    QAction* replaceAction = nullptr;
    Button* replace = nullptr;
    QAction* replaceAllAction = nullptr;
    Button* replaceAll = nullptr;
};

SimpleTextSearchToolbar::Implementation::Implementation(QWidget* _parent)
    : closeAction(new QAction)
    , searchTextAction(new QAction)
    , searchText(new TextField(_parent))
    , goToNextAction(new QAction)
    , goToPreviousAction(new QAction)
    , matchCaseAction(new QAction)
    , replaceTextAction(new QAction)
    , replaceText(new TextField(_parent))
    , replaceAction(new QAction)
    , replace(new Button(_parent))
    , replaceAllAction(new QAction)
    , replaceAll(new Button(_parent))
{
    searchText->setCapitalizeWords(false);
    searchText->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    searchText->setUnderlineDecorationVisible(false);
    searchText->setWordWrapMode(QTextOption::NoWrap);

    replaceText->setCapitalizeWords(false);
    replaceText->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    replaceText->setUnderlineDecorationVisible(false);
    replaceText->setWordWrapMode(QTextOption::NoWrap);

    replace->setFocusPolicy(Qt::NoFocus);
    replaceAll->setFocusPolicy(Qt::NoFocus);
}


// ****


SimpleTextSearchToolbar::SimpleTextSearchToolbar(QWidget* _parent)
    : FloatingToolBar(_parent)
    , d(new Implementation(this))
{
    setCurtain(true);

    _parent->installEventFilter(this);
    d->searchText->installEventFilter(this);
    d->replaceText->installEventFilter(this);
    setFocusProxy(d->searchText);

    d->closeAction->setIconText(u8"\U000F004D");
    d->closeAction->setShortcut(QKeySequence::Find);
    addAction(d->closeAction);
    connect(d->closeAction, &QAction::triggered, this, [this] {
        if (d->searchText->hasFocus()) {
            emit closePressed();
        } else {
            d->searchText->setFocus();
            d->searchText->selectAll();
        }
    });
    //
    addAction(d->searchTextAction);
    connect(d->searchText, &TextField::textChanged, this, [this] {
        if (d->lastSearchText == d->searchText->text()) {
            return;
        }

        d->lastSearchText = d->searchText->text();
        emit findTextRequested();
    });
    //
    d->goToPreviousAction->setIconText(u8"\U000f0143");
    d->goToPreviousAction->setShortcut(QKeySequence::FindPrevious);
    addAction(d->goToPreviousAction);
    connect(d->goToPreviousAction, &QAction::triggered, this,
            &SimpleTextSearchToolbar::findPreviousRequested);
    d->goToNextAction->setIconText(u8"\U000f0140");
    d->goToNextAction->setShortcut(QKeySequence::FindNext);
    addAction(d->goToNextAction);
    connect(d->goToNextAction, &QAction::triggered, this,
            &SimpleTextSearchToolbar::findNextRequested);
    d->matchCaseAction->setIconText(u8"\U000f0b34");
    d->matchCaseAction->setCheckable(true);
    addAction(d->matchCaseAction);
    connect(d->matchCaseAction, &QAction::toggled, this, [this] {
        d->matchCaseAction->setToolTip(d->matchCaseAction->isChecked()
                                           ? tr("Search without case sensitive")
                                           : tr("Search with case sensitive"));
    });
    connect(d->matchCaseAction, &QAction::toggled, this,
            &SimpleTextSearchToolbar::findTextRequested);

    addAction(d->replaceTextAction);
    addAction(d->replaceAction);
    connect(d->replace, &Button::clicked, this, &SimpleTextSearchToolbar::replaceOnePressed);
    addAction(d->replaceAllAction);
    connect(d->replaceAll, &Button::clicked, this, &SimpleTextSearchToolbar::replaceAllPressed);
}

SimpleTextSearchToolbar::~SimpleTextSearchToolbar() = default;

void SimpleTextSearchToolbar::setReadOnly(bool _readOnly)
{
    const auto enabled = !_readOnly;
    d->replaceText->setEnabled(enabled);
    d->replace->setEnabled(enabled);
    d->replaceAll->setEnabled(enabled);
}

void SimpleTextSearchToolbar::refocus()
{
    d->searchText->setFocus();
}

QString SimpleTextSearchToolbar::searchText() const
{
    return d->lastSearchText;
}

bool SimpleTextSearchToolbar::isCaseSensitive() const
{
    return d->matchCaseAction->isChecked();
}

QString SimpleTextSearchToolbar::replaceText() const
{
    return d->replaceText->text();
}

bool SimpleTextSearchToolbar::eventFilter(QObject* _watched, QEvent* _event)
{
    switch (_event->type()) {
    case QEvent::Resize: {
        if (_watched == parent()) {
            designSystemChangeEvent(nullptr);
        }
        break;
    }

    case QEvent::KeyPress: {
        if (_watched == d->searchText) {
            const auto keyEvent = static_cast<QKeyEvent*>(_event);
            if ((keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
                && !d->searchText->text().isEmpty()) {
                emit findTextRequested();
            }
        }
        break;
    }

    case QEvent::KeyRelease: {
        if (_watched == d->searchText) {
            const auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Escape) {
                emit focusTextRequested();
            }
        }
        break;
    }

    default:
        break;
    }

    return FloatingToolBar::eventFilter(_watched, _event);
}

void SimpleTextSearchToolbar::processBackgroundColorChange()
{
    d->searchText->setBackgroundColor(backgroundColor());
    d->replaceText->setBackgroundColor(backgroundColor());
    d->replace->setBackgroundColor(Ui::DesignSystem::color().accent());
    d->replaceAll->setBackgroundColor(Ui::DesignSystem::color().accent());
}

void SimpleTextSearchToolbar::processTextColorChange()
{
    d->searchText->setTextColor(textColor());
    d->replaceText->setTextColor(textColor());
    d->replace->setTextColor(Ui::DesignSystem::color().accent());
    d->replaceAll->setTextColor(Ui::DesignSystem::color().accent());
}

void SimpleTextSearchToolbar::updateTranslations()
{
    d->closeAction->setToolTip(
        tr("Exit from search")
        + QString(" (%1)").arg(
            QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText)));
    d->searchText->setLabel(tr("Search"));
    d->searchText->setPlaceholderText(tr("Enter search phrase here"));
    d->goToNextAction->setToolTip(
        tr("Go to the next search result")
        + QString(" (%1)").arg(
            QKeySequence(QKeySequence::FindNext).toString(QKeySequence::NativeText)));
    d->goToPreviousAction->setToolTip(
        tr("Go to the previous search result")
        + QString(" (%1)").arg(
            QKeySequence(QKeySequence::FindPrevious).toString(QKeySequence::NativeText)));
    d->matchCaseAction->setToolTip(d->matchCaseAction->isChecked()
                                       ? tr("Search without case sensitive")
                                       : tr("Search with case sensitive"));

    d->replaceText->setLabel(tr("Replace with"));
    d->replaceText->setPlaceholderText(tr("Enter phrase to replace"));
    d->replace->setText(tr("Replace"));
    d->replaceAll->setText(tr("All"));
}

void SimpleTextSearchToolbar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    FloatingToolBar::designSystemChangeEvent(_event);

    //
    // Рассчитываем размер полей поиска и замены
    //
    d->replace->resize(d->replace->sizeHint());
    const auto replaceActionWidth
        = d->replace->sizeHint().width() - Ui::DesignSystem::floatingToolBar().spacing();
    d->replaceAll->resize(d->replaceAll->sizeHint());
    const auto replaceAllActionWidth
        = d->replaceAll->sizeHint().width() - Ui::DesignSystem::floatingToolBar().spacing();
    auto textFieldWidth = parentWidget()->width() * 0.8;
    textFieldWidth -= (Ui::DesignSystem::floatingToolBar().iconSize().width()
                       + Ui::DesignSystem::floatingToolBar().spacing())
            * 4 // 4 обычных кнопки
        + replaceActionWidth // кнопка единичной замены
        + replaceAllActionWidth; // кнопка полной замены
    textFieldWidth /= 2; // делим поровну
    if (textFieldWidth < 0) {
        return;
    }

    //
    // Расставляем элементы в панели
    //
    setActionCustomWidth(d->searchTextAction, textFieldWidth);
    d->searchText->setFixedWidth(textFieldWidth);
    const auto searchLeft = isLeftToRight()
        ? (Ui::DesignSystem::floatingToolBar().shadowMargins().left()
           + Ui::DesignSystem::floatingToolBar().iconSize().width()
           + Ui::DesignSystem::floatingToolBar().spacing())
        : (width() - Ui::DesignSystem::floatingToolBar().shadowMargins().right()
           - Ui::DesignSystem::floatingToolBar().iconSize().width()
           - Ui::DesignSystem::floatingToolBar().spacing())
            - textFieldWidth;
    d->searchText->move(searchLeft, Ui::DesignSystem::floatingToolBar().shadowMargins().top());

    const auto replaceLeft = isLeftToRight()
        ? (searchLeft + d->searchText->width() + Ui::DesignSystem::floatingToolBar().spacing()
           + (Ui::DesignSystem::floatingToolBar().iconSize().width()
              + Ui::DesignSystem::floatingToolBar().spacing())
               * 3)
        : (searchLeft - Ui::DesignSystem::floatingToolBar().spacing()
           - (Ui::DesignSystem::floatingToolBar().iconSize().width()
              + Ui::DesignSystem::floatingToolBar().spacing())
               * 3
           - textFieldWidth);

    setActionCustomWidth(d->replaceTextAction, textFieldWidth);
    d->replaceText->setFixedWidth(textFieldWidth);
    d->replaceText->move(replaceLeft, Ui::DesignSystem::floatingToolBar().shadowMargins().top());
    //
    setActionCustomWidth(d->replaceAction, replaceActionWidth);
    d->replace->move(
        isLeftToRight()
            ? (d->replaceText->geometry().right() + Ui::DesignSystem::floatingToolBar().spacing())
            : (d->replaceText->geometry().left() - Ui::DesignSystem::floatingToolBar().spacing()
               - replaceActionWidth),
        Ui::DesignSystem::floatingToolBar().shadowMargins().top()
            + Ui::DesignSystem::layout().px8());
    setActionCustomWidth(d->replaceAllAction, replaceAllActionWidth);
    d->replaceAll->move(isLeftToRight() ? d->replace->geometry().right()
                                        : (d->replace->geometry().left()
                                           - Ui::DesignSystem::floatingToolBar().spacing()
                                           - replaceAllActionWidth),
                        d->replace->geometry().top());

    resize(sizeHint());
}

} // namespace Ui
