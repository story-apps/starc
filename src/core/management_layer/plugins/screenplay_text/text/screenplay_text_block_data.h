#pragma once

#include <QModelIndex>
#include <QTextBlockUserData>


namespace BusinessLayer {
    class ScreenplayTextModelItem;
}

namespace BusinessLayer
{

class ScreenplayTextBlockData : public QTextBlockUserData
{
public:
    explicit ScreenplayTextBlockData(BusinessLayer::ScreenplayTextModelItem* _item);
    explicit ScreenplayTextBlockData(const ScreenplayTextBlockData* _other);

    BusinessLayer::ScreenplayTextModelItem* item() const;

private:
    BusinessLayer::ScreenplayTextModelItem* m_item = nullptr;
};

} // namespace Ui
