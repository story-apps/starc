#include "create_draft_dialog.h"

#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/color_picker/color_picker_popup.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/ui_helper.h>

#include <QBoxLayout>
#include <QFile>
#include <QFileDialog>
#include <QStringListModel>


namespace Ui {

namespace {

/**
 * @brief Состояние диалога
 */
enum State {
    AddNew,
    Edit,
};

} // namespace

class CreateDraftDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Можно ли импортировать драфт для документа
     */
    bool canImport() const;


    Body1Label* draftHint = nullptr;
    TextField* draftName = nullptr;
    ColorPickerPopup* draftColorPopup = nullptr;
    ComboBox* sourceDraft = nullptr;
    QStringListModel* sourceDraftModel = nullptr;
    TextField* sourceFileForImport = nullptr;
    QString importFilters;
    QString importFolder;
    CheckBox* lockEditingDraft = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;

    State state = AddNew;
};

CreateDraftDialog::Implementation::Implementation(QWidget* _parent)
    : draftHint(new Body1Label(_parent))
    , draftName(new TextField(_parent))
    , draftColorPopup(new ColorPickerPopup(_parent))
    , sourceDraft(new ComboBox(_parent))
    , sourceDraftModel(new QStringListModel(sourceDraft))
    , sourceFileForImport(new TextField(_parent))
    , lockEditingDraft(new CheckBox(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , createButton(new Button(_parent))
{
    draftColorPopup->setColorCanBeDeselected(false);
    draftColorPopup->setSelectedColor(Qt::white);
    draftName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    draftName->setTrailingIcon(u8"\U000F0765");
    draftName->setTrailingIconColor(draftColorPopup->selectedColor());
    sourceDraft->setModel(sourceDraftModel);
    sourceFileForImport->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    sourceFileForImport->setTrailingIcon(u8"\U000f0256");
    lockEditingDraft->setChecked(false);
    createButton->setEnabled(false);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);

    sourceFileForImport->hide();
}

bool CreateDraftDialog::Implementation::canImport() const
{
    return !importFilters.isEmpty();
}


// ****


CreateDraftDialog::CreateDraftDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->createButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->draftHint, row++, 0);
    contentsLayout()->addWidget(d->draftName, row++, 0);
    contentsLayout()->addWidget(d->sourceDraft, row++, 0);
    contentsLayout()->addWidget(d->sourceFileForImport, row++, 0);
    contentsLayout()->addWidget(d->lockEditingDraft, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    auto updateCreateButtonAvailability = [this] {
        d->createButton->setEnabled(!d->draftName->text().isEmpty()
                                    && (d->sourceFileForImport->isHidden()
                                        || (d->sourceFileForImport->isVisible()
                                            && QFile::exists(d->sourceFileForImport->text()))));
    };
    connect(d->draftName, &TextField::textChanged, this, updateCreateButtonAvailability);
    connect(d->draftName, &TextField::trailingIconPressed, this, [this] {
        d->draftColorPopup->showPopup(d->draftName, Qt::AlignBottom | Qt::AlignRight);
    });
    connect(d->sourceDraft, &ComboBox::currentIndexChanged, this,
            [this, updateCreateButtonAvailability](const QModelIndex& _currentInex) {
                //
                // Если драфт не может быть импортированным, то ничего не делаем
                //
                if (!d->canImport()) {
                    return;
                }
                //
                // А если может, то проверяем какую опцию выбрал пользователь,
                // - если последнюю, значит будет импортировать, покажем поле для выбора файла
                // - если нет, то скрыть и очистить поле для выбора файл
                //
                d->sourceFileForImport->setVisible(_currentInex.row()
                                                   == d->sourceDraftModel->rowCount() - 1);
                if (d->sourceFileForImport->isHidden()) {
                    d->sourceFileForImport->clear();
                }
                updateCreateButtonAvailability();
            });
    connect(d->sourceFileForImport, &TextField::textChanged, this, updateCreateButtonAvailability);
    connect(d->sourceFileForImport, &TextField::trailingIconPressed, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, tr("Choose the file to import"),
                                                       d->importFolder, d->importFilters);
        if (path.isEmpty()) {
            return;
        }

        //
        // Старый вордовский формат не поддерживаем
        //
        if (path.endsWith(".doc", Qt::CaseInsensitive)) {
            StandardDialog::information(parentWidget(), tr("File format not supported"),
                                        tr("Importing from DOC files is not supported. You need to "
                                           "save the file in DOCX format and repeat the import."));
            return;
        }

        //
        // Если всё в порядке, принимаем файл к импорту
        //
        d->sourceFileForImport->setText(path);
    });
    connect(d->draftColorPopup, &ColorPickerPopup::selectedColorChanged, this,
            [this](const QColor& _color) { d->draftName->setTrailingIconColor(_color); });
    connect(d->createButton, &Button::clicked, this, [this] {
        emit savePressed(d->draftName->text(), d->draftColorPopup->selectedColor(),
                         d->sourceDraft->currentIndex().row(), d->lockEditingDraft->isChecked());
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateDraftDialog::hideDialog);
}

CreateDraftDialog::~CreateDraftDialog() = default;

void CreateDraftDialog::setDrafts(const QStringList& _drafts, int _selectDraftIndex,
                                  Domain::DocumentObjectType _documentType)
{
    d->importFilters.clear();
    switch (_documentType) {
    case Domain::DocumentObjectType::ScreenplayText: {
        d->importFilters = DialogHelper::filtersForImportScreenplay();
        break;
    }
    case Domain::DocumentObjectType::ComicBookText: {
        d->importFilters = DialogHelper::filtersForImportComicBook();
        break;
    }
    case Domain::DocumentObjectType::AudioplayText: {
        d->importFilters = DialogHelper::filtersForImportAudioplay();
        break;
    }
    case Domain::DocumentObjectType::StageplayText: {
        d->importFilters = DialogHelper::filtersForImportStageplay();
        break;
    }
    case Domain::DocumentObjectType::NovelText: {
        d->importFilters = DialogHelper::filtersForImportNovel();
        break;
    }
    default: {
        break;
    }
    }

    auto drafts = _drafts;
    if (d->canImport()) {
        drafts.append(tr("Draft from external file"));
    }

    d->sourceDraft->setVisible(drafts.size() > 1);
    d->sourceDraftModel->setStringList(drafts);
    d->sourceDraft->setCurrentText(drafts.at(_selectDraftIndex));
}

QString CreateDraftDialog::importFilePath() const
{
    return d->sourceFileForImport->text();
}

void CreateDraftDialog::setImportFolder(const QString& _path)
{
    d->importFolder = _path;
}

void CreateDraftDialog::edit(const QString& _name, const QColor& _color, bool _readOnly,
                             bool _comparison)
{
    d->state = Edit;
    updateTranslations();

    d->draftName->setText(_name);
    d->draftName->setTrailingIconColor(_color);
    d->sourceDraft->hide();
    d->sourceFileForImport->hide();
    d->draftColorPopup->setSelectedColor(_color);
    d->lockEditingDraft->setChecked(_readOnly);
    d->lockEditingDraft->setVisible(_comparison == false);
}

QWidget* CreateDraftDialog::focusedWidgetAfterShow() const
{
    return d->draftName;
}

QWidget* CreateDraftDialog::lastFocusableWidget() const
{
    return d->createButton->isEnabled() ? d->createButton : d->cancelButton;
}

void CreateDraftDialog::updateTranslations()
{
    setTitle(d->state == AddNew ? tr("Create document draft") : tr("Edit document draft"));

    d->draftHint->setText(tr("Store actual draft as a separate document to keep your progress."));
    d->draftName->setLabel(tr("Draft name"));
    d->sourceDraft->setLabel(tr("Based on"));
    d->sourceFileForImport->setLabel(tr("Choose file with draft to import"));
    d->sourceFileForImport->setTrailingIconToolTip(tr("Choose file for importing"));
    d->lockEditingDraft->setText(tr("Lock draft text editing"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(d->state == AddNew ? tr("Create") : tr("Save"));
}

void CreateDraftDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->draftHint->setContentsMargins(DesignSystem::layout().px24(), 0,
                                     DesignSystem::layout().px16(), DesignSystem::layout().px24());
    d->draftHint->setBackgroundColor(DesignSystem::color().background());
    d->draftHint->setTextColor(DesignSystem::color().onBackground());
    d->draftName->setTextColor(DesignSystem::color().onBackground());
    d->draftName->setBackgroundColor(DesignSystem::color().onBackground());
    d->draftColorPopup->setBackgroundColor(DesignSystem::color().background());
    d->draftColorPopup->setTextColor(DesignSystem::color().onBackground());
    d->sourceDraft->setTextColor(DesignSystem::color().onBackground());
    d->sourceDraft->setBackgroundColor(DesignSystem::color().onBackground());
    d->sourceDraft->setPopupBackgroundColor(DesignSystem::color().background());
    d->sourceDraft->setCustomMargins({ DesignSystem::layout().px24(),
                                       DesignSystem::compactLayout().px16(),
                                       DesignSystem::layout().px24(), 0.0 });
    d->sourceFileForImport->setTextColor(DesignSystem::color().onBackground());
    d->sourceFileForImport->setBackgroundColor(DesignSystem::color().onBackground());
    d->sourceFileForImport->setCustomMargins({ DesignSystem::layout().px24(),
                                               DesignSystem::compactLayout().px16(),
                                               DesignSystem::layout().px24(), 0.0 });
    d->lockEditingDraft->setTextColor(DesignSystem::color().onBackground());
    d->lockEditingDraft->setBackgroundColor(DesignSystem::color().background());

    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->createButton, UiHelper::DialogAccept);

    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
