#include "import_dialog.h"

#include <business_layer/import/import_options.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/label/label.h>

#include <QEvent>
#include <QFileInfo>
#include <QGridLayout>


namespace Ui
{

class ImportDialog::Implementation
{
public:
    Implementation(const QString& _importFilePath, QWidget* _parent);


    QString importFilePath;

    OverlineLabel* documentsTitle = nullptr;
    CheckBox* importCharacters = nullptr;
    CheckBox* importLocations = nullptr;
    OverlineLabel* screenplayTitle = nullptr;
    CheckBox* importScreenplay = nullptr;
    CheckBox* removeSceneNumbers = nullptr;
    CheckBox* keepReviewMarks = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* importButton = nullptr;
    Button* cancelButton = nullptr;
};


ImportDialog::Implementation::Implementation(const QString& _importFilePath, QWidget* _parent)
    : importFilePath(_importFilePath),
      documentsTitle(new OverlineLabel(_parent)),
      importCharacters(new CheckBox(_parent)),
      importLocations(new CheckBox(_parent)),
      screenplayTitle(new OverlineLabel(_parent)),
      importScreenplay(new CheckBox(_parent)),
      removeSceneNumbers(new CheckBox(_parent)),
      keepReviewMarks(new CheckBox(_parent)),
      buttonsLayout(new QHBoxLayout),
      importButton(new Button(_parent)),
      cancelButton(new Button(_parent))
{
    for (auto checkBox : { importCharacters, importLocations,
                           importScreenplay, removeSceneNumbers, keepReviewMarks }) {
        checkBox->setChecked(true);
    }

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(importButton);
}


// ****


ImportDialog::ImportDialog(const QString& _importFilePath, QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(_importFilePath, this))
{
    d->importButton->installEventFilter(this);

    setAcceptButton(d->importButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->addWidget(d->documentsTitle, 0, 0);
    contentsLayout()->addWidget(d->importCharacters, 1, 0);
    contentsLayout()->addWidget(d->importLocations, 2, 0);
    contentsLayout()->addWidget(d->screenplayTitle, 3, 0);
    contentsLayout()->addWidget(d->importScreenplay, 4, 0);
    contentsLayout()->addWidget(d->removeSceneNumbers, 5, 0);
    contentsLayout()->addWidget(d->keepReviewMarks, 6, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 7, 0);

    auto configureImportAvailability = [this] {
        d->importButton->setEnabled(d->importCharacters->isChecked()
                                    || d->importLocations->isChecked()
                                    || d->importScreenplay->isChecked());
    };
    connect(d->importCharacters, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importLocations, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importScreenplay, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importScreenplay, &CheckBox::checkedChanged, this, [this] (bool _checked) {
        d->removeSceneNumbers->setVisible(_checked);
        d->keepReviewMarks->setVisible(_checked);
    });
    connect(d->importButton, &Button::clicked, this, &ImportDialog::importRequested);
    connect(d->cancelButton, &Button::clicked, this, &ImportDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ImportDialog::~ImportDialog() = default;

BusinessLayer::ImportOptions ImportDialog::importOptions() const
{
    BusinessLayer::ImportOptions options;
    options.path = d->importFilePath;
    options.importCharacters = d->importCharacters->isChecked();
    options.importLocations = d->importLocations->isChecked();
    options.importScreenplay = d->importScreenplay->isChecked();
    options.removeSceneNumbers = d->removeSceneNumbers->isChecked();
    options.keepReviewMarks = d->keepReviewMarks->isChecked();
    return options;
}

QWidget* ImportDialog::focusedWidgetAfterShow() const
{
    return d->importCharacters;
}

void ImportDialog::updateTranslations()
{
    const QFileInfo importFileInfo(d->importFilePath);
    setTitle(tr("Import data from the file \"%1\"").arg(importFileInfo.fileName()));

    d->documentsTitle->setText(tr("Documents"));
    d->importCharacters->setText(tr("Import characters"));
    d->importLocations->setText(tr("Import locations"));
    d->screenplayTitle->setText(tr("Screenplay"));
    d->importScreenplay->setText(tr("Import screenplay"));
    d->removeSceneNumbers->setText(tr("Remove scene numbers"));
    d->keepReviewMarks->setText(tr("Import with review marks"));

    d->importButton->setText(tr("Import"));
    d->cancelButton->setText(tr("Cancel"));
}

void ImportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setTop(Ui::DesignSystem::layout().px8());
    titleMargins.setBottom(0);
    for (auto label : { d->documentsTitle, d->screenplayTitle }) {
        label->setContentsMargins(titleMargins);
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto checkBox : { d->importCharacters, d->importLocations,
                           d->importScreenplay, d->removeSceneNumbers, d->keepReviewMarks }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->importButton,
                         d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8()).toMargins());
}

bool ImportDialog::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // Зацикливаем фокус, чтобы он всегда оставался внутри диалога
    //
    if (_watched == d->importButton
        && _event->type() == QEvent::FocusOut) {
        focusedWidgetAfterShow()->setFocus();
    }

    return AbstractDialog::eventFilter(_watched, _event);
}

} //namespace Ui
