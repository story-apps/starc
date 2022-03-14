#include "simple_text_block_data.h"


namespace BusinessLayer {

SimpleTextBlockData::SimpleTextBlockData(BusinessLayer::TextModelItem* _item)
    : QTextBlockUserData()
    , m_item(_item)
{
}

SimpleTextBlockData::SimpleTextBlockData(const SimpleTextBlockData* _other)
    : QTextBlockUserData()
    , m_item(_other->m_item)
{
}

TextModelItem* SimpleTextBlockData::item() const
{
    return m_item;
}

} // namespace BusinessLayer
