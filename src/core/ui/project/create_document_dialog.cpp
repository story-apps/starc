#include "create_document_dialog.h"

#include <domain/document_object.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/tree/tree.h>

#include <QGridLayout>
#include <QStandardItemModel>


namespace Ui
{

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
    TextField* documentName = nullptr;
    Body1Label* documentInfo = nullptr;
    CheckBox* insertIntoParent = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;
};

CreateDocumentDialog::Implementation::Implementation(QWidget* _parent)
    : typesModel(new QStandardItemModel(_parent)),
      documentType(new Tree(_parent)),
      documentName(new TextField(_parent)),
      documentInfo(new Body1Label(_parent)),
      insertIntoParent(new CheckBox(_parent)),
      cancelButton(new Button(_parent)),
      createButton(new Button(_parent))
{
    auto makeItem = [] (Domain::DocumentObjectType _type) {
        auto item = new QStandardItem;
        item->setData(Domain::iconForType(_type), Qt::DecorationRole);
        item->setData(static_cast<int>(_type), kMimeTypeRole);
        return item;
    };

    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Screenplay));

    documentType->setModel(typesModel);
    documentType->setCurrentIndex(typesModel->index(0, 0));

    insertIntoParent->hide();

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);
}

void CreateDocumentDialog::Implementation::updateDocumentInfo()
{
    const QHash<Domain::DocumentObjectType, QString> documenTypeToInfo
            = {{ Domain::DocumentObjectType::Screenplay,
                 tr("Create a document that will provide abilities for working "
                    "on the script of the feature film, series or animation") }};

    const auto documentTypeData = documentType->currentIndex().data(kMimeTypeRole).toInt();
    documentInfo->setText(documenTypeToInfo.value(static_cast<Domain::DocumentObjectType>(documentTypeData)));
}


// ****

CreateDocumentDialog::CreateDocumentDialog(QWidget *_parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    setAcceptButton(d->createButton);
    setRejectButton(d->cancelButton);
    setContentMaximumWidth(600);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    contentsLayout()->addWidget(d->documentType, 0, 0, 4, 1);
    contentsLayout()->addWidget(d->documentName, 0, 1, 1, 1);
    contentsLayout()->addWidget(d->documentInfo, 1, 1, 1, 1);
    contentsLayout()->setRowStretch(2, 1);
    contentsLayout()->addWidget(d->insertIntoParent, 3, 1, 1, 1);
    contentsLayout()->addLayout(d->buttonsLayout, 4, 0, 1, 2);
    contentsLayout()->setColumnStretch(0, 1);
    contentsLayout()->setColumnStretch(1, 2);

    connect(d->documentType, &Tree::currentIndexChanged, this, [this] { d->updateDocumentInfo(); });
    connect(d->createButton, &Button::clicked, this, [this] {
        const auto documentTypeData = d->documentType->currentIndex().data(kMimeTypeRole);
        Q_ASSERT(documentTypeData.isValid());
        const auto documentType = static_cast<Domain::DocumentObjectType>(documentTypeData.toInt());
        emit createPressed(documentType, d->documentName->text());
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateDocumentDialog::hideDialog);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

CreateDocumentDialog::~CreateDocumentDialog() = default;

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

    d->typesModel->item(0)->setText(tr("Screenplay"));

    d->documentName->setLabel(tr("Name"));
    d->updateDocumentInfo();
//    d->insertIntoParent->setText(tr("Insert into parent"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(tr("Create"));
}

void CreateDocumentDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->documentType->setBackgroundColor(DesignSystem::color().background());
    d->documentType->setTextColor(DesignSystem::color().onPrimary());

    d->documentName->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentName->setBackgroundColor(Ui::DesignSystem::color().onBackground());

    d->documentInfo->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    d->documentInfo->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentInfo->setBackgroundColor(Ui::DesignSystem::color().background());

    d->insertIntoParent->setTextColor(Ui::DesignSystem::color().onBackground());
    d->insertIntoParent->setBackgroundColor(Ui::DesignSystem::color().background());

    for (auto button : { d->cancelButton,
                         d->createButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->buttonsLayout->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8()).toMargins());
}

} // namespace Ui
