#include "text_block_data.h"


namespace BusinessLayer {

TextBlockData::TextBlockData(BusinessLayer::SimpleTextModelItem* _item)
    : QTextBlockUserData()
    , m_item(_item)
{
}

TextBlockData::TextBlockData(const TextBlockData* _other)
    : QTextBlockUserData()
    , m_item(_other->m_item)
{
}

BusinessLayer::SimpleTextModelItem* TextBlockData::item() const
{
    return m_item;
}

} // namespace BusinessLayer
