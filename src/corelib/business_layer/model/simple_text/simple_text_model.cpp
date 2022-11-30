#include "simple_text_model.h"

#include "simple_text_model_chapter_item.h"

#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/simple_text_template.h>
#include <domain/document_object.h>


namespace BusinessLayer {

namespace {

const char* kMimeType = "application/x-starc/text/item";

/**
 * @brief Найти первый текстовый элемент вложенный в заданный
 */
TextModelItem* firstTextItem(TextModelItem* _item)
{
    Q_ASSERT(_item->type() != TextModelItemType::Text);
    for (auto childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
        auto childItem = _item->childAt(childIndex);
        if (childItem->type() == TextModelItemType::Group) {
            return firstTextItem(childItem);
        } else {
            return childItem;
        }
    }
    return nullptr;
};

} // namespace

class SimpleTextModel::Implementation
{
public:
    explicit Implementation(SimpleTextModel* _q);

    /**
     * @brief Получить корневой элемент
     */
    TextModelItem* rootItem() const;

    /**
     * @brief Обновить название документа
     */
    void updateDocumentName(const QModelIndex& _index = {});

    /**
     * @brief Обновить номера глав
     */
    void updateNumbering();


    /**
     * @brief Родительский элемент
     */
    SimpleTextModel* q = nullptr;

    /**
     * @brief Название документа
     */
    QString name;
};

SimpleTextModel::Implementation::Implementation(SimpleTextModel* _q)
    : q(_q)
{
}

TextModelItem* SimpleTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}

void SimpleTextModel::Implementation::updateDocumentName(const QModelIndex& _index)
{
    if (_index.isValid()) {
        return;
    }

    const auto item = firstTextItem(rootItem());
    if (item == nullptr || item->type() != TextModelItemType::Text) {
        q->setDocumentName({});
    } else {
        const auto textItem = static_cast<TextModelTextItem*>(item);
        q->setDocumentName(textItem->text());
    }
}


void SimpleTextModel::Implementation::updateNumbering()
{
    int sceneNumber = 1;
    std::function<void(const TextModelItem*)> updateChildNumbering;
    updateChildNumbering = [&sceneNumber, &updateChildNumbering](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Group: {
                updateChildNumbering(childItem);
                auto chapterItem = static_cast<SimpleTextModelChapterItem*>(childItem);
                chapterItem->setNumber(sceneNumber++, {});
                break;
            }

            default:
                break;
            }
        }
    };
    updateChildNumbering(rootItem());
}


// ****


SimpleTextModel::SimpleTextModel(QObject* _parent)
    : TextModel(_parent, createFolderItem(TextFolderType::Root))
    , d(new Implementation(this))
{
    connect(this, &SimpleTextModel::dataChanged, this,
            [this](const QModelIndex& _index) { d->updateDocumentName(_index); });

    auto updateNumbering = [this] { d->updateNumbering(); };

    //
    // Обновляем счётчики после того, как операции вставки и удаления будут обработаны клиентами
    // модели (главным образом внутри прокси-моделей), т.к. обновление элемента модели может
    // приводить к падению внутри них
    //
    connect(this, &SimpleTextModel::afterRowsInserted, this, updateNumbering);
    connect(this, &SimpleTextModel::afterRowsRemoved, this, updateNumbering);
}

SimpleTextModel::~SimpleTextModel() = default;

TextModelFolderItem* SimpleTextModel::createFolderItem(TextFolderType _type) const
{
    Q_UNUSED(_type)

    return new TextModelFolderItem(this);
}

TextModelGroupItem* SimpleTextModel::createGroupItem(TextGroupType _type) const
{
    return new SimpleTextModelChapterItem(this, _type);
}

TextModelTextItem* SimpleTextModel::createTextItem() const
{
    return new TextModelTextItem(this);
}

QString SimpleTextModel::name() const
{
    return d->name;
}

void SimpleTextModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    const auto item = firstTextItem(d->rootItem());
    if (item != nullptr && item->type() == TextModelItemType::Text) {
        auto textItem = static_cast<TextModelTextItem*>(item);
        textItem->setText(_name);
    }

    d->name = _name;
    emit nameChanged(d->name);
}

void SimpleTextModel::setDocumentName(const QString& _name)
{
    setName(_name);
    emit documentNameChanged(_name);
}

void SimpleTextModel::setDocumentContent(const QByteArray& _content)
{
    clearDocument();
    document()->setContent(_content);
    initDocument();
}

QStringList SimpleTextModel::mimeTypes() const
{
    return { kMimeType };
}

void SimpleTextModel::initEmptyDocument()
{
    auto textItem = createTextItem();
    textItem->setParagraphType(TextParagraphType::Text);
    appendItem(textItem);
}

void SimpleTextModel::finalizeInitialization()
{
    d->updateDocumentName();

    emit rowsAboutToBeChanged();
    d->updateNumbering();
    emit rowsChanged();
}

void SimpleTextModel::applyPatch(const QByteArray& _patch)
{
    TextModel::applyPatch(_patch);

    d->updateNumbering();
}

} // namespace BusinessLayer
