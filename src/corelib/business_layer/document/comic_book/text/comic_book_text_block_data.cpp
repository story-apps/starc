#include "comic_book_text_block_data.h"


namespace BusinessLayer {

ComicBookTextBlockData::ComicBookTextBlockData(BusinessLayer::TextModelItem* _item)
    : QTextBlockUserData()
    , m_item(_item)
{
}

ComicBookTextBlockData::ComicBookTextBlockData(const ComicBookTextBlockData* _other)
    : QTextBlockUserData()
    , m_item(_other->m_item)
{
}

BusinessLayer::TextModelItem* ComicBookTextBlockData::item() const
{
    return m_item;
}

} // namespace BusinessLayer
