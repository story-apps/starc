#pragma once

#include <corelib_global.h>

#include <QTextBlockUserData>


namespace BusinessLayer
{

class TextModelItem;

class CORE_LIBRARY_EXPORT TextBlockData : public QTextBlockUserData
{
public:
    explicit TextBlockData(BusinessLayer::TextModelItem* _item);
    explicit TextBlockData(const TextBlockData* _other);

    BusinessLayer::TextModelItem* item() const;

private:
    BusinessLayer::TextModelItem* m_item = nullptr;
};

} // namespace BusinessLayer
