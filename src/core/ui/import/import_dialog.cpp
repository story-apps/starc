#include "import_dialog.h"

#include <business_layer/import/screenplay/screenlay_import_options.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/label/label.h>

#include <QEvent>
#include <QFileInfo>
#include <QGridLayout>


namespace Ui {

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
    CheckBox* keepSceneNumbers = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* importButton = nullptr;
};


ImportDialog::Implementation::Implementation(const QString& _importFilePath, QWidget* _parent)
    : importFilePath(_importFilePath)
    , documentsTitle(new OverlineLabel(_parent))
    , importCharacters(new CheckBox(_parent))
    , importLocations(new CheckBox(_parent))
    , screenplayTitle(new OverlineLabel(_parent))
    , importScreenplay(new CheckBox(_parent))
    , keepSceneNumbers(new CheckBox(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , importButton(new Button(_parent))
{
    for (auto checkBox :
         { importCharacters, importLocations, importScreenplay, keepSceneNumbers }) {
        checkBox->setChecked(true);
    }

    keepSceneNumbers->setChecked(false);
    keepSceneNumbers->hide();

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(importButton);
}


// ****


ImportDialog::ImportDialog(const QString& _importFilePath, QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(_importFilePath, this))
{
    setAcceptButton(d->importButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->addWidget(d->documentsTitle, 0, 0);
    contentsLayout()->addWidget(d->importCharacters, 1, 0);
    contentsLayout()->addWidget(d->importLocations, 2, 0);
    contentsLayout()->addWidget(d->screenplayTitle, 3, 0);
    contentsLayout()->addWidget(d->importScreenplay, 4, 0);
    contentsLayout()->addWidget(d->keepSceneNumbers, 5, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 6, 0);

    auto configureImportAvailability = [this] {
        d->importButton->setEnabled(d->importCharacters->isChecked()
                                    || d->importLocations->isChecked()
                                    || d->importScreenplay->isChecked());
    };
    connect(d->importCharacters, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importLocations, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importScreenplay, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importScreenplay, &CheckBox::checkedChanged, this, [this](bool _checked) {
        //        d->keepSceneNumbers->setVisible(_checked);
    });
    connect(d->importButton, &Button::clicked, this, &ImportDialog::importRequested);
    connect(d->cancelButton, &Button::clicked, this, &ImportDialog::canceled);
}

ImportDialog::~ImportDialog() = default;

BusinessLayer::ScreenplayImportOptions ImportDialog::importOptions() const
{
    BusinessLayer::ScreenplayImportOptions options;
    options.filePath = d->importFilePath;
    options.importCharacters = d->importCharacters->isChecked();
    options.importLocations = d->importLocations->isChecked();
    options.importScreenplay = d->importScreenplay->isChecked();
    options.keepSceneNumbers = d->keepSceneNumbers->isChecked();
    return options;
}

QWidget* ImportDialog::focusedWidgetAfterShow() const
{
    return d->importCharacters;
}

QWidget* ImportDialog::lastFocusableWidget() const
{
    return d->importButton;
}

void ImportDialog::updateTranslations()
{
    const QFileInfo importFileInfo(d->importFilePath);
    setTitle(QString("%1 \"%2\"").arg(tr("Import data from the file"), importFileInfo.fileName()));

    d->documentsTitle->setText(tr("Documents"));
    d->importCharacters->setText(tr("Import characters"));
    d->importLocations->setText(tr("Import locations"));
    d->screenplayTitle->setText(tr("Screenplay"));
    d->importScreenplay->setText(tr("Import screenplay"));
    d->keepSceneNumbers->setText(tr("Keep scene numbers"));

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

    for (auto checkBox :
         { d->importCharacters, d->importLocations, d->importScreenplay, d->keepSceneNumbers }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->importButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
