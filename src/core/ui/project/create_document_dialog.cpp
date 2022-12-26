#include "create_document_dialog.h"

#include "create_document_dialog_option.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/layouts/flow_layout/flow_layout.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/names_generator.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>


namespace Ui {

namespace {
const int kTypeRole = Qt::UserRole + 1;
}

class CreateDocumentDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateDocumentInfo(Domain::DocumentObjectType _type);


    Widget* optionsContainer = nullptr;
    QVBoxLayout* optionsLayout = nullptr;
    AbstractLabel* storyTitle;
    AbstractLabel* storyWorldTitle;
    AbstractLabel* otherTitle;
    QVector<CreateDocumentDialogOption*> options;

    H6Label* title = nullptr;
    Body1Label* documentInfo = nullptr;
    TextField* documentName = nullptr;
    bool insertIntoParentEnabled = false;
    CheckBox* insertIntoParent = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;
};

CreateDocumentDialog::Implementation::Implementation(QWidget* _parent)
    : optionsContainer(new Widget(_parent))
    , optionsLayout(new QVBoxLayout(optionsContainer))
    , storyTitle(new Subtitle1Label(_parent))
    , storyWorldTitle(new Subtitle1Label(_parent))
    , otherTitle(new Subtitle1Label(_parent))
    , title(new H6Label(_parent))
    , documentInfo(new Body1Label(_parent))
    , documentName(new TextField(_parent))
    , insertIntoParent(new CheckBox(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , createButton(new Button(_parent))
{
    auto makeOption = [this, &_parent](Domain::DocumentObjectType _type) {
        auto option = new CreateDocumentDialogOption(_type, _parent);
        options.append(option);
        return option;
    };

    optionsLayout->setContentsMargins({});
    optionsLayout->setSpacing(0);
    optionsLayout->addWidget(storyTitle);
    {
        auto layout = new FlowLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        if (settingsValue(DataStorageLayer::kComponentsScreenplayAvailableKey).toBool()) {
            layout->addWidget(makeOption(Domain::DocumentObjectType::Screenplay));
        }
        if (settingsValue(DataStorageLayer::kComponentsComicBookAvailableKey).toBool()) {
            layout->addWidget(makeOption(Domain::DocumentObjectType::ComicBook));
        }
        if (settingsValue(DataStorageLayer::kComponentsAudioplayAvailableKey).toBool()) {
            layout->addWidget(makeOption(Domain::DocumentObjectType::Audioplay));
        }
        if (settingsValue(DataStorageLayer::kComponentsStageplayAvailableKey).toBool()) {
            layout->addWidget(makeOption(Domain::DocumentObjectType::Stageplay));
        }
        optionsLayout->addLayout(layout);
    }
    optionsLayout->addWidget(storyWorldTitle);
    {
        auto layout = new FlowLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(makeOption(Domain::DocumentObjectType::Character));
        layout->addWidget(makeOption(Domain::DocumentObjectType::Location));
        layout->addWidget(makeOption(Domain::DocumentObjectType::World));
        optionsLayout->addLayout(layout);
    }
    optionsLayout->addWidget(otherTitle);
    {
        auto layout = new FlowLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        if (settingsValue(DataStorageLayer::kComponentsSimpleTextAvailableKey).toBool()) {
            layout->addWidget(makeOption(Domain::DocumentObjectType::Folder));
            layout->addWidget(makeOption(Domain::DocumentObjectType::SimpleText));
        }
        layout->addWidget(makeOption(Domain::DocumentObjectType::ImagesGallery));
        optionsLayout->addLayout(layout);
    }

    UiHelper::setFocusPolicyRecursively(optionsContainer, Qt::NoFocus);
    options.constFirst()->setChecked(true);

    documentName->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    insertIntoParent->hide();

    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);
}

void CreateDocumentDialog::Implementation::updateDocumentInfo(Domain::DocumentObjectType _type)
{
    const QHash<Domain::DocumentObjectType, QString> documenTypeToTitle = {
        { Domain::DocumentObjectType::Folder, tr("Add folder") },
        { Domain::DocumentObjectType::SimpleText, tr("Add text") },
        { Domain::DocumentObjectType::Character, tr("Add character") },
        { Domain::DocumentObjectType::Location, tr("Add location") },
        { Domain::DocumentObjectType::World, tr("Add world") },
        { Domain::DocumentObjectType::Screenplay, tr("Add screenplay") },
        { Domain::DocumentObjectType::ComicBook, tr("Add comic book") },
        { Domain::DocumentObjectType::Audioplay, tr("Add audioplay") },
        { Domain::DocumentObjectType::Stageplay, tr("Add stageplay") },
        { Domain::DocumentObjectType::ImagesGallery, tr("Add image gallery") },
    };
    const QHash<Domain::DocumentObjectType, QString> documenTypeToInfo = {
        { Domain::DocumentObjectType::Folder,
          tr("Create a folder to group documents inside the story.") },
        { Domain::DocumentObjectType::SimpleText,
          tr("Create a plain text document to write out ideas and notes.") },
        { Domain::DocumentObjectType::Character,
          tr("Create a document with full character's description to track his relations and "
             "follow his journey within the story.") },
        { Domain::DocumentObjectType::Location,
          tr("Create a document to note down the location's description and keep the details.") },
        { Domain::DocumentObjectType::World,
          tr("Create a document with detailed world description to keep all the notes in one "
             "place") },
        { Domain::DocumentObjectType::Screenplay,
          tr("Create a document set to streamline your work on the feature film, series, or "
             "animation.") },
        { Domain::DocumentObjectType::ComicBook,
          tr("Create a document set to streamline your work on the comic book, graphic novel, or "
             "manga.") },
        { Domain::DocumentObjectType::Audioplay,
          tr("Create a document set to streamline your work on the audio drama, or podcast.") },
        { Domain::DocumentObjectType::Stageplay,
          tr("Create a document set to streamline your work on the stage play, or musical.") },
        { Domain::DocumentObjectType::ImagesGallery,
          tr("Create a moodboard with atmospheric images or photos.") },
    };

    title->setText(documenTypeToTitle.value(_type));
    documentInfo->setText(documenTypeToInfo.value(_type));
    if (_type == Domain::DocumentObjectType::Character
        || _type == Domain::DocumentObjectType::Location) {
        insertIntoParent->hide();
    } else {
        insertIntoParent->setVisible(insertIntoParentEnabled);
    }
}


// ****

CreateDocumentDialog::CreateDocumentDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->createButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    contentsLayout()->addWidget(d->optionsContainer, 0, 0, 6, 1);
    contentsLayout()->addWidget(d->title, 0, 1, 1, 1);
    contentsLayout()->addWidget(d->documentInfo, 1, 1, 1, 1);
    contentsLayout()->addWidget(d->documentName, 2, 1, 1, 1);
    contentsLayout()->addWidget(d->insertIntoParent, 3, 1, 1, 1);
    contentsLayout()->setRowStretch(4, 1);
    contentsLayout()->addLayout(d->buttonsLayout, 5, 1, 1, 1);
    contentsLayout()->setColumnStretch(0, 2);
    contentsLayout()->setColumnStretch(1, 1);

    for (auto option : std::as_const(d->options)) {
        connect(option, &CreateDocumentDialogOption::checkedChanged, this, [this](bool _isChecked) {
            if (!_isChecked) {
                return;
            }

            for (auto option : std::as_const(d->options)) {
                if (option != sender()) {
                    option->setChecked(false);
                    continue;
                }

                d->updateDocumentInfo(option->documentType());
            }
        });
    }
    connect(d->documentName, &TextField::textChanged, this,
            [this] { d->documentName->setError({}); });
    connect(d->createButton, &Button::clicked, this, [this] {
        Domain::DocumentObjectType documentType = Domain::DocumentObjectType::Undefined;
        for (auto option : std::as_const(d->options)) {
            if (option->isChecked()) {
                documentType = option->documentType();
                break;
            }
        }

        //
        // Персонажи и локации нельзя создавать без названия
        //
        if (d->documentName->text().isEmpty()) {
            QString errorMessage;
            if (documentType == Domain::DocumentObjectType::Character) {
                errorMessage = tr("The character should have a name");
            } else if (documentType == Domain::DocumentObjectType::Location) {
                errorMessage = tr("The location should have a name");
            } else if (documentType == Domain::DocumentObjectType::World) {
                errorMessage = tr("The world should have a name");
            }
            if (!errorMessage.isEmpty()) {
                d->documentName->setError(errorMessage);
                return;
            }
        }

        emit createPressed(documentType, d->documentName->text());
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateDocumentDialog::hideDialog);
}

CreateDocumentDialog::~CreateDocumentDialog() = default;

void CreateDocumentDialog::setDocumentType(Domain::DocumentObjectType _type)
{
    for (auto option : std::as_const(d->options)) {
        if (option->documentType() == _type) {
            option->setChecked(true);
            break;
        }
    }
}

void CreateDocumentDialog::setNameError(const QString& _error)
{
    d->documentName->setError(_error);
}

void CreateDocumentDialog::setInsertionParent(const QString& _parentName)
{
    if (_parentName.isEmpty()) {
        d->insertIntoParentEnabled = false;
        d->insertIntoParent->hide();
        return;
    }

    d->insertIntoParentEnabled = true;
    d->insertIntoParent->setText(QString("%1 \"%2\"").arg(tr("Insert into"), _parentName));
    d->insertIntoParent->setChecked(true);
    d->insertIntoParent->show();
}

bool CreateDocumentDialog::needInsertIntoParent() const
{
    return d->insertIntoParent->isChecked();
}

QWidget* CreateDocumentDialog::focusedWidgetAfterShow() const
{
    return d->documentName;
}

QWidget* CreateDocumentDialog::lastFocusableWidget() const
{
    return d->createButton;
}

void CreateDocumentDialog::updateTranslations()
{
    d->storyTitle->setText(tr("Story"));
    d->storyWorldTitle->setText(tr("World of story"));
    d->otherTitle->setText(tr("Structure & notes"));

    d->title->setText(tr("Add document"));
    for (auto option : std::as_const(d->options)) {
        if (option->isChecked()) {
            d->updateDocumentInfo(option->documentType());
            break;
        }
    }
    d->documentName->setLabel(tr("Name"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(tr("Create"));
}

void CreateDocumentDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentFixedWidth(Ui::DesignSystem::layout().px(888));

    d->optionsContainer->setBackgroundColor(DesignSystem::color().surface());
    d->optionsContainer->setFixedHeight(DesignSystem::layout().px(542));
    d->optionsLayout->setContentsMargins(
        isLeftToRight() ? DesignSystem::layout().px16() : 0.0, DesignSystem::layout().px8(),
        isLeftToRight() ? 0.0 : DesignSystem::layout().px16(), DesignSystem::layout().px16());
    for (auto title : std::vector<Widget*>{
             d->storyTitle,
             d->storyWorldTitle,
             d->otherTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().surface());
        title->setTextColor(DesignSystem::color().onSurface());
        title->setContentsMargins(DesignSystem::layout().px16(), DesignSystem::layout().px12(),
                                  DesignSystem::layout().px16(), DesignSystem::layout().px4());
    }
    for (auto option : std::as_const(d->options)) {
        option->setBackgroundColor(DesignSystem::color().surface());
        option->setTextColor(DesignSystem::color().onSurface());
        option->setContentsMargins(DesignSystem::layout().px8(), DesignSystem::layout().px8(),
                                   DesignSystem::layout().px8(), DesignSystem::layout().px8());
    }

    d->title->setBackgroundColor(Ui::DesignSystem::color().background());
    d->title->setTextColor(Ui::DesignSystem::color().onBackground());
    d->title->setContentsMargins(DesignSystem::layout().px24(), DesignSystem::layout().px24(),
                                 DesignSystem::layout().px24(), DesignSystem::layout().px16());
    d->documentName->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentName->setBackgroundColor(Ui::DesignSystem::color().onBackground());

    auto documentInfoMargins = Ui::DesignSystem::label().margins().toMargins();
    documentInfoMargins.setTop(0);
    d->documentInfo->setContentsMargins(documentInfoMargins);
    d->documentInfo->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentInfo->setBackgroundColor(Ui::DesignSystem::color().background());

    d->insertIntoParent->setTextColor(Ui::DesignSystem::color().onBackground());
    d->insertIntoParent->setBackgroundColor(Ui::DesignSystem::color().background());

    for (auto button : { d->cancelButton, d->createButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
