#pragma once

#include <QTextBlockUserData>

#include <corelib_global.h>


namespace BusinessLayer {

class TextModelItem;

class CORE_LIBRARY_EXPORT SimpleTextBlockData : public QTextBlockUserData
{
public:
    explicit SimpleTextBlockData(BusinessLayer::TextModelItem* _item);
    explicit SimpleTextBlockData(const SimpleTextBlockData* _other);

    BusinessLayer::TextModelItem* item() const;

private:
    BusinessLayer::TextModelItem* m_item = nullptr;
};

} // namespace BusinessLayer
