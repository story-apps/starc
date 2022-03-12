#include "screenplay_text_block_data.h"


namespace BusinessLayer {

ScreenplayTextBlockData::ScreenplayTextBlockData(TextModelItem* _item)
    : QTextBlockUserData()
    , m_item(_item)
{
}

ScreenplayTextBlockData::ScreenplayTextBlockData(const ScreenplayTextBlockData* _other)
    : QTextBlockUserData()
    , m_item(_other->m_item)
{
}

TextModelItem* ScreenplayTextBlockData::item() const
{
    return m_item;
}

} // namespace BusinessLayer
