#pragma once

#include <QModelIndex>
#include <QTextBlockUserData>


namespace BusinessLayer {
    class ScreenplayTextModelTextItem;
}

namespace Ui
{

class ScreenplayTextBlockData : public QTextBlockUserData
{
public:
    explicit ScreenplayTextBlockData(BusinessLayer::ScreenplayTextModelTextItem* _item);
    explicit ScreenplayTextBlockData(const ScreenplayTextBlockData* _other);

    BusinessLayer::ScreenplayTextModelTextItem* item() const;

private:
    BusinessLayer::ScreenplayTextModelTextItem* m_item = nullptr;
};

} // namespace Ui
