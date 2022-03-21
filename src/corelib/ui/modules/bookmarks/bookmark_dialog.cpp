#include "bookmark_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/text_field/text_field.h>

#include <QBoxLayout>


namespace Ui {

class BookmarkDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateBookmarkColor();


    DialogType dialogType = CreateNew;

    TextField* bookmarkText = nullptr;
    ColorPickerPopup* bookmarkColorPopup = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* saveButton = nullptr;
};

BookmarkDialog::Implementation::Implementation(QWidget* _parent)
    : bookmarkText(new TextField(_parent))
    , bookmarkColorPopup(new ColorPickerPopup(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , saveButton(new Button(_parent))
{
    bookmarkColorPopup->setColorCanBeDeselected(false);
    bookmarkColorPopup->setSelectedColor(Qt::yellow);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(saveButton, 0, Qt::AlignVCenter);
}

void BookmarkDialog::Implementation::updateBookmarkColor()
{
    bool isColorSet = bookmarkColorPopup->selectedColor().isValid();
    bookmarkText->setTrailingIcon(isColorSet ? u8"\U000F0765" : u8"\U000f0766");
    bookmarkText->setTrailingIconColor(isColorSet ? bookmarkColorPopup->selectedColor()
                                                  : bookmarkText->textColor());
    bookmarkText->setTrailingIconToolTip(tr("Select character color"));
}


// ****


BookmarkDialog::BookmarkDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->saveButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->bookmarkText, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);


    connect(d->bookmarkText, &TextField::textChanged, this,
            [this] { d->bookmarkText->setError({}); });
    connect(d->bookmarkText, &TextField::trailingIconPressed, this, [this] {
        d->bookmarkColorPopup->showPopup(d->bookmarkText, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->bookmarkColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this] { d->updateBookmarkColor(); });
    connect(d->cancelButton, &Button::clicked, this, &BookmarkDialog::hideDialog);
    connect(d->saveButton, &Button::clicked, this, &BookmarkDialog::savePressed);


    updateTranslations();
    designSystemChangeEvent(nullptr);


    d->updateBookmarkColor();
}

BookmarkDialog::~BookmarkDialog() = default;

void BookmarkDialog::setDialogType(DialogType _type)
{
    if (d->dialogType == _type) {
        return;
    }

    d->dialogType = _type;
    updateTranslations();
}

QString BookmarkDialog::bookmarkText() const
{
    return d->bookmarkText->text();
}

void BookmarkDialog::setBookmarkText(const QString& _text)
{
    d->bookmarkText->setText(_text);
}

QColor BookmarkDialog::bookmarkColor() const
{
    return d->bookmarkColorPopup->selectedColor();
}

void BookmarkDialog::setBookmarkColor(const QColor& _color)
{
    if (d->bookmarkColorPopup->selectedColor() == _color) {
        return;
    }

    d->bookmarkColorPopup->setSelectedColor(_color);
    d->updateBookmarkColor();
}

QWidget* BookmarkDialog::focusedWidgetAfterShow() const
{
    return d->bookmarkText;
}

QWidget* BookmarkDialog::lastFocusableWidget() const
{
    return d->saveButton;
}

void BookmarkDialog::updateTranslations()
{
    setTitle(d->dialogType == CreateNew ? tr("Create new bookmark") : tr("Edit bookmark"));
    d->bookmarkText->setLabel(tr("Bookmark text"));
    d->cancelButton->setText(tr("Cancel"));
    d->saveButton->setText(d->dialogType == CreateNew ? tr("Create") : tr("Update"));
}

void BookmarkDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->bookmarkText->setTextColor(Ui::DesignSystem::color().onBackground());
    d->bookmarkText->setBackgroundColor(Ui::DesignSystem::color().onBackground());

    d->bookmarkColorPopup->setBackgroundColor(Ui::DesignSystem::color().background());
    d->bookmarkColorPopup->setTextColor(Ui::DesignSystem::color().onBackground());

    for (auto button : {
             d->cancelButton,
             d->saveButton,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setVerticalSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px12())
            .toMargins());
}

} // namespace Ui
