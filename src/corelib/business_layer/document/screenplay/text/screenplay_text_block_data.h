#pragma once

#include <QTextBlockUserData>

#include <corelib_global.h>


namespace BusinessLayer {

class ScreenplayTextModelItem;

class CORE_LIBRARY_EXPORT ScreenplayTextBlockData : public QTextBlockUserData
{
public:
    explicit ScreenplayTextBlockData(BusinessLayer::ScreenplayTextModelItem* _item);
    explicit ScreenplayTextBlockData(const ScreenplayTextBlockData* _other);

    BusinessLayer::ScreenplayTextModelItem* item() const;

private:
    BusinessLayer::ScreenplayTextModelItem* m_item = nullptr;
};

} // namespace BusinessLayer
