#pragma once

#include <QTextBlockUserData>

#include <corelib_global.h>


namespace BusinessLayer {

class SimpleTextModelItem;

class CORE_LIBRARY_EXPORT TextBlockData : public QTextBlockUserData
{
public:
    explicit TextBlockData(BusinessLayer::SimpleTextModelItem* _item);
    explicit TextBlockData(const TextBlockData* _other);

    BusinessLayer::SimpleTextModelItem* item() const;

private:
    BusinessLayer::SimpleTextModelItem* m_item = nullptr;
};

} // namespace BusinessLayer
