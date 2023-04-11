#include "text_model_item.h"

#include <QVariant>
#include <QXmlStreamReader>


namespace BusinessLayer {

class TextModelItem::Implementation
{
public:
    Implementation(TextModelItemType _type, const TextModel* _model);

    const TextModelItemType type;
    QString icon;
    const TextModel* model = nullptr;
};

TextModelItem::Implementation::Implementation(TextModelItemType _type, const TextModel* _model)
    : type(_type)
    , model(_model)
{
}


// ****


TextModelItem::TextModelItem(TextModelItemType _type, const TextModel* _model)
    : d(new Implementation(_type, _model))
{
    Q_ASSERT(_model);
}

TextModelItem::~TextModelItem() = default;

const TextModelItemType& TextModelItem::type() const
{
    return d->type;
}

int TextModelItem::subtype() const
{
    return 0;
}

QString TextModelItem::customIcon() const
{
    return d->icon;
}

void TextModelItem::setCustomIcon(const QString& _icon)
{
    d->icon = _icon;
}

const TextModel* TextModelItem::model() const
{
    return d->model;
}

TextModelItem* TextModelItem::parent() const
{
    return static_cast<TextModelItem*>(AbstractModelItem::parent());
}

TextModelItem* TextModelItem::childAt(int _index) const
{
    return static_cast<TextModelItem*>(AbstractModelItem::childAt(_index));
}

QVariant TextModelItem::data(int _role) const
{
    if (_role == Qt::UserRole) {
        return static_cast<int>(d->type);
    }

    return {};
}

QStringRef TextModelItem::readCustomContent(QXmlStreamReader& _contentReader)
{
    return _contentReader.name();
}

QByteArray TextModelItem::customContent() const
{
    return {};
}

} // namespace BusinessLayer
