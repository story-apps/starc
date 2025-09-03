#include "simple_text_model.h"

#include "simple_text_model_chapter_item.h"
#include "simple_text_model_folder_item.h"

#include <business_layer/model/abstract_model_xml.h>
#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/simple_text_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

namespace {

const QLatin1String kDocumentNameKey("document_name");

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
     * @brief Обновить номера глав
     */
    void updateNumbering();

    /**
     * @brief Пересчитать счетчики элемента и всех детей
     */
    void updateChildrenCounters(const TextModelItem* _item, bool _force = false);


    /**
     * @brief Родительский элемент
     */
    SimpleTextModel* q = nullptr;

    /**
     * @brief Название документа
     */
    QString name;

    /**
     * @brief Отображаемое название документа
     * @note Если название не задано явно, то тут хранится текст из первой строки документа
     */
    QString displayName;

    /**
     * @brief Цвет документа
     */
    QColor color;

    /**
     * @brief Количество страниц
     */
    int textPageCount = 0;

    /**
     * @brief Последний сохранённый хэш документа
     */
    QByteArray lastContentHash;

    /**
     * @brief Запланировано ли обновление нумерации
     */
    bool isUpdateNumberingPlanned = false;
};

SimpleTextModel::Implementation::Implementation(SimpleTextModel* _q)
    : q(_q)
{
}

TextModelItem* SimpleTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}


void SimpleTextModel::Implementation::updateNumbering()
{
    if (isUpdateNumberingPlanned) {
        return;
    }

    int sceneNumber = 1;
    std::function<void(const TextModelItem*)> updateChildNumbering;
    updateChildNumbering = [&sceneNumber, &updateChildNumbering](const TextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
            case TextModelItemType::Group: {
                updateChildNumbering(childItem);
                auto groupItem = static_cast<TextModelGroupItem*>(childItem);
                if (groupItem->setNumber(sceneNumber, {})) {
                    ++sceneNumber;
                }
                groupItem->prepareNumberText("#");
                break;
            }

            default:
                break;
            }
        }
    };
    updateChildNumbering(rootItem());
}

void SimpleTextModel::Implementation::updateChildrenCounters(const TextModelItem* _item,
                                                             bool _force)
{
    for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
        auto childItem = _item->childAt(childIndex);
        switch (childItem->type()) {
        case TextModelItemType::Group: {
            updateChildrenCounters(childItem, _force);
            break;
        }

        case TextModelItemType::Text: {
            auto textItem = static_cast<TextModelTextItem*>(childItem);
            textItem->updateCounters(_force);
            break;
        }

        default:
            break;
        }
    }
}


// ****


SimpleTextModel::SimpleTextModel(QObject* _parent)
    : TextModel(_parent, createFolderItem(TextFolderType::Root))
    , d(new Implementation(this))
{
    connect(this, &SimpleTextModel::dataChanged, this,
            [this](const QModelIndex& _index) { updateDisplayName(_index); });

    auto updateCounters = [this](const QModelIndex& _index = {}) {
        if (const auto hash = contentHash(); d->lastContentHash != hash) {
            d->updateNumbering();
            d->lastContentHash = hash;
        }

        d->updateChildrenCounters(itemForIndex(_index));
    };

    //
    // Обновляем счётчики после того, как операции вставки и удаления будут обработаны клиентами
    // модели (главным образом внутри прокси-моделей), т.к. обновление элемента модели может
    // приводить к падению внутри них
    //
    connect(this, &SimpleTextModel::afterRowsInserted, this, updateCounters);
    connect(this, &SimpleTextModel::afterRowsRemoved, this, updateCounters);
    connect(this, &SimpleTextModel::modelReset, this, updateCounters);
    //
    // Если модель планируем большое изменение, то планируем отложенное обновление нумерации
    //
    connect(this, &SimpleTextModel::rowsAboutToBeChanged, this,
            [this] { d->isUpdateNumberingPlanned = true; });
    connect(this, &SimpleTextModel::rowsChanged, this, [this] {
        if (d->isUpdateNumberingPlanned) {
            d->isUpdateNumberingPlanned = false;
            d->updateNumbering();
        }
    });
}

SimpleTextModel::~SimpleTextModel() = default;

TextModelFolderItem* SimpleTextModel::createFolderItem(TextFolderType _type) const
{
    return new SimpleTextModelFolderItem(this, _type);
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
    return d->displayName;
}

void SimpleTextModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);

    updateDisplayName({});

    const auto item = firstTextItem(d->rootItem());
    if (item != nullptr && item->type() == TextModelItemType::Text) {
        auto textItem = static_cast<TextModelTextItem*>(item);
        if (textItem->text().isEmpty() && !_name.isEmpty()) {
            textItem->setText(_name);
        }
    }
}

QString SimpleTextModel::documentName() const
{
    return name();
}

void SimpleTextModel::setDocumentName(const QString& _name)
{
    setName(_name);
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

int SimpleTextModel::wordsCount() const
{
    return static_cast<SimpleTextModelFolderItem*>(d->rootItem())->wordsCount();
}

QPair<int, int> SimpleTextModel::charactersCount() const
{
    return static_cast<SimpleTextModelFolderItem*>(d->rootItem())->charactersCount();
}

int SimpleTextModel::textPageCount() const
{
    return d->textPageCount;
}

void SimpleTextModel::setTextPageCount(int _count)
{
    if (d->textPageCount == _count) {
        return;
    }

    d->textPageCount = _count;

    //
    // Создаём фейковое уведомление, чтобы оповестить клиентов
    //
    emit dataChanged(index(0, 0), index(0, 0));
}

void SimpleTextModel::updateDisplayName(const QModelIndex& _index)
{
    //
    // Если задано название, то его и отображаем
    //
    if (!d->name.isEmpty()) {
        if (d->displayName == d->name) {
            return;
        }

        d->displayName = d->name;
    }
    //
    // А если название не задано, то используем текст из первой строчки документа
    //
    else {
        //
        // ... обновление производим, только когда нужно обрабовать изменение в корне модели
        //
        if (_index.isValid()) {
            return;
        }

        const auto item = firstTextItem(d->rootItem());
        QString newDisplayName;
        if (item != nullptr && item->type() == TextModelItemType::Text) {
            const auto textItem = static_cast<TextModelTextItem*>(item);
            newDisplayName = textItem->text();
        }
        if (d->displayName == newDisplayName) {
            return;
        }

        d->displayName = newDisplayName;
    }

    emit documentNameChanged(d->displayName);
}

void SimpleTextModel::initEmptyDocument()
{
    auto textItem = createTextItem();
    textItem->setParagraphType(TextParagraphType::Text);
    appendItem(textItem);
}

void SimpleTextModel::finalizeInitialization()
{
    updateDisplayName({});

    beginChangeRows();
    d->updateNumbering();
    endChangeRows();
}

ChangeCursor SimpleTextModel::applyPatch(const QByteArray& _patch)
{
    const auto changeCursor = TextModel::applyPatch(_patch);

    d->updateNumbering();

    return changeCursor;
}

} // namespace BusinessLayer
