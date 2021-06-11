#include "cover_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/image_cropper/image_cropper.h>
#include <ui/widgets/label/label.h>

#include <QGridLayout>


namespace Ui {

class CoverDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    ImageCropper* imageCropper = nullptr;
    Body1Label* imageCroppingNote = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* selectButton = nullptr;
};

CoverDialog::Implementation::Implementation(QWidget* _parent)
    : imageCropper(new ImageCropper(_parent))
    , imageCroppingNote(new Body1Label(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , selectButton(new Button(_parent))
{
    imageCropper->setProportion({ 3.0, 4.0 });
    imageCropper->setProportionFixed(true);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(selectButton);
}


// ****


CoverDialog::CoverDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    contentsLayout()->addWidget(d->imageCropper, 0, 0);
    contentsLayout()->addWidget(d->imageCroppingNote, 1, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 2, 0);

    connect(d->cancelButton, &Button::clicked, this, &CoverDialog::hideDialog);
    connect(d->selectButton, &Button::clicked, this, [this] {
        emit coverSelected(d->imageCropper->croppedImage());
        hideDialog();
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

void CoverDialog::setCover(const QPixmap& _cover)
{
    d->imageCropper->setImage(_cover);
}

CoverDialog::~CoverDialog() = default;

QWidget* CoverDialog::focusedWidgetAfterShow() const
{
    return d->imageCropper;
}

QWidget* CoverDialog::lastFocusableWidget() const
{
    return d->selectButton;
}

void CoverDialog::updateTranslations()
{
    d->imageCroppingNote->setText(tr("Select an area for project cover"));
    d->cancelButton->setText(tr("Cancel"));
    d->selectButton->setText(tr("Select"));
}

void CoverDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->imageCropper->setMaximumSize(500, 500);

    d->imageCropper->setBackgroundColor(Ui::DesignSystem::color().shadow());
    d->imageCropper->setTextColor(Ui::DesignSystem::color().onShadow());

    d->imageCroppingNote->setTextColor(Ui::DesignSystem::color().onBackground());
    d->imageCroppingNote->setBackgroundColor(Ui::DesignSystem::color().background());
    d->imageCroppingNote->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px24(),
                                                       Ui::DesignSystem::layout().px12(),
                                                       Ui::DesignSystem::layout().px24(), 0.0)
                                                 .toMargins());

    for (auto button : { d->cancelButton, d->selectButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px8())
            .toMargins());
}

} // namespace Ui
