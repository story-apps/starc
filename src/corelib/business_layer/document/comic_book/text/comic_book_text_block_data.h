#pragma once

#include <QTextBlockUserData>

#include <corelib_global.h>


namespace BusinessLayer {

class TextModelItem;

class CORE_LIBRARY_EXPORT ComicBookTextBlockData : public QTextBlockUserData
{
public:
    explicit ComicBookTextBlockData(BusinessLayer::TextModelItem* _item);
    explicit ComicBookTextBlockData(const ComicBookTextBlockData* _other);

    BusinessLayer::TextModelItem* item() const;

private:
    BusinessLayer::TextModelItem* m_item = nullptr;
};

} // namespace BusinessLayer
