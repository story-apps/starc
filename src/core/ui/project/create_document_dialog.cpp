#include "create_document_dialog.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/tree/tree.h>
#include <utils/helpers/names_generator.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>
#include <QStandardItemModel>
#include <QWidgetAction>


namespace Ui {

namespace {
const int kTypeRole = Qt::UserRole + 1;
}

class CreateDocumentDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateDocumentInfo();


    QStandardItemModel* typesModel = nullptr;
    struct {
        QString type;
        int gender = 0;
    } characterNameGeneratorOptions;

    Tree* documentType = nullptr;
    Body1Label* documentInfo = nullptr;
    TextField* documentName = nullptr;
    bool insertIntoParentEnabled = false;
    CheckBox* insertIntoParent = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;
};

CreateDocumentDialog::Implementation::Implementation(QWidget* _parent)
    : typesModel(new QStandardItemModel(_parent))
    , documentType(new Tree(_parent))
    , documentInfo(new Body1Label(_parent))
    , documentName(new TextField(_parent))
    , insertIntoParent(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , createButton(new Button(_parent))
{
    new Shadow(Qt::TopEdge, documentType);

    auto makeItem = [](Domain::DocumentObjectType _type) {
        auto item = new QStandardItem;
        item->setData(Domain::iconForType(_type), Qt::DecorationRole);
        item->setData(static_cast<int>(_type), kTypeRole);
        item->setEditable(false);
        return item;
    };

    if (settingsValue(DataStorageLayer::kComponentsSimpleTextAvailableKey).toBool()) {
        typesModel->appendRow(makeItem(Domain::DocumentObjectType::Folder));
        typesModel->appendRow(makeItem(Domain::DocumentObjectType::SimpleText));
    }
    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Character));
    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Location));
    if (settingsValue(DataStorageLayer::kComponentsScreenplayAvailableKey).toBool()) {
        typesModel->appendRow(makeItem(Domain::DocumentObjectType::Screenplay));
    }
    if (settingsValue(DataStorageLayer::kComponentsComicBookAvailableKey).toBool()) {
        typesModel->appendRow(makeItem(Domain::DocumentObjectType::ComicBook));
    }
    if (settingsValue(DataStorageLayer::kComponentsAudioplayAvailableKey).toBool()) {
        typesModel->appendRow(makeItem(Domain::DocumentObjectType::Audioplay));
    }

    UiHelper::setFocusPolicyRecursively(documentType, Qt::NoFocus);
    documentType->setModel(typesModel);
    documentType->setCurrentIndex(typesModel->index(0, 0));

    documentName->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    insertIntoParent->hide();

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);
}

void CreateDocumentDialog::Implementation::updateDocumentInfo()
{
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
        { Domain::DocumentObjectType::Screenplay,
          tr("Create a document set to streamline your work on the feature film, series, or "
             "animation.") },
        { Domain::DocumentObjectType::ComicBook,
          tr("Create a document set to streamline your work on the comic book, graphic novel, or "
             "manga.") },
        { Domain::DocumentObjectType::Audioplay,
          tr("Create a document set to streamline your work on the audio drama, or podcast.") }
    };

    const auto documentTypeData = static_cast<Domain::DocumentObjectType>(
        documentType->currentIndex().data(kTypeRole).toInt());
    documentInfo->setText(documenTypeToInfo.value(documentTypeData));
    if (documentTypeData == Domain::DocumentObjectType::Character
        || documentTypeData == Domain::DocumentObjectType::Location) {
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
    contentsLayout()->addWidget(d->documentType, 0, 0, 5, 1);
    contentsLayout()->addWidget(d->documentInfo, 0, 1, 1, 1);
    contentsLayout()->addWidget(d->documentName, 1, 1, 1, 1);
    contentsLayout()->addWidget(d->insertIntoParent, 2, 1, 1, 1);
    contentsLayout()->setRowStretch(3, 1);
    contentsLayout()->addLayout(d->buttonsLayout, 4, 0, 1, 2);
    contentsLayout()->setColumnStretch(0, 1);
    contentsLayout()->setColumnStretch(1, 2);

    connect(d->documentType, &Tree::currentIndexChanged, this, [this] {
        d->documentName->setFocus();
        d->updateDocumentInfo();

        const auto documentType = static_cast<Domain::DocumentObjectType>(
            d->documentType->currentIndex().data(kTypeRole).toInt());
        if (documentType == Domain::DocumentObjectType::Character) {
            d->documentName->setTrailingIcon(u8"\U000F076E");
            connect(d->documentName, &TextField::trailingIconPressed, this, [this] {
                d->documentName->setText(
                    NamesGenerator::generate(d->characterNameGeneratorOptions.type,
                                            d->characterNameGeneratorOptions.gender));
            });
            connect(d->documentName, &TextField::trailingIconContextMenuRequested, this, [this] {
                auto menu = new ContextMenu(this);
                QVector<QAction*> actions;

                //
                // Действия для выбора пола
                //
                {
                    auto genderGroup = new QActionGroup(menu);
                    const auto genderIdKey = "gender-id";
                    auto buildGenderAction = [this, &actions, genderGroup,
                                              genderIdKey](const QString& _name, int _gender) {
                        auto action = new QAction;
                        action->setText(_name);
                        action->setProperty(genderIdKey, _gender);
                        action->setCheckable(true);
                        action->setChecked(d->characterNameGeneratorOptions.gender == _gender);
                        action->setIconText(action->isChecked() ? u8"\U000F012C" : u8"\U000F68c0");
                        genderGroup->addAction(action);
                        actions.append(action);
                        return action;
                    };
                    //
                    auto maleAction = buildGenderAction(tr("Male names"), 1);
                    //
                    auto femaleAction = buildGenderAction(tr("Female names"), 2);
                    //
                    auto bothAction = buildGenderAction(tr("Both names"), 0);
                    //
                    for (auto genderAction : {
                             maleAction,
                             femaleAction,
                             bothAction,
                         }) {
                        connect(genderAction, &QAction::toggled, genderAction,
                                [this, genderAction, genderIdKey] {
                                    genderAction->setIconText(genderAction->isChecked()
                                                                  ? u8"\U000F012C"
                                                                  : u8"\U000F68c0");
                                    if (genderAction->isChecked()) {
                                        d->characterNameGeneratorOptions.gender
                                            = genderAction->property(genderIdKey).toInt();
                                    }
                                });
                    }
                }

                //
                // Выбор типа имени
                //
                {
                    auto typeAction = new QAction;
                    typeAction->setText(d->characterNameGeneratorOptions.type.isEmpty()
                                            ? tr("Type of name")
                                            : d->characterNameGeneratorOptions.type);
                    typeAction->setSeparator(true);
                    actions.append(typeAction);

                    auto typesGroup = new QActionGroup(menu);
                    for (const auto& type : NamesGenerator::types()) {
                        auto action = new QAction(typeAction);
                        action->setText(type);
                        action->setCheckable(true);
                        action->setChecked(d->characterNameGeneratorOptions.type == type);
                        action->setIconText(action->isChecked() ? u8"\U000F012C" : u8"\U000F68c0");
                        typesGroup->addAction(action);
                        connect(action, &QAction::toggled, action, [this, typeAction, action] {
                            action->setIconText(action->isChecked() ? u8"\U000F012C"
                                                                    : u8"\U000F68c0");
                            d->characterNameGeneratorOptions.type = action->text();
                            typeAction->setText(action->text());
                        });
                    }
                }

                menu->setBackgroundColor(Ui::DesignSystem::color().background());
                menu->setTextColor(Ui::DesignSystem::color().onBackground());
                menu->setActions(actions);
                connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

                //
                // Покажем меню
                //
                menu->showContextMenu(QCursor::pos());
            });
        } else {
            d->documentName->setTrailingIcon({});
            disconnect(d->documentName, &TextField::trailingIconPressed, nullptr, nullptr);
            disconnect(d->documentName, &TextField::trailingIconContextMenuRequested, nullptr,
                       nullptr);
        }
    });
    connect(d->documentName, &TextField::textChanged, this,
            [this] { d->documentName->setError({}); });
    connect(d->createButton, &Button::clicked, this, [this] {
        const auto documentTypeData = d->documentType->currentIndex().data(kTypeRole);
        Q_ASSERT(documentTypeData.isValid());
        const auto documentType = static_cast<Domain::DocumentObjectType>(documentTypeData.toInt());

        //
        // Персонажи и локации нельзя создавать без названия
        //
        if (d->documentName->text().isEmpty()) {
            QString errorMessage;
            if (documentType == Domain::DocumentObjectType::Character) {
                errorMessage = tr("The character should have a name");
            } else if (documentType == Domain::DocumentObjectType::Location) {
                errorMessage = tr("The location should have a name");
            }
            if (!errorMessage.isEmpty()) {
                d->documentName->setError(errorMessage);
                return;
            }
        }

        emit createPressed(documentType, d->documentName->text());
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateDocumentDialog::hideDialog);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

CreateDocumentDialog::~CreateDocumentDialog() = default;

void CreateDocumentDialog::setDocumentType(Domain::DocumentObjectType _type)
{
    const auto typesModel = d->documentType->model();
    for (int row = 0; row < typesModel->rowCount(); ++row) {
        const auto typeIndex = typesModel->index(row, 0);
        if (typeIndex.data(kTypeRole).isValid()
            && typeIndex.data(kTypeRole).toInt() == static_cast<int>(_type)) {
            d->documentType->setCurrentIndex(typeIndex);
            return;
        }
    }
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
    setTitle(tr("Add document to the story"));

    for (int row = 0; row < d->typesModel->rowCount(); ++row) {
        auto item = d->typesModel->item(row);
        switch (static_cast<Domain::DocumentObjectType>(item->data(kTypeRole).toInt())) {
        case Domain::DocumentObjectType::Folder: {
            item->setText(tr("Folder"));
            break;
        }

        case Domain::DocumentObjectType::SimpleText: {
            item->setText(tr("Text"));
            break;
        }

        case Domain::DocumentObjectType::Character: {
            item->setText(tr("Character"));
            break;
        }

        case Domain::DocumentObjectType::Location: {
            item->setText(tr("Location"));
            break;
        }

        case Domain::DocumentObjectType::Screenplay: {
            item->setText(tr("Screenplay"));
            break;
        }

        case Domain::DocumentObjectType::ComicBook: {
            item->setText(tr("Comic book"));
            break;
        }

        case Domain::DocumentObjectType::Audioplay: {
            item->setText(tr("Audioplay"));
            break;
        }

        default: {
            Q_ASSERT(false);
            break;
        }
        }
    }

    d->documentName->setLabel(tr("Name"));
    d->updateDocumentInfo();
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(tr("Create"));
}

void CreateDocumentDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentMaximumWidth(Ui::DesignSystem::layout().px(600));

    d->documentType->setBackgroundColor(DesignSystem::color().background());
    d->documentType->setTextColor(DesignSystem::color().onBackground());
    d->documentType->setMinimumWidth(d->documentType->sizeHintForColumn(0));
    d->documentType->setMinimumHeight(360 * Ui::DesignSystem::scaleFactor());

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
