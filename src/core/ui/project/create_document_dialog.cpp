#include "create_document_dialog.h"

#include <domain/document_object.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
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

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;
};

CreateDocumentDialog::Implementation::Implementation(QWidget* _parent)
    : typesModel(new QStandardItemModel(_parent)),
      documentType(new Tree(_parent)),
      documentName(new TextField(_parent)),
      documentInfo(new Body1Label(_parent)),
      cancelButton(new Button(_parent)),
      createButton(new Button(_parent))
{
    auto makeItem = [] (Domain::DocumentObjectType _type) {
        auto item = new QStandardItem;
        item->setData(Domain::iconForType(_type), Qt::DecorationRole);
        item->setData(Domain::mimeTypeFor(_type), kMimeTypeRole);
        return item;
    };

    typesModel->appendRow(makeItem(Domain::DocumentObjectType::Screenplay));
    documentType->setModel(typesModel);
    documentType->setCurrentIndex(typesModel->index(0, 0));

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);
}

void CreateDocumentDialog::Implementation::updateDocumentInfo()
{
    const QHash<QString, QString> documenTypeToInfo
            = {{ Domain::mimeTypeFor(Domain::DocumentObjectType::Screenplay),
                 tr("Some description of the screenplay document") }};
    documentInfo->setText(documenTypeToInfo.value(documentType->currentIndex().data(kMimeTypeRole).toString()));
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
    contentsLayout()->addWidget(d->documentType, 0, 0, 3, 1);
    contentsLayout()->addWidget(d->documentName, 0, 1, 1, 1);
    contentsLayout()->addWidget(d->documentInfo, 1, 1, 1, 1);
    contentsLayout()->setRowStretch(2, 1);
    contentsLayout()->addLayout(d->buttonsLayout, 3, 0, 1, 2);
    contentsLayout()->setColumnStretch(0, 1);
    contentsLayout()->setColumnStretch(1, 2);

    connect(d->documentType, &Tree::currentIndexChanged, this, [this] { d->updateDocumentInfo(); });
    connect(d->cancelButton, &Button::clicked, this, &CreateDocumentDialog::hideDialog);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

CreateDocumentDialog::~CreateDocumentDialog() = default;

QWidget* CreateDocumentDialog::focusedWidgetAfterShow() const
{
    return d->documentType;
}

void CreateDocumentDialog::updateTranslations()
{
    setTitle(tr("Add document to the story"));

    d->typesModel->item(0)->setText(tr("Screenplay"));

    d->documentName->setLabel(tr("Name"));
    d->updateDocumentInfo();
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(tr("Create"));
}

void CreateDocumentDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    d->documentType->setBackgroundColor(DesignSystem::color().background());
    d->documentType->setTextColor(DesignSystem::color().onPrimary());

    d->documentInfo->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    d->documentInfo->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentInfo->setBackgroundColor(Ui::DesignSystem::color().background());

    d->documentName->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentName->setBackgroundColor(Ui::DesignSystem::color().background());

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
