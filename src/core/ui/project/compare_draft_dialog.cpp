#include "compare_draft_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/ui_helper.h>

#include <QBoxLayout>
#include <QStringListModel>


namespace Ui {

class CompareDraftDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Body1Label* draftHint = nullptr;
    ComboBox* lhsDraft = nullptr;
    QStringListModel* lhsDraftModel = nullptr;
    ComboBox* rhsDraft = nullptr;
    QStringListModel* rhsDraftModel = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* compareButton = nullptr;
};

CompareDraftDialog::Implementation::Implementation(QWidget* _parent)
    : draftHint(new Body1Label(_parent))
    , lhsDraft(new ComboBox(_parent))
    , lhsDraftModel(new QStringListModel(lhsDraft))
    , rhsDraft(new ComboBox(_parent))
    , rhsDraftModel(new QStringListModel(lhsDraft))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , compareButton(new Button(_parent))
{
    lhsDraft->setModel(lhsDraftModel);
    rhsDraft->setModel(rhsDraftModel);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(compareButton);
}


// ****


CompareDraftDialog::CompareDraftDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->compareButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->draftHint, row++, 0);
    contentsLayout()->addWidget(d->lhsDraft, row++, 0);
    contentsLayout()->addWidget(d->rhsDraft, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->compareButton, &Button::clicked, this, [this] {
        emit comparePressed(d->lhsDraft->currentIndex().row(), d->rhsDraft->currentIndex().row());
    });
    connect(d->cancelButton, &Button::clicked, this, &CompareDraftDialog::hideDialog);
}

CompareDraftDialog::~CompareDraftDialog() = default;

void CompareDraftDialog::setDrafts(const QString& _lhsName, const QStringList& _lhsDrafts,
                                   int _selectLhsDraftIndex, const QString& _rhsName,
                                   const QStringList& _rhsDrafts, int _selectRhsDraftIndex)
{
    d->lhsDraft->setLabel(_lhsName);
    d->lhsDraft->setEnabled(_lhsDrafts.size() > 1);
    d->lhsDraftModel->setStringList(_lhsDrafts);
    d->lhsDraft->setCurrentText(_lhsDrafts.at(_selectLhsDraftIndex));

    d->rhsDraft->setLabel(_rhsName);
    d->rhsDraft->setEnabled(_rhsDrafts.size() > 1);
    d->rhsDraftModel->setStringList(_rhsDrafts);
    d->rhsDraft->setCurrentText(_rhsDrafts.at(_selectRhsDraftIndex));
}

QWidget* CompareDraftDialog::focusedWidgetAfterShow() const
{
    return d->lhsDraft;
}

QWidget* CompareDraftDialog::lastFocusableWidget() const
{
    return d->compareButton;
}

void CompareDraftDialog::updateTranslations()
{
    setTitle(tr("Compare documents"));

    d->draftHint->setText(tr("Select drafts to compare."));
    d->cancelButton->setText(tr("Cancel"));
    d->compareButton->setText(tr("Compare"));
}

void CompareDraftDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->draftHint->setContentsMargins(DesignSystem::layout().px24(), 0,
                                     DesignSystem::layout().px16(), DesignSystem::layout().px24());
    d->draftHint->setBackgroundColor(DesignSystem::color().background());
    d->draftHint->setTextColor(DesignSystem::color().onBackground());
    for (auto comboBox : {
             d->lhsDraft,
             d->rhsDraft,
         }) {
        comboBox->setTextColor(DesignSystem::color().onBackground());
        comboBox->setBackgroundColor(DesignSystem::color().onBackground());
        comboBox->setPopupBackgroundColor(DesignSystem::color().background());
        comboBox->setCustomMargins({ DesignSystem::layout().px24(), DesignSystem::layout().px12(),
                                     DesignSystem::layout().px24(), 0.0 });
    }

    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->compareButton, UiHelper::DialogAccept);

    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
