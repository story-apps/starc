#include "create_document_dialog.h"

#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/tree/tree.h>
#include <utils/helpers/ui_helper.h>

#include <QGridLayout>
#include <QStandardItemModel>


namespace Ui {

namespace {
const int kMimeTypeRole = Qt::UserRole + 1;
}

class CreateDocumentDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateDocumentInfo();

    QStandardItemModel* typesModel = nullptr;

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
        item->setData(static_cast<int>(_type), kMimeTypeRole);
        item->setEditable(false);
        return item;
    };

    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Folder));
    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Text));
    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Character));
    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Location));
    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Screenplay));
    typesModel->appendRow(makeItem(Domain::DocumentObjectType::ComicBook));

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
        { Domain::DocumentObjectType::Text,
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
             "manga.") }
    };

    const auto documentTypeData = static_cast<Domain::DocumentObjectType>(
        documentType->currentIndex().data(kMimeTypeRole).toInt());
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
    });
    connect(d->documentName, &TextField::textChanged, this,
            [this] { d->documentName->setError({}); });
    connect(d->createButton, &Button::clicked, this, [this] {
        const auto documentTypeData = d->documentType->currentIndex().data(kMimeTypeRole);
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
        if (typeIndex.data(kMimeTypeRole).isValid()
            && typeIndex.data(kMimeTypeRole).toInt() == static_cast<int>(_type)) {
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

    d->typesModel->item(0)->setText(tr("Folder"));
    d->typesModel->item(1)->setText(tr("Text"));
    d->typesModel->item(2)->setText(tr("Character"));
    d->typesModel->item(3)->setText(tr("Location"));
    d->typesModel->item(4)->setText(tr("Screenplay"));
    d->typesModel->item(5)->setText(tr("Comic book"));

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
