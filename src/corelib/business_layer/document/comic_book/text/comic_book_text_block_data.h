#pragma once

#include <QTextBlockUserData>

#include <corelib_global.h>


namespace BusinessLayer {

class ComicBookTextModelItem;

class CORE_LIBRARY_EXPORT ComicBookTextBlockData : public QTextBlockUserData
{
public:
    explicit ComicBookTextBlockData(BusinessLayer::ComicBookTextModelItem* _item);
    explicit ComicBookTextBlockData(const ComicBookTextBlockData* _other);

    BusinessLayer::ComicBookTextModelItem* item() const;

private:
    BusinessLayer::ComicBookTextModelItem* m_item = nullptr;
};

} // namespace BusinessLayer
