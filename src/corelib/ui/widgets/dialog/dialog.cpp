#include "dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/ui_helper.h>

#include <QHBoxLayout>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {
const char* kButtonRole = "button-role";
}


class Dialog::Implementation
{
public:
    Implementation(QWidget* _parent);
    void applyDiffHighlighting();

    Body1Label* supportingText;
    QPlainTextEdit* supportingTextEdit = nullptr;
    bool diffHighlightingEnabled = false;
    QHBoxLayout* buttonsSideBySideLayout = nullptr;
    QVBoxLayout* buttonsStackedLayout = nullptr;
    QVector<Button*> buttons;
};

Dialog::Implementation::Implementation(QWidget* _parent)
    : supportingText(new Body1Label(_parent))
    , buttonsSideBySideLayout(new QHBoxLayout)
    , buttonsStackedLayout(new QVBoxLayout)
{
    buttonsSideBySideLayout->setContentsMargins({});
    buttonsSideBySideLayout->setSpacing(0);
    buttonsSideBySideLayout->addStretch();

    buttonsStackedLayout->setContentsMargins({});
    buttonsStackedLayout->setSpacing(0);
}

// ****

Dialog::Dialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    contentsLayout()->addWidget(d->supportingText, 0, 0);
    contentsLayout()->addLayout(d->buttonsSideBySideLayout, 1, 0);
    contentsLayout()->addLayout(d->buttonsStackedLayout, 2, 0, Qt::AlignRight);
}

Dialog::~Dialog() = default;

void Dialog::Implementation::applyDiffHighlighting()
{
    if (supportingTextEdit == nullptr || !diffHighlightingEnabled) {
        return;
    }

    auto diffFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    diffFont.setPointSizeF(Ui::DesignSystem::font().body1().pointSizeF());
    supportingTextEdit->setFont(diffFont);

    QColor additionColor = Ui::DesignSystem::color().success();
    QColor removalColor = Ui::DesignSystem::color().error();
    additionColor.setAlpha(45);
    removalColor.setAlpha(45);
    QList<QTextEdit::ExtraSelection> selections;
    for (auto block = supportingTextEdit->document()->begin(); block.isValid();
         block = block.next()) {
        const auto text = block.text();
        if (!text.startsWith("+ ") && !text.startsWith(QString::fromUtf8("− "))
            && !text.startsWith("- ")) {
            continue;
        }
        QTextEdit::ExtraSelection selection;
        selection.cursor = QTextCursor(block);
        selection.cursor.select(QTextCursor::BlockUnderCursor);
        selection.format.setBackground(text.startsWith("+ ") ? additionColor : removalColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selections.append(selection);
    }
    supportingTextEdit->setExtraSelections(selections);
}

void Dialog::enableSupportingTextScrolling()
{
    if (d->supportingTextEdit != nullptr) {
        return;
    }

    contentsLayout()->removeWidget(d->supportingText);
    d->supportingText->hide();

    d->supportingTextEdit = new QPlainTextEdit(this);
    d->supportingTextEdit->setReadOnly(true);
    d->supportingTextEdit->setUndoRedoEnabled(false);
    d->supportingTextEdit->setFrameShape(QFrame::NoFrame);
    d->supportingTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    d->supportingTextEdit->setFocusPolicy(Qt::StrongFocus);
    d->supportingTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->supportingTextEdit->setPlainText(d->supportingText->text());
    UiHelper::setupScrolling(d->supportingTextEdit);
    contentsLayout()->addWidget(d->supportingTextEdit, 0, 0);

    designSystemChangeEvent(nullptr);
}

void Dialog::enableDiffHighlighting()
{
    d->diffHighlightingEnabled = true;
    d->applyDiffHighlighting();
}

void Dialog::showDialog(const QString& _title, const QString& _supportingText,
                        const QVector<ButtonInfo>& _buttons, bool _placeButtonsSideBySide)
{
    Q_ASSERT(!_buttons.isEmpty());

    setTitle(_title);
    d->supportingText->setText(_supportingText);
    if (d->supportingTextEdit != nullptr) {
        d->supportingTextEdit->setPlainText(_supportingText);
        d->supportingTextEdit->moveCursor(QTextCursor::Start);
        d->applyDiffHighlighting();
    }
    if (_placeButtonsSideBySide) {
        contentsLayout()->removeItem(d->buttonsStackedLayout);
        d->buttonsStackedLayout->deleteLater();
    } else {
        contentsLayout()->removeItem(d->buttonsSideBySideLayout);
        d->buttonsSideBySideLayout->deleteLater();
    }
    for (const auto& buttonInfo : _buttons) {
        auto button = new Button(this);
        button->setText(buttonInfo.text);
        button->setProperty(kButtonRole, buttonInfo.type);
        d->buttons.append(button);
        if (_placeButtonsSideBySide) {
            d->buttonsSideBySideLayout->addWidget(button);
        } else {
            d->buttonsStackedLayout->addWidget(button, 0, Qt::AlignRight);
        }
        connect(button, &Button::clicked, this, [this, buttonInfo] { emit finished(buttonInfo); });
        if (buttonInfo.type == Dialog::AcceptButton
            || buttonInfo.type == Dialog::AcceptCriticalButton) {
            setAcceptButton(button);
        } else if (buttonInfo.type == Dialog::RejectButton) {
            setRejectButton(button);
        }
    }

    //
    // Настраиваем стили добавленных кнопок
    //
    designSystemChangeEvent(nullptr);

    AbstractDialog::showDialog();
}

QWidget* Dialog::focusedWidgetAfterShow() const
{
    return d->supportingTextEdit != nullptr
        ? static_cast<QWidget*>(d->supportingTextEdit)
        : static_cast<QWidget*>(d->supportingText);
}

QWidget* Dialog::lastFocusableWidget() const
{
    return d->buttons.last();
}

void Dialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->supportingText->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(),
                  title().isEmpty() ? Ui::DesignSystem::layout().px24() : 0,
                  Ui::DesignSystem::layout().px24(), 0)
            .toMargins());
    d->supportingText->setBackgroundColor(Qt::transparent);
    d->supportingText->setTextColor(Ui::DesignSystem::color().onBackground());

    if (d->supportingTextEdit != nullptr) {
        d->supportingTextEdit->setFont(Ui::DesignSystem::font().body1());
        auto palette = d->supportingTextEdit->palette();
        palette.setColor(QPalette::Base, Qt::transparent);
        palette.setColor(QPalette::Text, Ui::DesignSystem::color().onBackground());
        palette.setColor(QPalette::Highlight, Ui::DesignSystem::color().accent());
        palette.setColor(QPalette::HighlightedText, Ui::DesignSystem::color().onAccent());
        d->supportingTextEdit->setPalette(palette);
        d->supportingTextEdit->document()->setDocumentMargin(
            Ui::DesignSystem::layout().px24());
        d->applyDiffHighlighting();
    }

    for (auto button : std::as_const(d->buttons)) {
        UiHelper::ButtonRole buttonRole = UiHelper::DialogDefault;
        switch (button->property(kButtonRole).toInt()) {
        case AcceptButton: {
            buttonRole = UiHelper::DialogAccept;
            break;
        }
        case AcceptCriticalButton: {
            buttonRole = UiHelper::DialogCritical;
            break;
        }
        default: {
            break;
        }
        }
        UiHelper::initColorsFor(button, buttonRole);
    }

    d->buttonsSideBySideLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());

    d->buttonsStackedLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}
