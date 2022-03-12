#pragma once

#include <QTextBlockUserData>

#include <corelib_global.h>


namespace BusinessLayer {

class TextModelItem;

class CORE_LIBRARY_EXPORT ScreenplayTextBlockData : public QTextBlockUserData
{
public:
    explicit ScreenplayTextBlockData(BusinessLayer::TextModelItem* _item);
    explicit ScreenplayTextBlockData(const ScreenplayTextBlockData* _other);

    TextModelItem* item() const;

private:
    TextModelItem* m_item = nullptr;
};

} // namespace BusinessLayer
