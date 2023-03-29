#include "dictionaries_view.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/tree/tree.h>
#include <ui/widgets/tree/tree_delegate.h>

#include <QBoxLayout>
#include <QScrollBar>


namespace Ui {

class DictionariesView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    bool isReadOnly = false;

    ComboBox* dictionaryType = nullptr;
    IconButton* addItemToDictionary = nullptr;
    Tree* dictionaryItems = nullptr;
    TextFieldItemDelegate* dictionaryItemsDelegate = nullptr;
};

DictionariesView::Implementation::Implementation(QWidget* _parent)
    : dictionaryType(new ComboBox(_parent))
    , addItemToDictionary(new IconButton(_parent))
    , dictionaryItems(new Tree(_parent))
    , dictionaryItemsDelegate(new TextFieldItemDelegate(dictionaryItems))
{
    addItemToDictionary->setIcon(u8"\U000F0415");
    dictionaryItems->setRootIsDecorated(false);
    dictionaryItems->setItemDelegate(dictionaryItemsDelegate);
    dictionaryItemsDelegate->setHoverTrailingIcon(u8"\U000F01B4");

    new Shadow(Qt::TopEdge, dictionaryItems);
}


// ****


DictionariesView::DictionariesView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(d->dictionaryType);
    topLayout->addWidget(d->addItemToDictionary);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(d->dictionaryItems);
    setLayout(layout);


    connect(d->dictionaryType, &ComboBox::currentIndexChanged, this,
            [this](const QModelIndex& _typeIndex) {
                d->dictionaryItemsDelegate->setLabel(_typeIndex.data().toString());
                emit typeChanged(_typeIndex);
            });
    connect(d->addItemToDictionary, &IconButton::clicked, this,
            [this] { emit addItemRequested(d->dictionaryType->currentIndex()); });
    connect(d->dictionaryItems, &Tree::clicked, this, [this] {
        const auto clickPosition = d->dictionaryItems->mapFromGlobal(QCursor::pos());
        if (!d->isReadOnly && d->dictionaryItems->isOnItemTrilingIcon(clickPosition)) {
            emit removeItemRequested(d->dictionaryType->currentIndex(),
                                     d->dictionaryItems->currentIndex());
        }
    });
    connect(d->dictionaryItemsDelegate, &TextFieldItemDelegate::commitData, this,
            [this](QWidget* _editor) {
                auto textField = qobject_cast<TextField*>(_editor);
                emit editItemRequested(d->dictionaryType->currentIndex(),
                                       d->dictionaryItems->currentIndex(), textField->text());
            });
}

DictionariesView::~DictionariesView() = default;

void DictionariesView::setReadOnly(bool _readOnly)
{
    if (d->isReadOnly == _readOnly) {
        return;
    }

    d->isReadOnly = _readOnly;
    d->addItemToDictionary->setEnabled(!d->isReadOnly);
    d->dictionaryItems->setItemDelegate(d->isReadOnly ? nullptr : d->dictionaryItemsDelegate);
    d->dictionaryItems->setEditTriggers(d->isReadOnly ? QAbstractItemView::NoEditTriggers
                                                      : QAbstractItemView::DoubleClicked);
}

void DictionariesView::setTypes(QAbstractItemModel* _types)
{
    if (d->dictionaryType->model() != nullptr) {
        d->dictionaryType->model()->disconnect(this);
    }

    d->dictionaryType->setModel(_types);

    if (d->dictionaryType->model() != nullptr) {
        auto model = d->dictionaryType->model();
        connect(model, &QAbstractItemModel::modelReset, this, [this, model] {
            if (model->rowCount() > 0) {
                d->dictionaryType->setCurrentIndex(model->index(0, 0));
            }
        });
    }
}

void DictionariesView::setDictionaryItems(QAbstractItemModel* _items)
{
    d->dictionaryItems->setModel(_items);
}

QModelIndex DictionariesView::currentTypeIndex() const
{
    return d->dictionaryType->currentIndex();
}

void DictionariesView::editLastItem()
{
    auto model = d->dictionaryItems->model();
    if (model == nullptr) {
        return;
    }

    const auto index = model->index(model->rowCount() - 1, 0);
    d->dictionaryItems->setCurrentIndex(index);
    d->dictionaryItems->edit(index);
    d->dictionaryItems->verticalScrollBar()->setValue(
        d->dictionaryItems->verticalScrollBar()->maximum());
}

void DictionariesView::updateTranslations()
{
    d->dictionaryType->setLabel(tr("Dictionary type"));
    d->addItemToDictionary->setToolTip(tr("Add new item to the current dictionary"));
}

void DictionariesView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().primary());
    d->dictionaryType->setBackgroundColor(Ui::DesignSystem::color().onPrimary());
    d->dictionaryType->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->dictionaryType->setPopupBackgroundColor(Ui::DesignSystem::color().primary());
    d->dictionaryType->setCustomMargins(
        { Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px24(),
          Ui::DesignSystem::compactLayout().px16(), Ui::DesignSystem::compactLayout().px24() });
    d->addItemToDictionary->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->addItemToDictionary->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->addItemToDictionary->setContentsMargins(0, Ui::DesignSystem::compactLayout().px24(),
                                               Ui::DesignSystem::layout().px12(),
                                               Ui::DesignSystem::compactLayout().px24());
    d->dictionaryItems->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->dictionaryItems->setTextColor(Ui::DesignSystem::color().onPrimary());
    d->dictionaryItemsDelegate->setAdditionalLeftMargin(Ui::DesignSystem::layout().px8());
}

} // namespace Ui
